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
    Geometry *geom = ObjectWrap::Unwrap<Geometry>(args[0]->ToObject());

    strtree->_strtree.insert(geom->_geom->getEnvelopeInternal(), NULL);

    args.GetReturnValue().Set(Undefined(isolate));
}
