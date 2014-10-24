
START_DIR="$PWD"

MIN_EPSILON=0.2
MAX_EPSILON=4.24
EPSILON_STEP=0.2

DATA_DIRS=(
    "/home/garrett/dev/maximal_clique/data/TestData_Images_603_622"
    "/home/garrett/dev/maximal_clique/data/TestData_Images_641_692"
)

FEATURES=(
    "18"
    "18"
)

BIN="/home/garrett/dev/maximal_clique/bin/myapps/naive"

ARGS="sort 0"

OUTPUT="$START_DIR/timing_test_naive_output"
cat /dev/null > "$OUTPUT"

ITERATIONS=100
TIMEFORMAT='%3R/%3U/%3S'

cd "/home/garrett/dev/maximal_clique"

echo "N = $ITERATIONS" >> "$OUTPUT"
echo "$OUTPUT"
echo "N = $ITERATIONS"

for (( i = 0; i < ${#DATA_DIRS[@]}; i++ ))
do
    echo "${DATA_DIRS[$i]}" >> "$OUTPUT"
    echo "${DATA_DIRS[$i]}"
    echo >> "$OUTPUT"

    for e in $(seq $MIN_EPSILON $EPSILON_STEP $MAX_EPSILON)
    do
        echo "e = $e" >> "$OUTPUT"
        echo "$e"

        rSum=0
        uSum=0
        sSum=0

        for (( j = 0; j < ${ITERATIONS}; j++ ))
        do
            command="$BIN file ${DATA_DIRS[$i]}/InputData.txt $ARGS features ${FEATURES[$i]} epsilon $e"
            result=`(time $command) 2>&1`

            r=`echo "$result" | cut -d / -f 1`
            u=`echo "$result" | cut -d / -f 2`
            s=`echo "$result" | cut -d / -f 3`

            rSum=`echo "$rSum+$r" | bc`
            uSum=`echo "$uSum+$u" | bc`
            sSum=`echo "$sSum+$s" | bc`

        done

        echo "Real   `echo "$rSum*1000/$ITERATIONS" | bc`ms" >> "$OUTPUT"
        echo "User   `echo "$uSum*1000/$ITERATIONS" | bc`ms" >> "$OUTPUT"
        echo "System `echo "$sSum*1000/$ITERATIONS" | bc`ms" >> "$OUTPUT"
        echo >> "$OUTPUT"

    done
done
