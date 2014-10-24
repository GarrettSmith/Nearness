#!/bin/bash
#
# Finds all cliques that are subsets of another clique in the same file
#
# Garrett Smith <garrettwsmith@gmail.com>
# 05/22/2013

if [ ! -f "$1" ]
then
    echo "No file given"
    exit 1
fi

file="$1"

while read line
do
    regex=$(echo "$line" | sed 's/\t/.*/g' | sed -r 's/[0-9]+/\\<&\\>/g' )
    result=$(grep "$regex" "$file")
    count=$(echo "$result" | wc -l)
    if [ "$count" -gt 1 ]
    then
        echo "$line"
    fi
done < "$file"