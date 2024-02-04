#!/bin/bash

SITE_TITLE='Writing'
SITE_SUBTITLE='Everything longer than 280 characters'
INDEX_YML='index.yaml'
SRC_DIR='src'

echo "title: $SITE_TITLE" > $INDEX_YML
echo "subtitle: $SITE_SUBTITLE" >> $INDEX_YML
echo "pages:" >> $INDEX_YML

SRC_DOCS=$(find $SRC_DIR \
  -not -name 'index.md' \
  -maxdepth 1 \
  -name '*.md')

# Create pairs of file name and creation date
for f in $SRC_DOCS; do
  if [ -f "$f" ] && [ "$(basename "$f")" != "index.md" ]; then
    date=$(grep -h -w -m 1 'date:' "$f" | sed -e 's/\"//g' -e 's/date: //g')
    pairs+=("${f}"'|'"${date}"'')
  fi
done

# Sort the pairs after their creation date and parse the relating meta data
while IFS='' read -r pair; do
  # Trunc to filename
  file="$(echo "$pair" | awk -F '|' '{print $1}')"

  title="$(grep -h -w -m 1 'title:' "$file")"
  date="$(grep -h -w -m 1 'date:' "$file")"

  # Check for missing meta data
  if [ -z "$title" ]; then
    printf "Error: Missing title in %s\n" "$file"
    exit 1
  elif [ -z "$date" ]; then
    printf "Error: Missing date in %s\n" "$file"
    exit 1
  fi

  {
    printf "  - %s\n" "$title";
    printf "    %s\n" "$date";
    printf "    %s\n" "path: $(basename "${file%.*}.html")";
    echo
  } >> "$INDEX_YML"

done < <(printf "%s\n" "${pairs[@]}" | sort -r -t '|' -k 2)

exit 0
