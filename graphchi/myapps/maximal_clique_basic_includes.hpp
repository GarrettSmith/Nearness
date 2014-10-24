#ifndef MAXIMAL_CLIQUE_BASIC_INCLUDES
#define MAXIMAL_CLIQUE_BASIC_INCLUDES

#include <bitset>
#include <iostream>
#include <string>
#include <time.h>
#include <mutex>

// Debug macros
#ifdef DEBUG
    #define d(x) do {std::cout << __LINE__ << ":'" << __FUNCTION__ << "' " << x << std::endl; } while (0)
    #define d_var(x) do { d(#x << " = " << x); } while (0)
    #define d_clique_var(x) do { d(#x << " = {" << clique_to_string(x) << "}"); } while (0)
#else
    #define d(x)
    #define d_var(x)
    #define d_clique_var(x)
#endif

// Timing Macros
#ifdef TIMING
    #define create_timing(label) clock_t start_clock_##label = 0; clock_t stop_clock_##label = 0; int count_##label = 0; clock_t total_clock_##label = 0;
    #define start_timing(label) start_clock_##label = std::clock()
    #define stop_timing(label) stop_clock_##label = std::clock(); ++count_##label; total_clock_##label += stop_clock_##label - start_clock_##label
    #define get_timing(label) ((float)total_clock_##label / CLOCKS_PER_SEC * count_##label)
    #define report_timing(label) printf("%12s %2.2fs\n", #label, get_timing(label))
#else
    #define create_timing(label)
    #define start_timing(label)
    #define stop_timing(label)
    #define get_timing(label)
    #define report_timing(label)
#endif

using namespace graphchi;

const static size_t MAX_VERTICES = 512;

typedef std::bitset<MAX_VERTICES> IdSet;

/**
 * The data structure of a vertex.
 *
 */
struct VertexData {
    IdSet _neighbours;
    bool _neighbours_set;
    int _cycles;

    VertexData(IdSet ids) : _neighbours_set(false), _cycles(0) {}

    void set_neighbours(const IdSet &ids) {
        _neighbours = ids;
        _neighbours_set = true;
    }
};

/**
 * A message contains the current path taken to find the clique and the
 * candidates that can still be considered.
 */
struct Message {
    bool _set;
    IdSet _current_clique;
    IdSet _candidates;
    IdSet _not;

    Message() : _set(false) {}

    void set(const IdSet &clique, const IdSet &cands, const IdSet &not_ids) {
        _set = true;
        _current_clique = clique;
        _candidates = cands;
        _not = not_ids;
    }

    void unset() {
        _set = false;
    }
};

/**
 * The data structure of an edge.
 * Provides the neighbours of the node and an optional message to find cliques.
 */
struct EdgeData {
    IdSet _neighbours;
    Message _message;
    IdSet _block_trace;

    EdgeData() : _neighbours(), _block_trace() {}

};

/**
 * Type definitions. Remember to create suitable graph shards using the
 * Sharder-program.
 */
typedef VertexData VertexDataType;
typedef EdgeData EdgeDataType;
typedef graphchi_vertex<VertexDataType, EdgeDataType> Vertex;

/**
 * Pretty print bitsets.
 * @param  clique [description]
 * @return        [description]
 */
std::string clique_to_string(IdSet clique) {
    std::stringstream ss;
    bool first = true;
    for (size_t i = 0; i < clique.size(); ++i) {
        if (clique[i]) {
            if (!first) ss << '\t';
            ss << i;
            first = false;
        }
    }
    return ss.str();
}

/**
 * Comparison function used to sort cliques
 * @param  a [description]
 * @param  b [description]
 * @return   [description]
 */
bool clique_compare(const IdSet &a, const IdSet &b) {
    if (a.count() == b.count()) {
        for (size_t i = 0; i < a.size(); i++) {
            if (a[i] && !b[i]) return true;
            if (!a[i] && b[i]) return false;
        }
        return false;
    }
    else {
        return a.count() < b.count();
    }
}

#endif