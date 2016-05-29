#include "strtree.hpp"

#include "geometry.hpp"

#include <iostream>

using namespace std;

//Struct to store in GEOS STRtree with info needed after query
struct indexed_data {
    const PreparedGeometry* prep_geom;
    Persistent<Value> * returnObject;
    Geometry* geom;
};

STRtree::STRtree(int nodeCapacity) : _strtree(nodeCapacity), built(false) {
}

STRtree::~STRtree() {
    cout<<"deleting STRtree"<<endl;
    for(size_t i = 0; i< indexed_datas.size(); i++) {
        if (indexed_datas[i]->prep_geom != NULL) {
            delete indexed_datas[i]->prep_geom;
        }
        delete indexed_datas[i]->geom;
        delete indexed_datas[i];
    }
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
    int nodeCapacity = 10; // default from GEOS

    if (args.Length() > 0) {
        nodeCapacity =args[0]->NumberValue();
    }

    if (nodeCapacity < 2) {
        isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Node capcity cannot be negative.")));
        args.GetReturnValue().Set(Undefined(isolate));
        return;
    }

    STRtree *strtree;
    strtree = new STRtree(nodeCapacity);
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
    indexed_data* i = new indexed_data;

    if (args.Length() == 2) {
        i->returnObject = new Persistent<Value>(isolate, args[1]);
    } else {
        i->returnObject = new Persistent<Value>(isolate, args[0]);
    }

    Geometry *geom = ObjectWrap::Unwrap<Geometry>(args[0]->ToObject());
    i->geom = geom;
    i->prep_geom = 0;
    if (!i->prep_geom) {
        i->prep_geom = PreparedGeometryFactory::prepare(i->geom->_geom);
    }

    strtree->_strtree.insert(geom->_geom->getEnvelopeInternal(), i );

    strtree->indexed_datas.push_back(i);

    args.GetReturnValue().Set(Undefined(isolate));
}

void STRtree::Build(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = Isolate::GetCurrent();

    STRtree *strtree = ObjectWrap::Unwrap<STRtree>(args.This());

    if (!strtree->built) {
        strtree->_strtree.build();
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

    if(tryCatch.HasCaught()) {
        FatalException(isolate, tryCatch);
    }

    closure->cb.Reset();
    closure->strtree->Unref();
    closure->geom->_unref();

    delete closure;
    delete req;
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
        req->data = closure;
        uv_queue_work(uv_default_loop(), req, QueryAsync, QueryAsyncComplete);
        strtree->Ref();
        geom->_ref();
        args.GetReturnValue().Set(Undefined(isolate));
    } else {
        try {
            vector<void*> indexed_data;
            strtree->_strtree.query(geom->_geom->getEnvelopeInternal(), indexed_data);
            Local<Array> result = makeQueryResult(strtree, indexed_data, geom);

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
        indexed_data* i = (indexed_data*)*it;
        Local<Value> localReturn = Local<Value>::New(isolate, *(i->returnObject));
        if (!i->prep_geom) {
            i->prep_geom = PreparedGeometryFactory::prepare(i->geom->_geom);
        }
        if (i->prep_geom->intersects(query_geom->_geom)) {
            result->Set(valid_records, localReturn);
            valid_records++;
        }
    }

    return result;
}
