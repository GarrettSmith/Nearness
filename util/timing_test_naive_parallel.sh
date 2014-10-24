
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

OUTPUT="$START_DIR/timing_test_output"
cat /dev/null > "$OUTPUT"

ITERATIONS=100
TIMEFORMAT='%3R/%3U/%3S'

cd "/home/garrett/dev/maximal_clique"

function finish {
    killall -TERM parallel
    exit 1
}

trap 'finish' INT QUIT TERM EXIT

for (( i = 0; i < ${#DATA_DIRS[@]}; i++ ))
do
    echo "${DATA_DIRS[$i]}" >> "$OUTPUT"
    echo "${DATA_DIRS[$i]}"
    echo >> "$OUTPUT"

    for e in $(seq $MIN_EPSILON $EPSILON_STEP $MAX_EPSILON)
    do
        echo "e = $e" >> "$OUTPUT"
        echo "$e"

        function record {
            rSum=0
            uSum=0
            sSum=0

            while read data
            do
                r=`echo "$data" | cut -d / -f 1`
                u=`echo "$data" | cut -d / -f 2`
                s=`echo "$data" | cut -d / -f 3`
            done

            rSum=`echo "$rSum+$r" | bc`
            uSum=`echo "$uSum+$u" | bc`
            sSum=`echo "$sSum+$s" | bc`

            echo "Real   `echo "$rSum*1000/$ITERATIONS" | bc`ms" >> "$OUTPUT"
            echo "User   `echo "$uSum*1000/$ITERATIONS" | bc`ms" >> "$OUTPUT"
            echo "System `echo "$sSum*1000/$ITERATIONS" | bc`ms" >> "$OUTPUT"
            echo >> "$OUTPUT"
        }

        function run {
            TIMEFORMAT='%3R/%3U/%3S'
            cd "/home/garrett/dev/maximal_clique"
            TIME=`(time $1) 2>&1`
            echo "$TIME"
        }


        command="$BIN file ${DATA_DIRS[$i]}/InputData.txt $ARGS features ${FEATURES[$i]} epsilon $e"

        export -f 'run'

        seq ${ITERATIONS} | echo "$command" | parallel --group run | record


    done
done
