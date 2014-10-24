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

// uncomment to enable debug printing
#define DEBUG

// #define DYNAMIC_BITSET

#include <boost/thread/mutex.hpp>
#include "boost/threadpool.hpp"

#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

#include <iostream>
#include <string>
#include <vector>
#include <iomanip>

#include "maximal_clique_basic_includes.hpp"

#include "convert_features.hpp"
#include "recursive.hpp"

#include "alphanum.hpp"
#include "libhungarian_c/hungarian.h"

typedef std::vector<float> Object;
typedef std::vector<float> Result;

const std::string VERSION = "1.1";

// mutex used for writing results
boost::mutex results_mutex;

/**
 * Simple adapter from fs::path to std::string to sort files in a nautral order.
 * eg. file1.txt, file2.txt, file10.txt
 * @param  p1 [First path object]
 * @param  p2 [Second path object]
 * @return    [p1 < p2]
 */
bool alphanum(const fs::path &p1, const fs::path &p2) {
    return doj::alphanum_comp(p1.string(), p2.string()) < 0;
}

/**
 * Recursively reads files and directories of files containing one feature value
 * per line.
 * @param p       [The path of the file to load]
 * @param objects [The two-dimensional vector of feature values to write to]
 */
void read_file(const fs::path &p, std::vector<Object> &objects) {
    if (fs::is_regular_file(p)) {
        Object obj;
        read_features_fast(p.string(), obj);
        objects.push_back(obj);
    }
    else if (fs::is_directory(p)) {
        typedef std::vector<fs::path> vec;
        vec v;
        copy(fs::directory_iterator(p), fs::directory_iterator(), back_inserter(v));
        sort(v.begin(), v.end(), alphanum);
        for (vec::const_iterator it(v.begin()), it_end(v.end()); it != it_end; ++it) {
            read_file(*it, objects);
        }
    }
    else {
        std::cerr << "error: Given file is neither a regular file nor a directory" << std::endl;
        assert(false);
    }
}

/**
 * Reads all given files recursively for feature values.
 * @param input   [The vector of input file names]
 * @param objects [The two-dimensional vector of feature values to write to]
 */
void read_objects(std::vector<std::string> &input, std::vector<Object> &objects) {

    for (unsigned int i = 0; i < input.size(); ++i) {
        const fs::path p(input[i]);

        try {
            if (fs::exists(p)) {
                read_file(p, objects);
            }
            else {
                std::cerr << "error: '" << p.string() << "' file does not exist" << std::endl;
                assert(false);
            }
        }
        catch (const fs::filesystem_error& ex) {
            std::cerr << ex.what() << std::endl;
            assert(false);
        }
    }
}

/**
 * Write progress bar to the console.
 * @param x [The current progress]
 * @param n [The maximum progress]
 * @param w [The width of the progress bar]
 */
static inline void loadbar(
    unsigned int x,
    unsigned int n,
    unsigned int w = 50) {

    float ratio = x/(float)n;
    unsigned int c = ratio * w;

    std::cerr << std::setw(5) << std::setprecision(2) << (ratio*100) << "% [";
    for (unsigned int i=0; i<c; ++i) std::cerr << "=";
    for (unsigned int i=c; i<w; ++i) std::cerr << " ";
    std::cerr << "]";

    // if finished move to next line
    if (x != n)
        std::cerr << '\r' << std::flush;
    else
        std::cerr << std::endl;
}

/**
 * Writes nearness values to the given file in the form i \t j \t value
 * @param out     [The path to the write to]
 * @param results [The nearness values to write]
 */
void output_results(
    std::string &out,
    std::vector<Result> &results) {

    std::ofstream out_file(out.c_str(), std::ofstream::trunc);
    for (unsigned int i = 0; i < results.size(); ++i) {
        for (unsigned int j = 0; j < results.size(); ++j) {
            // min and max are so we can treat the vector as though it is fully
            // populated. eg [i][j] == [j][i]
            out_file << i << '\t' << j << '\t'
                << results[std::min(i,j)][std::max(i,j)] << std::endl;
        }
    }
    out_file.close();
}

/**
 * Calculates the nearness of two sets given their maximal cliques.
 * @param  cliques     [The maximal clique found in the union of the two objects]
 * @param  num_objects [The total number of objects]
 * @param  singletons  [Whether to include singleton cliques in the result]
 * @return             [The nearness between the two objects]
 */
void nearness_mce(
    const unsigned int num_objects,
    const bool singletons,
    IdSet &clique,
    float &numerator,
    int &denominator) {

        unsigned int count = clique.count();

        // ignore singletons unless set otherwise
        if (singletons || count > 1) {

            unsigned int x = 0;
            unsigned int y = 0;

            for (unsigned int i = 0; i < num_objects/2; ++i) {
                if (clique[i]) ++x;
            }
            for (unsigned int i = num_objects/2; i < num_objects; ++i) {
                if (clique[i]) ++y;
            }

            numerator += (std::min(x, y) / (float) std::max(x, y)) * count;
            denominator += count;
        }
}

/**
 * Task to calculate the nearness from one object to all later objects.
 * @param i            [The outer set that will be compared]
 * @param objects      [The vector of all objects]
 * @param partial_graphs  [The vector of partial neighborhoods]
 * @param results      [The vector of nearness values to write to]
 * @param epsilon      [The epsilon value used to find the neighborhoods]
 * @param num_features [The number of features per object]
 * @param singletons   [Whether singletons should be included in the results]
 * @param total        [The total number of comparisons to be computed, used for
 *                     progress reporting]
 * @param current      [The number of completed comparisons, used for progress
 *                     reporting]
 */
void nearness_task_mce(
    const unsigned int i,
    std::vector<Object> &objects,
    std::vector<std::vector<IdSet> > &partial_graphs,
    std::vector<Result> &results,
    const float epsilon,
    const unsigned int num_features,
    const bool singletons,
    const unsigned int total,
    unsigned int &current ) {

    std::vector<float> tmp(objects.size());

    // no need to calculate the nearness to the same object
    tmp[i] = 0;

    // compare to each object that hasn't been compared to yet
    for (unsigned int j = i + 1; j < objects.size(); ++j) {

        // create the graph
        // d("Combine Graphs");
        std::vector<IdSet> graph;
        bool meet = features_to_graph(objects[i], objects[j],
            partial_graphs[i], partial_graphs[j],
            graph, epsilon, num_features);

        // if the two graphs are disjoint the can have no relevant maximal
        // cliques and thus we can assume the nearness is 0
        if (meet) {
            // find maximal cliques
            // d("Calculate Cliques");
            float numerator = 0;
            int denominator = 0;
            clique_enumerate(graph,
                boost::bind(nearness_mce,
                    graph.size(),
                    singletons,
                    _1,
                    boost::ref(numerator),
                    boost::ref(denominator)));

            // d("Calculate Nearness");
            tmp[j] = numerator / denominator;
        }
        else {
            tmp[j] = 0;
        }
    }

    results_mutex.lock();
    results[i] = tmp;
    current += (objects.size() - i);
    loadbar(current, total);
    results_mutex.unlock();
}

/**
 * Read files, calculate nearness, and output results.
 * @param input        [Vector of input files and directories]
 * @param output       [The name of the output file]
 * @param epsilon      [The epsilon value used to calculate neighborhoods]
 * @param num_features [The number of features per object]
 * @param singletons   [Whether to include singletons in the results]
 * @param num_threads  [The number of threads to run with, when set to 1 runs
 *                     in serial]
 */
void run_mce(
    std::vector<std::string> &input,
    std::string &output,
    const float epsilon,
    const unsigned int num_features,
    const bool singletons,
    const unsigned int num_threads) {

    assert(num_threads > 0);
    assert(num_features > 0);
    assert(epsilon > 0);

    d("Read Objects");
    std::vector<Object> objects;
    read_objects(input, objects);
    d_var(objects.size());

    std::vector<Result> results(objects.size());

    d("Calculate Partial Graphs");
    std::vector<std::vector<IdSet> > partial_graphs(objects.size());
    for (unsigned int i = 0; i < objects.size(); ++i) {
        features_to_graph(objects[i], partial_graphs[i], epsilon, num_features);
    }

    // progress
    unsigned int total = (objects.size() + 1) * (objects.size() / 2);
    unsigned int current = 0;

    // if in serial mode
    if (num_threads == 1) {
        d("Serial Mode");
        for (unsigned int i = 0; i < objects.size(); ++i) {
            nearness_task_mce(
                i,
                objects, partial_graphs, results,
                epsilon, num_features, singletons,
                total, current);
        }
    }
    else {
        d("Parallel Mode");
        // create a threadpool with a thread for each core
        boost::threadpool::pool threadpool(num_threads);

        // find cliques
        for (unsigned int i = 0; i < objects.size(); ++i) {
            threadpool.schedule(
                boost::bind(nearness_task_mce,
                    i,
                    boost::ref(objects), boost::ref(partial_graphs), boost::ref(results),
                    epsilon, num_features, singletons,
                    total, boost::ref(current)));
        }

        d("All tasks scheduled");

        // wait for tasks to complete
        threadpool.wait();
    }

    // output results
    d("Output");
    output_results(output, results);
}

/**
 * Task to calculate the nearness from one object to all later objects.
 * @param i            [The outer set that will be compared]
 * @param partial_graphs  [The vector of partial neighborhoods]
 * @param results      [The vector of nearness values to write to]
 * @param epsilon      [The epsilon value used to find the neighborhoods]
 * @param total        [The total number of comparisons to be computed, used for
 *                     progress reporting]
 * @param current      [The number of completed comparisons, used for progress
 *                     reporting]
 */
void nearness_task_sgmd(
    const unsigned int i,
    std::vector<std::vector<IdSet> > &partial_graphs,
    std::vector<std::vector<int> > &subset_sizes,
    std::vector<Result> &results,
    const unsigned int total,
    unsigned int &current ) {

    Result tmp(partial_graphs.size());

    // no need to calculate the nearness to the same object
    tmp[i] = 0;

    hungarian_problem_t* hungarian = new hungarian_problem_t;

    for (unsigned int j = i+1; j < partial_graphs.size(); ++j) {

        // d("Calculate Distance Matrix"); 
        std::vector<std::vector<int> > distance_matrix(partial_graphs[i].size());
        std::vector<int*> ptrs(distance_matrix.size() * partial_graphs[0].size());
        for (unsigned int k = 0; k < distance_matrix.size(); ++k) {
            distance_matrix[k].resize(partial_graphs[j].size());
            for (unsigned int l = 0; l < distance_matrix[k].size(); ++l) {
                distance_matrix[k][l] = std::abs(subset_sizes[i][k] - subset_sizes[j][l]);
                ptrs[k * distance_matrix[k].size() + l] = &distance_matrix[k][l];
            }
        }

        // d("Hungarian Algorithm");

        // setup
        hungarian_init(hungarian, &ptrs.front(), partial_graphs[i].size(), partial_graphs[j].size(), 
            HUNGARIAN_MODE_MINIMIZE_COST);
        hungarian_solve(hungarian);

        // Sum the assignment cost
        tmp[j] = 0;
        for (int k = 0; k < hungarian->num_rows; ++k) {
            for (int l = 0; l < hungarian->num_cols; ++l) {
                if (hungarian->assignment[k][l]) {
                    tmp[j] += distance_matrix[k][l];
                }
            }
        }

        // without this we get a large memory leak
        hungarian_free(hungarian);
    }
        
    // free memory
    delete hungarian;

    results_mutex.lock();
    results[i] = tmp;
    current += (partial_graphs.size() - i);
    loadbar(current, total);
    results_mutex.unlock();
}

/**
 * Read files, calculate nearness, and output results.
 * @param input        [Vector of input files and directories]
 * @param output       [The name of the output file]
 * @param epsilon      [The epsilon value used to calculate neighborhoods]
 * @param num_features [The number of features per object]
 * @param singletons   [Whether to include singletons in the results]
 * @param num_threads  [The number of threads to run with, when set to 1 runs
 *                     in serial]
 */
void run_sgmd(
    std::vector<std::string> &input,
    std::string &output,
    const float epsilon,
    const unsigned int num_features,
    const unsigned int num_threads) {

    assert(num_threads > 0);
    assert(num_features > 0);
    assert(epsilon > 0);

    d("Read Objects");
    std::vector<Object> objects;
    read_objects(input, objects);
    d_var(objects.size());

    // size results
    std::vector<Result> results(objects.size());
    for (unsigned int i = 0; i < results.size(); ++i) {
        results[i].resize(objects.size());
    }

    d("Calculate Partial Graphs");
    std::vector<std::vector<IdSet> > partial_graphs(objects.size());
    for (unsigned int i = 0; i < objects.size(); ++i) {
        features_to_graph(objects[i], partial_graphs[i], epsilon, num_features);
    }

    d("Count Subsets Size");
    std::vector<std::vector<int> > subset_sizes(partial_graphs.size());
    for (unsigned int i = 0; i < subset_sizes.size(); ++i) {
        subset_sizes[i].resize(partial_graphs[i].size());
        for (unsigned int j = 0; j < subset_sizes[i].size(); ++j) {
            subset_sizes[i][j] = partial_graphs[i][j].count();
        }
    }

    // progress
    unsigned int total = (objects.size() + 1) * (objects.size() / 2);
    unsigned int current = 0;

    // if in serial mode
    if (num_threads == 1) {
        d("Serial Mode");
        for (unsigned int i = 0; i < objects.size(); ++i) {
            nearness_task_sgmd(
                i,
                partial_graphs, subset_sizes, results,
                total, current);
        }
    }
    else {
        d("Parallel Mode");
        // create a threadpool with a thread for each core
        boost::threadpool::pool threadpool(num_threads);

        // find cliques
        for (unsigned int i = 0; i < objects.size(); ++i) {
            threadpool.schedule(
                boost::bind(nearness_task_sgmd,
                    i,
                    boost::ref(partial_graphs), boost::ref(subset_sizes), boost::ref(results),
                    total, boost::ref(current)));
        }

        d("All tasks scheduled");

        // wait for tasks to complete
        threadpool.wait();
    }

    // output results
    d("Output");
    output_results(output, results);
}

/**
 * Read arguments then run program.
 * @param  argc [description]
 * @param  argv [description]
 * @return      [description]
 */
int main(int argc, char const *argv[]) {

    float epsilon = 0;
    int num_features = 0;
    bool singletons = false;
    std::string output;
    std::string distance_measure;
    std::vector<std::string> input;
    int num_threads;

    // Args
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Display this help message")
        ("version,v", "Display the current version")
        ("distance-measure,d", po::value<std::string>(&distance_measure)->default_value("mce"), 
        		"Determine the graph distance measure to use. Options are \'mce\'' or \'sgmd\'")
        ("epsilon,e", po::value<float>(&epsilon),
            "Set the epsilon used to determine the maximum distance allowed between neighbouring Objects in (0, sqrt(features)]")
        ("features,f", po::value<int>(&num_features),
            "Set the number of feature values per object")
        ("output,o", po::value<std::string>(&output)->default_value("output"),
            "The file to output results to")
        ("singletons", "Include singleton cliques in results")
        ("threads", po::value<int>(&num_threads)->default_value(boost::thread::hardware_concurrency()),
            "Explicitly set the number of threads to execute with. This does not include the main thread. Specifying 1 runs the test in serial mode")
        ("serial", "Runs the test in serial. This is the same as specifying '--threads=1'")
        ("input", po::value<std::vector<std::string> >(&input),
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

        // read serial arg
        if (vm.count("serial")) {
            num_threads = 1;
        }

        bool error = false;

        // ensure input files were given
        if (input.empty()) {
            std::cerr << "error: Must give at least 1 input file" << std::endl;
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

        if (num_threads < 0) {
            std::cerr << "error: Cannot use negative threads" << std::endl;
            error = true;
        }

        // exit if an error occurred
        if (error) {
            std::cout << desc << std::endl;
            return 1;
        }

        // read singleton value
        if (vm.count("singletons")) {
            singletons = true;
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

    // debug info
    d_var(epsilon);
    d_var(num_features);
    d_var(output);
    d_var(num_threads);
    d_var(distance_measure);

    // run
    if (distance_measure == "mce") {
        run_mce(input, output, epsilon, num_features, singletons, num_threads);
    }
    else if (distance_measure == "sgmd") {
        run_sgmd(input, output, epsilon, num_features, num_threads);        
    }
    else {
        std::cerr << "error: Must specify a valid distance measure" << std::endl;
        std::cout << desc << std::endl;
        return 1;
    }

    return 0;
}