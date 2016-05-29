var fs = require('fs');

var PolygonLookup = require('polygon-lookup');

var geos = require('./..');
var WKTReader = geos.WKTReader;
var GeoJSONReader = geos.GeoJSONReader;
var Geometry = geos.Geometry;

var reader = new GeoJSONReader();
var tree = new geos.STRtree();

function randomLat() {
  return 180 * Math.random() - 90;
}

function randomLon() {
  return 360 * Math.random() - 180;
}

var wof_ids = [85688719];
var wof_country_ids  = [
85632287,85632229,85632281,85632263,85632247,85632271,85632231,85632245,85632259,85632241,85632249,85632295,85632227,85632299,85632285,85632235,85632215,85632257,85632223,85632243,85632261,85632207,85632269,85632253,85632213,85632233,85632189,85632187,85632181,85632171,85632191,85632179,85632185,85632167,85632161,85632173,85632765,85632729,85632781,85632763,85632793,85632747,85632717,85632749,85632703,85632785,85632735,85632755,85632709,85632757,85632761,85632721,85632713,85632751,85632773,85632365,85632329,85632305,85632393,85632347,85632371,85632391,85632317,85632331,85632339,85632359,85632395,85632319,85632303,85632379,85632385,85632335,85632315,85632355,85632357,85632397,85632323,85632343,85632361,85632307,85632369,85632383,85632313,85632351,85632325,85632373,85632997,85633805,85633813,85632529,85632581,85632505,85632593,85632547,85632571,85632591,85632531,85632545,85632559,85632541,85632519,85632503,85632599,85632535,85632509,85632523,85632569,85632521,85632583,85632553,85632513,85632551,85632511,85632533,85632573,85632681,85632663,85632605,85632693,85632647,85632671,85632691,85632639,85632645,85632659,85632603,85632627,85632679,85632685,85632667,85632635,85632609,85632657,85632623,85632643,85632675,85632661,85632607,85632625,85633789,85633763,85633793,85633739,85633745,85633779,85633735,85633755,85633723,85633769,85633041,85633001,85633009,85633051,85633287,85633229,85633293,85633217,85633259,85633241,85633249,85633279,85633285,85633267,85633275,85633269,85633237,85633253,85632465,85632487,85632429,85632405,85632491,85632431,85632439,85632441,85632449,85632495,85632403,85632499,85632401,85632467,85632455,85632443,85632475,85632461,85632407,85632469,85632483,85632437,85632413,85632451,85632433,85632425,85633129,85633163,85633105,85633147,85633171,85633159,85633135,85633143,85633121,85633111,85633331,85633345,85633341,85633337 ];


var wof_ids = wof_country_ids;//.slice(0, 100);
function loadWofFeatures(wof_ids) {
  var base_filename = '/home/julian/repos/whosonfirst/whosonfirst-data/data/';

  return wof_ids.map(function(id) {
    var id_s = id.toString();
    var id_part1 = id_s.substring(0, 3);
    var id_part2 = id_s.substring(3, 6);
    var id_part3 = id_s.substring(6, 8);

    var filename = base_filename + id_part1 + '/' + id_part2 + '/' + id_part3 + '/' + id_s + '.geojson';
    return JSON.parse(fs.readFileSync(filename))
  });
}

function featureToGeometry(feature) {
  return {
    type: feature.geometry.type,
    coordinates: feature.geometry.coordinates
  }
}

function readIntoGeosGeometry(geometry) {
  return reader.read(geometry);
}

var wof_features = loadWofFeatures(wof_ids);
var wof_geometries = wof_features.map(featureToGeometry);
var geos_wof_geometries = wof_geometries.map(readIntoGeosGeometry);

var featureCollection = {
  type: "FeatureCollection",
  features: wof_features
};

var lookup = new PolygonLookup(featureCollection);

var iterations = 10000;
var loops = 100;

var randomPoints = [];

for (var i = 0; i < iterations; i++ ) {
  randomPoints.push([randomLon(), randomLat()]);
}

var pointObjects = randomPoints.map(function(point) {
  return {
    type: "Point",
    coordinates: point
  };
});

var pointGeoObjects = pointObjects.map(function(pointObject) {
  return reader.read(pointObject);
});

geos_wof_geometries.forEach(function(geos_wof_geom) {
  tree.insert(geos_wof_geom);
});
console.log("done inserting");

//console.time('geos-plain');
//var intersections = 0;
//for(var i = 0; i < iterations; i++) {
  //geos_wof_geometries.forEach(function(geos_wof_geom) {
    //if (geos_wof_geom.intersects(pointGeoObjects[i])) {
      //intersections++;
    //}
  //});
//}
//console.timeEnd('geos-plain');
//console.log(intersections + " intersections");

//console.log();
//console.time('PolygonLookup');
//var intersections2 = 0;
//for(var i = 0; i < iterations; i++) {
  //var poly = lookup.search(randomPoints[i][0], randomPoints[i][1]);
  //if (poly) {
    //intersections2++;
  //}
//}
//console.timeEnd('PolygonLookup');
//console.log(intersections2 + " intersections");

console.log();
console.time('geos-STRtree-build');
tree.build();
console.timeEnd('geos-STRtree-build');

var intersections3 = 0;
console.log();
console.time('geos-STRtree');
for (var j = 0; j < loops; j++) {
  for (var i = 0; i < iterations; i++) {
    //var polys = tree.query(pointGeoObjects[i]);
    //intersections3 += polys.length;
    //
    tree.query(pointGeoObjects[i], function(err, polys) {
      intersections3 += polys.length;
    });
  }
}
console.timeEnd('geos-STRtree');
console.log(intersections3 + " intersections");
