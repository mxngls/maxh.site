#!/bin/bash

SITE_TITLE='Blog'
SITE_SUBTITLE='More than 140 characters; Less than a paper'
INDEX_YML='index.yaml'
SRC_DIR='src'

echo "title: $SITE_TITLE" > $INDEX_YML
echo "subtitle: $SITE_SUBTITLE" >> $INDEX_YML
echo "pages:" >> $INDEX_YML

SRC_DOCS=$(find $SRC_DIR \
  -not -name 'about.md' \
  -maxdepth 1 \
  -name '*.md')

for f in $SRC_DOCS; do
  {
    printf "  - %s\n" "$(grep -h -w -m 1 'title:' "$f")"
    printf "    %s\n" "$(grep -h -w -m 1 'date:' "$f")"
    printf "    %s %s\n" "path:" "$(echo "$f" | sed -e 's/src\///g' -e 's/\.md/\.html/g')"
    echo
  } >> $INDEX_YML

done
