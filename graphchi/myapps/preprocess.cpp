// #define DEBUG
// #define TIMING
#define OUTPUTLEVEL 4 // fatal logging only

#include <string>
#include <assert.h>

#include "graphchi_basic_includes.hpp"

#include "maximal_clique_basic_includes.hpp"
#include "convert_features.hpp"

using namespace graphchi;

int main(int argc, const  char ** argv) {
    create_timing(total);
    start_timing(total);
    create_timing(convert);
    create_timing(preprocess);

    /* GraphChi initialization will read the command line
       arguments and the configuration file. */
    graphchi_init(argc, argv);

    // input file
    std::string orig_filename = get_option_string("file", "");

    float epsilon = get_option_float("epsilon", 0);
    int features = get_option_int("features", 0);

    assert(orig_filename != "");
    assert(epsilon > 0);
    assert(features > 0);

    // convert features
    start_timing(convert);
    std::string converted = convert_features(orig_filename, epsilon, features);
    stop_timing(convert);

    // convert to grapchi format
    start_timing(preprocess);
    convert_if_notexists<EdgeDataType>(converted,
        get_option_string("nshards", "auto"));
    stop_timing(preprocess);

    stop_timing(total);

    report_timing(convert);
    report_timing(preprocess);
    report_timing(total);

    return 0;
}