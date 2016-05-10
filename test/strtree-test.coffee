#!/usr/bin/env coffee

vows = require "vows"
assert = require "assert"

WKTReader = (require "../src").WKTReader
Geometry = (require "../src").Geometry
STRtree = (require "../src").STRtree

point = (new WKTReader()).read "POINT (0.5 0.5)"
polygon = (new WKTReader()).read "POLYGON ((0 0, 0 1, 1 1, 0 1, 0 0))"
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


tests.export module
