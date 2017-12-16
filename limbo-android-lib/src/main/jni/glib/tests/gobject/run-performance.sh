#!/bin/sh
DIR=`dirname $0`;
(cd $DIR; make performance)
ID=`git rev-list --max-count=1 HEAD`
echo "Testing revision ${ID}"
$DIR/performance | tee "perf-${ID}.log"
