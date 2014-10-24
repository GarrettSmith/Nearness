#ifndef MAXIMAL_CLIQUE_BASIC_INCLUDES
#define MAXIMAL_CLIQUE_BASIC_INCLUDES

#include <boost/dynamic_bitset.hpp>

#include <bitset>
#include <iostream>
#include <sstream>
#include <string>
#include <time.h>

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

#ifndef MAX_VERTICES
    #define MAX_VERTICES 512
#endif

// flag set to use dynamic bitsets
#ifdef DYNAMIC_BITSET
    typedef boost::dynamic_bitset<> IdSet;
#else
    typedef std::bitset<MAX_VERTICES> IdSet;
#endif

const static int NONE = -1;

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