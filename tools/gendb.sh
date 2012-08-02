#!/bin/sh
rm -f db.s3db
sqlite3 db.s3db < sql/schema.sqlite
sqlite3 db.s3db < sql/sampleworld.sql
