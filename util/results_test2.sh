
START_DIR="$PWD"

EPSILONS=(
    0.05
    0.1
    0.2
    0.3
    0.4
    0.5
    0.6
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
TEST_BIN="/home/garrett/dev/maximal_clique/bin/myapps/maximal_clique_test"

TMP1=$(mktemp)
TMP2=$(mktemp)

ARGS="nshards 1 sort 0 clean 0"

OUTPUT="$START_DIR/results_test_output"
cat /dev/null > "$OUTPUT"

cd "/home/garrett/dev/maximal_clique"

for (( i = 0; i < ${#DATA_DIRS[@]}; i++ ))
do
    echo "${DATA_DIRS[$i]}" >> "$OUTPUT"
    echo >> "$OUTPUT"

    echo "${DATA_DIRS[$i]}"

    for e in "${EPSILONS[@]}"
    do
        echo "e = $e" >> "$OUTPUT"

        echo "$e"

        "$BIN" file "${DATA_DIRS[$i]}"/InputData.txt "$ARGS" features "${FEATURES[$i]}" epsilon "$e" output "$TMP1" > /dev/null
        "$TEST_BIN" features_file "${DATA_DIRS[$i]}"/InputData.txt clique_file "$TMP1" features "${FEATURES[$i]}" epsilon "$e" output "$TMP2" > /dev/null
        cat "$TMP2" >> "$OUTPUT"
        echo >> "$OUTPUT"
    done
done

echo "Complete"

rm "$TMP1"
rm "$TMP2"
