EPSILONS=(
    0.1
    0.2
    0.3
    0.4
    0.5
)

INPUT="/home/garrett/dev/nearness/data/objectDescriptions"
FEATURES="18"
DISTANCE_MEASURE="sgmd"

OUTPUT_PREFIX="output_bulk"
TIME_OUTPUT=$OUTPUT_PREFIX"_time"

BIN="/home/garrett/dev/nearness/recursive/bin/nearness"

echo "$INPUT"
for e in "${EPSILONS[@]}"
do
    echo "$e"
    echo "$e" >> "$TIME_OUTPUT"
    /usr/bin/time -v "$BIN" -d "$DISTANCE_MEASURE" -e "$e" -f "$FEATURES" -o "$OUTPUT_PREFIX"_e"$e" "$INPUT"
    echo
    echo >> "$TIME_OUTPUT"
    echo "Saved to $OUTPUT_PREFIX"_e"$e"
done
