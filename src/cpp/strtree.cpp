#include "strtree.hpp"

#include "geometry.hpp"

STRtree::STRtree() : built(false) {
}

STRtree::~STRtree() {
}

Persistent<Function> STRtree::constructor;

void STRtree::Initialize(Handle<Object> target) {
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);

    Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, STRtree::New);

    tpl->InstanceTemplate()->SetInternalFieldCount(1);
    tpl->SetClassName(String::NewFromUtf8(isolate, "STRtree"));

    NODE_SET_PROTOTYPE_METHOD(tpl, "insert", Insert);
    NODE_SET_PROTOTYPE_METHOD(tpl, "build", Build);
    NODE_SET_PROTOTYPE_METHOD(tpl, "query", Query);

    constructor.Reset(isolate, tpl->GetFunction());

    target->Set(String::NewFromUtf8(isolate, "STRtree"), tpl->GetFunction());
}

void STRtree::New(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);

    STRtree *strtree;
    strtree = new STRtree();
    strtree->Wrap(args.This());
    args.GetReturnValue().Set(args.This());
}

void STRtree::Insert(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = Isolate::GetCurrent();

    STRtree *strtree = ObjectWrap::Unwrap<STRtree>(args.This());

    if (strtree->built) {
        isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Cannot insert after STRtree has been built.")));
        args.GetReturnValue().Set(Undefined(isolate));
        return;
    }

    Geometry *geom = ObjectWrap::Unwrap<Geometry>(args[0]->ToObject());
    const PreparedGeometry* prep_geom = PreparedGeometryFactory::prepare(geom->_geom);

    Persistent<Object>* obj = new Persistent<Object>(isolate, args[0]->ToObject());
    strtree->prep_geoms.push_back(prep_geom);
    strtree->persistent_objs.push_back(obj);

    int* index = new int(strtree->persistent_objs.size() -1);

    strtree->_strtree.insert(geom->_geom->getEnvelopeInternal(), index );

    args.GetReturnValue().Set(Undefined(isolate));
}

void STRtree::Build(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = Isolate::GetCurrent();

    STRtree *strtree = ObjectWrap::Unwrap<STRtree>(args.This());

    if (!strtree->built) {
        strtree->_strtree.build();
        strtree->built = true;
    }

    args.GetReturnValue().Set(Undefined(isolate));
}

typedef struct {
    STRtree* strtree;
    Geometry* geom;
    Persistent<Function> cb;
    vector<void *> results;
} query_baton_t;

void STRtree::QueryAsync(uv_work_t *req) {
    query_baton_t *closure = static_cast<query_baton_t *>(req->data);
    closure->strtree->_strtree.query(closure->geom->_geom->getEnvelopeInternal(), closure->results);
}

void STRtree::QueryAsyncComplete(uv_work_t *req, int status) {
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);

    assert(status == 0);
    query_baton_t *closure = static_cast<query_baton_t *>(req->data);
    Local<Value> argv[2] = {
        Null(isolate), makeQueryResult(closure->strtree, closure->results, closure->geom)
    };
    TryCatch tryCatch;
    Local<Function> local_callback = Local<Function>::New(isolate, closure->cb);
    local_callback->Call(isolate->GetCurrentContext()->Global(), 2, argv);

    if (tryCatch.HasCaught()) {
        FatalException(isolate, tryCatch);
    }

    closure->cb.Reset();
    closure->strtree->Unref();
    closure->geom->_unref();
    uv_unref((uv_handle_t*) req);

    delete closure;
}

void STRtree::Query(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    STRtree *strtree = ObjectWrap::Unwrap<STRtree>(args.This());
    Geometry *geom = ObjectWrap::Unwrap<Geometry>(args[0]->ToObject());
    Local<Function> f = Local<Function>::Cast(args[1]);
    strtree->built = true;
    if (args.Length() == 2) {
        query_baton_t *closure = new query_baton_t();
        closure->strtree = strtree;
        closure->geom = geom;
        closure->cb.Reset(isolate, Persistent<Function>(isolate, f));
        uv_work_t *req = new uv_work_t;
        uv_ref((uv_handle_t*) req);
        req->data = closure;
        uv_queue_work(uv_default_loop(), req, QueryAsync, QueryAsyncComplete);
        strtree->Ref();
        geom->_ref();
        args.GetReturnValue().Set(Undefined(isolate));
    } else {
        try {
            vector<void*> indices;
            strtree->_strtree.query(geom->_geom->getEnvelopeInternal(), indices);
            Local<Array> result = makeQueryResult(strtree, indices, geom);

            args.GetReturnValue().Set(result);
            return;
        } catch(geos::util::GEOSException exception) {
            isolate->ThrowException(
              Exception::Error(String::NewFromUtf8(isolate, exception.what()))
            );
        }
        args.GetReturnValue().Set(Undefined(isolate));
    }
}

/*
 * Given a list of result indices, and the geometry that is being queried against the tree,
 * compute results.
 *
 * This includes gathering all the required objects from the individual vectors,
 * and performing a full intersection test against the query geometry for each result, to ensure it's actually
 * inside the result. The GEOS STRtree only compares against bounding boxes.
 */
Handle<Array> STRtree::makeQueryResult(const STRtree* strtree, vector<void *> geom_query_result_indices, const Geometry* query_geom) {
    Isolate* isolate = Isolate::GetCurrent();

    int valid_records = 0;
    Local<Array> result = Array::New(isolate);
    for (vector<void*>::iterator it = geom_query_result_indices.begin(); it != geom_query_result_indices.end(); it++) {
        int index = *(int *)*it;
        Local<Object> geomLocal = Local<Object>::New(isolate, *(strtree->persistent_objs[index]));
        const PreparedGeometry* prep_geom = strtree->prep_geoms[index];
        if (prep_geom->intersects(query_geom->_geom)) {
            result->Set(valid_records, geomLocal);
            valid_records++;
        }
    }

    return result;
}
