
START_DIR="$PWD"

EPSILONS=(
    0.05
    0.1
    0.2
    0.3
)

DATA_DIRS=(
    "/home/garrett/dev/maximal_clique/data/TestData_Images_603_622"
    "/home/garrett/dev/maximal_clique/data/TestData_Images_641_692"
)

FEATURES=(
    "18"
    "18"
)

BIN="/home/garrett/dev/maximal_clique/bin/maximal_clique"

ARGS="nshards 1 sort 0 clean 1"

OUTPUT="$START_DIR/results_test_output"
cat /dev/null > "$OUTPUT"

cd "/home/garrett/dev/maximal_clique"

for (( i = 0; i < ${#DATA_DIRS[@]}; i++ ))
do
    echo "${DATA_DIRS[$i]}" >> "$OUTPUT"
    echo >> "$OUTPUT"

    for e in "${EPSILONS[@]}"
    do
        echo "e = $e" >> "$OUTPUT"
        $("$BIN" file "${DATA_DIRS[$i]}"/InputData.txt "$ARGS" features "${FEATURES[$i]}" epsilon "$e")

        results=`./util/clique_difference.sh "${DATA_DIRS[$i]}"/InputData.txt_e"$e"_f"${FEATURES[$i]}"_output "${DATA_DIRS[$i]}"/e"$e".txt`
        errors=`./util/subset_check.sh "${DATA_DIRS[$i]}"/e"$e".txt`
        real_errors=`echo "$results" | grep -v "$errors"`
        count=`echo "$real_errors" | wc -l`
        if [ -z "$real_errors" ]
        then
            echo "Correct!" >> "$OUTPUT"
        else
            echo "$count error(s)" >> "$OUTPUT"
            echo "$real_errors" >> "$OUTPUT"
        fi
        echo >> "$OUTPUT"
    done
done
