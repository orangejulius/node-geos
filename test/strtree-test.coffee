#!/usr/bin/env coffee

vows = require "vows"
assert = require "assert"

WKTReader = (require "../src").WKTReader
Geometry = (require "../src").Geometry
STRtree = (require "../src").STRtree

point = (new WKTReader()).read "POINT (0.5 0.5)"
polygon = (new WKTReader()).read "POLYGON ((0 0, 0 1, 1 1, 0 1, 0 0))"

point2 = (new WKTReader()).read "POINT (0.25 0.25)"

# square with left bottom quarter removed. has the same bbox as a square, but different intersections
polygon2 = (new WKTReader()).read "POLYGON ((0 1, 1 1, 0 1, 0.5 0, 0.5 0.5, 0 0.5, 0 1))"
item = null

tests = (vows.describe "STRtree").addBatch

  "A STRtree":
    topic: ->
      new STRtree()

    "a new instance should be an instance of STRtree": (tree) ->
      assert.instanceOf tree, STRtree

    "should have a insert function": (tree) ->
      assert.isFunction tree.insert
      assert.isUndefined tree.insert polygon

    "query for a point in a polygon should return that polygon": (tree) ->
      # this relies on the tree.insert from the last test
      assert.deepEqual tree.query(point), [polygon]

    "async query for a point in a polygon should return that polygon": (tree) ->
      # this relies on the tree.insert from the last test
      tree.query point, (err, results) ->
        assert.deepEqual results, [polygon]

    "throws an error if inserts are attempted after querying": (tree) ->
      assert.throws( ->
        tree.insert polygon
      )

    "query does not return geoms that have bbox, but not actual geom, enclosing point": () ->
      tree = new STRtree()
      tree.insert polygon
      tree.insert polygon2

      # point that intersects both polygons
      assert.deepEqual tree.query(point), [polygon, polygon2]
      # point that intersects both bboxes, but only one polygon
      assert.deepEqual tree.query(point2), [polygon2]

    "query can take second parameter for metadata that is returned on intersection with geom": () ->
      tree = new STRtree()
      tree.insert(polygon, "foo")

      assert.deepEqual tree.query(point), ["foo"]

    "async query can take second parameter for metadata that is returned on intersection with geom": () ->
      tree = new STRtree()
      tree.insert(polygon, "foo")

      tree.query point, (err, results) ->
        assert.deepEqual results, ["foo"]

    "build can be called after inserting": () ->
      tree = new STRtree()

      tree.insert polygon

      tree.build()

    "build can be called after querying (it is a noop)": () ->
      tree = new STRtree()

      tree.insert polygon
      tree.query(point)

      tree.build()

    "insert throws after build": () ->
      tree = new STRtree()

      tree.insert polygon

      tree.build()
      assert.throws( ->
        tree.insert polygon2
        "Cannot insert after STRtree has been built."
      )

    "node capacity can be set during construction": () ->
      tree = new STRtree(5)

    "node capacity cannot be set to a negative value": () ->
      assert.throws( ->
        tree = new STRtree(-3)
      )

    "node capacity cannot be set to 1": () ->
      assert.throws( ->
        tree = new STRtree(1)
      )

    "node capacity cannot be set to 0": () ->
      assert.throws( ->
        tree = new STRtree(0)
      )
tests.export module
