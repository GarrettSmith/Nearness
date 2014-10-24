OBJECTS="$2"
ORIG_OBJECTS="1000"
i="-1"
while read line
do
    let "i = $i + 1"

    let "x = $i / $ORIG_OBJECTS"
    if [ "$x" -ge "$OBJECTS" ]
    then
        break
    fi

    let "y = $i % $ORIG_OBJECTS"
    if [ "$y" -lt "$OBJECTS" ]
    then
        echo "$line"
    fi
done < "$1"
