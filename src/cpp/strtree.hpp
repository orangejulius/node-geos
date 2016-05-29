#ifndef STRTREE_HPP
#define STRTREE_HPP

#include <vector>

#include <uv.h>
#include <geos/geom/prep/PreparedGeometryFactory.h>
#include <geos/index/strtree/STRtree.h>
#include "binding.hpp"

using std::vector;
using geos::geom::prep::PreparedGeometry;
using geos::geom::prep::PreparedGeometryFactory;

class Geometry;
struct indexed_data;

class STRtree : public ObjectWrap {
 public:
    geos::index::strtree::STRtree _strtree;
    STRtree(int nodeCapacity);
    ~STRtree();
    static void Initialize(Handle<Object> target);

    static void Insert(const FunctionCallbackInfo<Value>& args);
    static void Build(const FunctionCallbackInfo<Value>& args);
    static void Query(const FunctionCallbackInfo<Value>& args);
    static void QueryAsyncComplete(uv_work_t *req, int status);
    static void QueryAsync(uv_work_t *req);

 protected:
    static void New(const FunctionCallbackInfo<Value>& args);

 private:
    bool built;
    static Persistent<Function> constructor;
    static Handle<Array> makeQueryResult(const STRtree* strtree, vector<void *> geom_query_result, const Geometry* query_geom);
	vector<indexed_data*> indexed_datas;
};

#endif