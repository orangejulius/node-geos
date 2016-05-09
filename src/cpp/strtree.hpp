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

 protected:
    static void New(const FunctionCallbackInfo<Value>& args);

 private:
    static Persistent<Function> constructor;
};

#endif
