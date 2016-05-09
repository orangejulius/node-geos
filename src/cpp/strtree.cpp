#include "strtree.hpp"

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
