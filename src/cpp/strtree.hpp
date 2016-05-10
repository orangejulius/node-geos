#ifndef STRTREE_HPP
#define STRTREE_HPP

#include <vector>

#include <uv.h>
#include <geos/index/strtree/STRtree.h>
#include "binding.hpp"

using std::vector;

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
    static Handle<Array> makeQueryResult(vector<void *> geom_query_result);
};

#endif
