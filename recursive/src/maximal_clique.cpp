/*    This file is part of Maximal Clique Nearness.
 *
 *    Maximal Clique Nearness is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    Maximal Clique Nearness is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Maximal Clique Nearness.  If not, see <http://www.gnu.org/licenses/>.
 */

// #define DEBUG

#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include <string>
#include <iostream>
#include <vector>
#include <assert.h>
#include <fstream>
#include <utility>
#include <algorithm>

#include "convert_features.hpp"
#include "recursive.hpp"

const std::string VERSION = "1.0";

int main(int argc, const char ** argv) {

    // read args
    bool sort_output = true;
    float epsilon = 0;
    int num_features = 0;
    bool singletons = false;
    std::string filename;
    std::string output;

    // Args
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Display this help message")
        ("version,v", "Display the current version")
        ("epsilon,e", po::value<float>(&epsilon),
            "Set the epsilon used to determine the maximum distance allowed between neighbouring Objects in (0, sqrt(features)]")
        ("features,f", po::value<int>(&num_features),
            "Set the number of feature values per object")
        ("output,o",
            po::value<std::string>(&output)->
                default_value("maximal_clique_output"),
            "The file to output results to")
        ("singletons", "Include singleton cliques in results")
        ("disable-sorting", "Disable sorting the output cliques")
        ("input", po::value<std::string>(&filename),
            "The list of input feature files")
    ;
    try {

        // read trailing args as input files
        po::positional_options_description p;
        p.add("input", -1);

        po::variables_map vm;
        po::store(
            po::command_line_parser(argc, argv).positional(p).
                options(desc).
                run(),
            vm);
        po::notify(vm);

        // print help
        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return 0;
        }

        // print version
        if (vm.count("version")) {
            std::cout << VERSION << std::endl;
            return 0;
        }

        bool error = false;

        // ensure input files were given
        if (filename == "") {
            std::cerr << "error: Must give an input file" << std::endl;
            error = true;
        }

        //ensure features were given
        if (num_features <= 0) {
            std::cerr << "error: Must specify a number of features greater than 0" << std::endl;
            error = true;
        }

        // ensure valid epsilon was given
        if (!(0 < epsilon && epsilon <= std::sqrt(num_features))) {
            std::cerr << "error: Must specify an epsilon in (0, sqrt(features)]" << std::endl;
            error = true;
        }

        // exit if an error occurred
        if (error) return 1;

        // read singleton value
        if (vm.count("singletons")) {
            singletons = true;
        }

        // read sorting arg
        if (vm.count("disable-sorting")) {
            sort_output = false;
        }
    }
    catch(std::exception& e) {
        std::cerr << "error: " << e.what() << std::endl;
        std::cerr << desc << std::endl;
        return 1;
    }
    catch(...) {
        std::cerr << "Exception of unknown type!" << std::endl;
    }

    // debug
    d_var(epsilon);
    d_var(num_features);
    d_var(filename);
    d_var(output);
    d_var(sort_output);

    std::vector<float> features;
    std::vector<IdSet> graph;
    read_features_fast(filename, features);
    features_to_graph(features, graph, epsilon, num_features);

    /* Run */
    std::vector<IdSet> results;
    clique_enumerate(graph, results);

    /* record Results */
    if (sort_output) std::sort(results.begin(), results.end(), clique_compare);

    std::ofstream out_file(output.c_str(), std::ofstream::trunc);
    for (std::vector<IdSet>::iterator it = results.begin(); it != results.end(); ++it) {
        if (!singletons) {
            if (it->count() == 1) continue; // skip singeltons
        }
        out_file << clique_to_string(*it) << std::endl;
    }
    out_file.close();

    return 0;
}
