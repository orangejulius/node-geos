#ifndef STRTREE_HPP
#define STRTREE_HPP

#include <uv.h>
#include <geos/index/strtree/STRtree.h>
#include "binding.hpp"

class STRtree : public ObjectWrap {
 public:
    geos::index::strtree::STRtree _strtree;
    STRtree();
    ~STRtree();
    static void Initialize(Handle<Object> target);

    static void Insert(const FunctionCallbackInfo<Value>& args);
    static void Query(const FunctionCallbackInfo<Value>& args);
    static void QueryAsyncComplete(uv_work_t *req, int status);
    static void QueryAsync(uv_work_t *req);

 protected:
    static void New(const FunctionCallbackInfo<Value>& args);

 private:
    static Persistent<Function> constructor;
};

#endif
