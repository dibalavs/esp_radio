#!/bin/bash

dst=../spiffs_image/w
src=dist

src_files=$(find $src/ -type f  \
  \( -name "*.js" ! -name "mockServiceWorker.js" \
  -o -name "*.css"    \
  -o -name "*.html"   \
  -o -name "*.ico" \) \
  -printf "%P\n")


rm -r -f $dst

for i in $src_files
do
  echo Install:$i
  mkdir -p `dirname $dst/$i`
  gzip -9 -c $src/$i > $dst/$i.gz
done