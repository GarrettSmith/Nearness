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

#ifndef CONVERT_FEATURES
#define CONVERT_FEATURES

#include <string>
#include <bitset>
#include <iostream>
#include <ios>
#include <fstream>
#include <assert.h>
#include <vector>
#include <cstdio>
#include <cstdlib>

#include "maximal_clique_basic_includes.hpp"

 /**
  * Squared euclidean distance between two n-degree points
  * @param  objects_a [The feature values of the first objects]
  * @param  objects_b [The feature values of the second objects]
  * @param  a         [The index to start from for the first object]
  * @param  b         [The index to start from for the second object]
  * @param  size      [The numbers of features]
  * @return           [The squared distance]
  */
float distance(
    const std::vector<float> &objects_a,
    const std::vector<float> &objects_b,
    const int a,
    const int b,
    const int size) {

    float sum = 0;
    for (int i = 0; i < size; ++i) {
        // faster than std::pow((a[i] - b[i]),2)
        float x = objects_a[a + i] - objects_b[b + i];
        x *= x;
        sum += x;
    }
    //sum = std::sqrt(sum);
    return sum;
}

/**
 * Helper when both objects are from the same set of features.
 */
inline float distance(
    const std::vector<float> &objects,
    const int a,
    const int b,
    const int size) {

    return distance(objects, objects, a, b, size);
}

// The size of input buffer
const int BUFFER_SIZE = 512;

/**
 * Reads input file of features values into a vector.
 * @param in      [The name of the input file]
 * @param results [The vector to put feature values into]
 */
void read_features_fast(
    const std::string &in,
    std::vector<float> &results) {

    // read in objects
    FILE *fp = fopen(in.c_str(), "rb");
    char buffer[BUFFER_SIZE];
    setvbuf(fp, (char *)NULL, _IOFBF, BUFFER_SIZE * 16);
    while (fgets(buffer, sizeof buffer, fp) != NULL) {
        results.push_back(atof(buffer));
    }
    fclose(fp);
}

/**
 * Creates a neighbourhood graph from a list of feature values.
 * @param features     [The vector of input values]
 * @param results      [The vector to output the neighbourhood graph to]
 * @param epsilon      [The epsilon value to use]
 * @param num_features [The number of features per object]
 */
void features_to_graph(
    std::vector<float> &features,
    std::vector<IdSet> &results,
    const float epsilon,
    const unsigned int num_features) {

    // ensure all objects have the right number of features
    assert(features.size() % num_features == 0);

    unsigned int num_objects = features.size() / num_features;
    // d_var(num_objects);

    results.resize(num_objects);

    #ifdef DYNAMIC_BITSET
        for (unsigned int i = 0; i < num_objects; ++i) {
            results[i].resize(num_objects * 2);
        }
    #endif

    // compare all objects
    float sqr_epsilon = epsilon * epsilon;
    for (unsigned int i = 0; i < num_objects; ++i) {
        for (unsigned int j = i + 1; j < num_objects; ++j) {
            if (distance(
                    features,
                    i * num_features,
                    j * num_features,
                    num_features) < sqr_epsilon) {
                results[i].set(j);
                results[j].set(i);
            }
        }
    }

}

/**
 * Combines two neighbourhood graphs with their feature vectors.
 * @param  features_a   [The features of the first object]
 * @param  features_b   [The features of the second object]
 * @param  graph_a      [The partial graph of the first object]
 * @param  graph_b      [The partial graph of the second object]
 * @param  results      [The combined graph]
 * @param  epsilon      [The epsilon to use]
 * @param  num_features [The number of features per object]
 * @return              [True if the two objects were not disjoint]
 */
bool features_to_graph(
    std::vector<float> &features_a,
    std::vector<float> &features_b,
    std::vector<IdSet> &graph_a,
    std::vector<IdSet> &graph_b,
    std::vector<IdSet> &results,
    const float epsilon,
    const unsigned int num_features) {

    // ensure all objects have the right number of features
    assert(features_b.size() % num_features == 0);
    assert(features_a.size() % num_features == 0);

    unsigned int num_objects_a = features_a.size() / num_features;
    unsigned int num_objects_b = features_b.size() / num_features;
    unsigned int num_objects = num_objects_a + num_objects_b;

    #ifndef DYNAMIC_BITSET
        assert(num_objects <= MAX_VERTICES);
    #endif

    // copy starting graph
    results = graph_a;

    // size bitsets
    results.resize(num_objects);

    // add shifted second graph
    for (unsigned int i = 0; i < num_objects_b; ++i) {
        results[num_objects_a + i] = graph_b[i] << num_objects_a;
    }

    // if the two graphs meet can be used to optimize
    bool meet = false;


    // compare between the two graphs
    float sqr_epsilon = epsilon * epsilon;
    for (unsigned int i = 0; i < num_objects_a; ++i) {
        for (unsigned int j = num_objects_a; j < num_objects; ++j) {
            if (distance(
                    features_a,
                    features_b,
                    i * num_features,
                    (j - num_objects_a) * num_features,
                    num_features) < sqr_epsilon) {
                results[i].set(j);
                results[j].set(i);
                meet = true;
            }
        }
    }

    return meet;
}

#endif