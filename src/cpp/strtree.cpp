#include "strtree.hpp"

#include "geometry.hpp"

STRtree::STRtree() {
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
    Persistent<Object>* obj = new Persistent<Object>(isolate, args[0]->ToObject());
    Geometry *geom = ObjectWrap::Unwrap<Geometry>(args[0]->ToObject());

    strtree->_strtree.insert(geom->_geom->getEnvelopeInternal(), obj);

    args.GetReturnValue().Set(Undefined(isolate));
}

typedef struct {
    STRtree* strtree;
    Geometry* geom;
    Persistent<Function> cb;
    std::vector<void *> results;
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
        Null(isolate), True(isolate)
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

void STRtree::Query(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    STRtree *strtree = ObjectWrap::Unwrap<STRtree>(args.This());
    Geometry *geom = ObjectWrap::Unwrap<Geometry>(args[0]->ToObject());
    Local<Function> f = Local<Function>::Cast(args[1]);
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
            std::vector<void *> v;
            strtree->_strtree.query(geom->_geom->getEnvelopeInternal(), v);
            Local<Array> result = Array::New(isolate, v.size());
            for (size_t i = 0; i < v.size(); i++) {
                Local<Object> geom = Local<Object>::New(isolate, *(Persistent<Object>*)v[i]);
                result->Set(i, geom);
            }

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
