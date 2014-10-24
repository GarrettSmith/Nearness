#ifndef CONVERT_FEATURES
#define CONVERT_FEATURES

#include <string>
#include <bitset>
#include <iostream>
#include <fstream>
#include <assert.h>
#include <vector>
#include <cstdio>

#include "maximal_clique_basic_includes.hpp"

using namespace graphchi;

/**
  * Checks if original file has more recent modification date
  * than the converted file. If it has, deletes the converted file and returns
  * false. Otherwise return true.
  */
bool check_origfile_modification_earlier(std::string basefilename, std::string convertedfilename) {
    if (get_option_int("disable-modtime-check", 0) == 0) {
        struct stat origstat, convertedstat;
        int err1 = stat(basefilename.c_str(), &origstat);
        int err2 = stat(convertedfilename.c_str(), &convertedstat);

        if (err1 != 0 || err2 != 0) {
            logstream(LOG_ERROR) << "Error when checking file modification times:  " << strerror(errno) << std::endl;
            return true;
        }

        if (origstat.st_mtime > convertedstat.st_mtime) {
            logstream(LOG_INFO) << "The input graph modification date was newer than of the conversion." << std::endl;
            logstream(LOG_INFO) << "Going to delete  and recreate new ones. To disable " << std::endl;
            logstream(LOG_INFO) << "functionality, specify --disable-modtime-check=1" << std::endl;

            // Delete the bin-file
            logstream(LOG_DEBUG) << "Deleting: " << convertedfilename << std::endl;
            int err = remove(convertedfilename.c_str());
            if (err != 0) {
                logstream(LOG_ERROR) << "Error deleting file: " << convertedfilename << ", " <<
                strerror(errno) << std::endl;
            }
            return false;
        } else {
            return true;
        }

    }
    return false;
}

/**
 * The squared distance between two n-degree points.
 * @param  a    [description]
 * @param  b    [description]
 * @param  size [description]
 * @return      [description]
 */
inline float distance(
    const std::vector<float> &objects,
    const int a,
    const int b,
    const int size) {

    float sum = 0;
    for (int i = 0; i < size; ++i) {
        // faster than std::pow((a[i] - b[i]),2)
        float x = objects[a + i] - objects[b + i];
        x *= x;
        sum += x;
    }
    //sum = std::sqrt(sum);
    return sum;
}

std::string output_name(
    const std::string &in,
    const float epsilon,
    const unsigned int num_features) {

    std::stringstream ss;
    ss << in << "_e" << epsilon << "_f" << num_features;
    return ss.str();
}


void read_features(
    const std::string &in,
    std::vector<IdSet> &results,
    const float epsilon,
    const unsigned int num_features) {

    // read in objects
    std::ifstream in_file(in);

    std::string line;
    unsigned int lines = 0;

    std::vector<float> objects(MAX_VERTICES * num_features);

    d_var(objects.size());

    // counts lines
    while (std::getline(in_file, line)) {
        objects[lines] = atof(line.c_str());
        ++lines;
    }

    in_file.close();

    d_var(lines);

    // ensure all objects have the right number of features
    assert(lines % num_features == 0);

    int num_objects = lines / num_features;
    d_var(num_objects);

    // compare all objects
    results.resize(num_objects);

    float sqr_epsilon = epsilon * epsilon;
    for (int i = 0; i < num_objects; ++i) {
        for (int j = i; j < num_objects; ++j) {
            if (distance(objects, i * num_features, j * num_features, num_features) < sqr_epsilon) {
                results[i].set(j);
                results[j].set(i);
            }
        }
    }
}

const int BUFFER_SIZE = 512;
void read_features_fast(
    const std::string &in,
    std::vector<IdSet> &results,
    const float epsilon,
    const unsigned int num_features) {


    unsigned int lines = 0;

    std::vector<float> objects(MAX_VERTICES * num_features);

    d_var(objects.size());

    // read in objects
    FILE *fp = fopen(in.c_str(), "rb");
    char buffer[BUFFER_SIZE];
    setvbuf(fp, (char *)NULL, _IOFBF, BUFFER_SIZE * 16);
    while (fgets(buffer, sizeof buffer, fp) != NULL) {
        objects[lines] = atof(buffer);
        d(std::string(buffer));
        ++lines;
    }
    fclose(fp);

    d_var(lines);

    // ensure all objects have the right number of features
    assert(lines % num_features == 0);

    int num_objects = lines / num_features;
    d_var(num_objects);

    // compare all objects
    results.resize(num_objects);

    float sqr_epsilon = epsilon * epsilon;
    for (int i = 0; i < num_objects; ++i) {
        for (int j = i; j < num_objects; ++j) {
            if (distance(objects, i * num_features, j * num_features, num_features) < sqr_epsilon) {
                results[i].set(j);
                results[j].set(i);
            }
        }
    }
}

// output graph
void write_adjlist(const std::string &out, std::vector<IdSet> &graph) {
    d_var(graph.size());

    std::ofstream out_file(out, std::ofstream::trunc);

    for (unsigned int i = 0; i < graph.size(); ++i) {
        out_file << i << '\t' << graph[i].count() << '\t'
            << clique_to_string(graph[i]) << std::endl;
        // for (int j = 0; j < num_objects; ++j) {
        //  out_file << results[i][j] << '\t';
        // }
        // out_file << std::endl;
    }

    out_file.close();
}

/**
 * Converts featues values to adjlist format file.
 * @param in           [description]
 * @param out          [description]
 * @param epsilon      [description]
 * @param num_features [description]
 */
std::string convert_features(
    const std::string &in,
    const float epsilon,
    const unsigned int num_features) {

    std::string out = output_name(in, epsilon, num_features);

    // ensure the in file esists
    assert(file_exists(in));
    d("Converting File");
    std::vector<IdSet> results;
    read_features(in, results, epsilon, num_features);
    d("Writing adjlist to " << out);
    write_adjlist(out, results);

    return out;
}

std::string convert_features_if_notexists(
    const std::string &in,
    const float epsilon,
    const unsigned int num_features) {

    std::string out = output_name(in, epsilon, num_features);

    // ensure the in file esists
    assert(file_exists(in));

    // if there is no output file yet or it is out of date
    if (!file_exists(out) || !check_origfile_modification_earlier(in, out)) {
        convert_features(in, epsilon, num_features);
    }

    return out;
}

#endif