var geos = require('./..');
var WKTReader = geos.WKTReader;
var GeoJSONReader = geos.GeoJSONReader;
var Geometry = geos.Geometry;

var reader = new WKTReader();

var enclosing_object = {
  geometry: reader.read("Point (1 1)")
};

console.log(enclosing_object);

delete enclosing_object.geometry;

console.log(enclosing_object);
gc();
console.log(enclosing_object);
