#!/bin/bash

SITE_TITLE='Posts'
INDEX_YML='index.yaml'
SRC_DIR='src'

echo "title: $SITE_TITLE" > $INDEX_YML
echo 'pages:' >> $INDEX_YML

find $SRC_DIR \
  -name '*.md' \
  -exec  bash -c 'grep -h -w -m 1 title: {} | xargs -0 printf "\ \ -\ %s"' ";" \
  -exec  bash -c 'echo {} | sed -e '\''s/src\///g'\'' -e "s/\.md/\.html/g" | xargs -0 printf "    %s %s" "path:"' ";" \
  -exec  bash -c 'grep -h -w -m 1 date: {}  | xargs -0 printf "\ \ \ \ %s\n"' ";" >> $INDEX_YML
