var fs = require('fs');
var simplify = require('simplify');


var geos = require('./..');
var WKTReader = geos.WKTReader;
var GeoJSONReader = geos.GeoJSONReader;
var Geometry = geos.Geometry;

var reader = new GeoJSONReader();

var o = {
  tree: new geos.STRtree()
};

function randomLat() {
  return 180 * Math.random() - 90;
}

function randomLon() {
  return 360 * Math.random() - 180;
}

function simplifyGeometry(geometry) {
  if( geometry ) {
    if ('Polygon' === geometry.type) {
      var coordinates = geometry.coordinates[0];
      if (coordinates.length > 3) {
        geometry.coordinates[0] = simplifyCoords(coordinates);
      }
    }
    else if ('MultiPolygon' === geometry.type) {
      var polygons = geometry.coordinates;
      polygons.forEach(function simplify(coordinates, idx) {
        var coords = simplifyCoords(coordinates[0]);
        if (coords.length > 3) {
          polygons[idx][0] = coords;
        }
      });
    }
  }

  return geometry;
}

/**
 * @param {array} coords A 2D GeoJson-style points array.
 * @return {array} A slightly simplified version of `coords`.
 */
function simplifyCoords( coords ){
  var pts = coords.map( function mapToSimplifyFmt( pt ){
    return { x: pt[ 0 ], y: pt[ 1 ] };
  });

  var simplificationRate = 0.0003;
  var simplified = simplify( pts, simplificationRate, true );

  return simplified.map( function mapToGeoJsonFmt( pt ){
    return [ pt.x, pt.y ];
  });
}

var wof_ids = [85688719];
var wof_country_ids  = [
85632287,85632229,85632281,85632263,85632247,85632271,85632231,85632245,85632259,85632241,85632249,85632295,85632227,85632299,85632285,85632235,85632215,85632257,85632223,85632243,85632261,85632207,85632269,85632253,85632213,85632233,85632189,85632187,85632181,85632171,85632191,85632179,85632185,85632167,85632161,85632173,85632765,85632729,85632781,85632763,85632793,85632747,85632717,85632749,85632703,85632785,85632735,85632755,85632709,85632757,85632761,85632721,85632713,85632751,85632773,85632365,85632329,85632305,85632393,85632347,85632371,85632391,85632317,85632331,85632339,85632359,85632395,85632319,85632303,85632379,85632385,85632335,85632315,85632355,85632357,85632397,85632323,85632343,85632361,85632307,85632369,85632383,85632313,85632351,85632325,85632373,85632997,85633805,85633813,85632529,85632581,85632505,85632593,85632547,85632571,85632591,85632531,85632545,85632559,85632541,85632519,85632503,85632599,85632535,85632509,85632523,85632569,85632521,85632583,85632553,85632513,85632551,85632511,85632533,85632573,85632681,85632663,85632605,85632693,85632647,85632671,85632691,85632639,85632645,85632659,85632603,85632627,85632679,85632685,85632667,85632635,85632609,85632657,85632623,85632643,85632675,85632661,85632607,85632625,85633789,85633763,85633793,85633739,85633745,85633779,85633735,85633755,85633723,85633769,85633041,85633001,85633009,85633051,85633287,85633229,85633293,85633217,85633259,85633241,85633249,85633279,85633285,85633267,85633275,85633269,85633237,85633253,85632465,85632487,85632429,85632405,85632491,85632431,85632439,85632441,85632449,85632495,85632403,85632499,85632401,85632467,85632455,85632443,85632475,85632461,85632407,85632469,85632483,85632437,85632413,85632451,85632433,85632425,85633129,85633163,85633105,85633147,85633171,85633159,85633135,85633143,85633121,85633111,85633331,85633341,85633337 ];

var wof_locality_ids = JSON.parse(fs.readFileSync('/home/julian/wof-locality-latest.json'));

console.log("loaded " + wof_locality_ids.length + " localities");

//var wof_ids = wof_country_ids.slice(0, 1);
var wof_ids = wof_locality_ids.slice(0, 10000);

function loadWofFeature(id) {
  var base_filename = '/home/julian/repos/whosonfirst/whosonfirst-data/data/';

  var id_s = id.toString();
  var id_part1 = id_s.substring(0, 3);
  var id_part2 = id_s.substring(3, 6);
  var id_part3 = id_s.substring(6, 8);

  var filename = base_filename + id_part1 + '/' + id_part2 + '/' + id_part3 + '/' + id_s + '.geojson';
  return JSON.parse(fs.readFileSync(filename))
};

var i = 0;
wof_ids.forEach(function(id) {
  var wof_data = loadWofFeature(id)
  var wof_geom = featureToGeometry(wof_data);
  wof_geom = simplifyGeometry(wof_geom);
  var geos_geom = readIntoGeosGeometry(wof_geom);
  o.tree.insert(geos_geom, wof_data.properties);
  if (i % 500 == 0) {
    console.log(i);
    console.log(process.memoryUsage());
  }
  i++;
});

function featureToGeometry(feature) {
  return {
    type: feature.geometry.type,
    coordinates: feature.geometry.coordinates
  }
}

function readIntoGeosGeometry(geometry) {
  return reader.read(geometry);
}

var iterations = 10;
var loops = 10;

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

console.log();
console.time('geos-STRtree-build');
o.tree.build();
console.timeEnd('geos-STRtree-build');

var intersections3 = 0;
console.log();
console.time('geos-STRtree');
var queriesDone = 0;
for (var j = 0; j < loops; j++) {
  for (var i = 0; i < iterations; i++) {
    //var polys = tree.query(pointGeoObjects[i]);
    //intersections3 += polys.length;

    o.tree.query(pointGeoObjects[i], function(err, polys) {
      queriesDone++;
      intersections3 += polys.length;
      if (queriesDone == loops * iterations) {
        console.timeEnd('geos-STRtree');
      }
    });
  }
}
console.timeEnd('geos-STRtree');
console.log(intersections3 + " intersections");
