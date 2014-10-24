#define DEBUG
// #define TIMING

#include <string>
#include <fstream>
#include <sstream>
#include <vector>

#include "graphchi_basic_includes.hpp"

#include "maximal_clique_basic_includes.hpp"
#include "convert_features.hpp"

using namespace graphchi;

void parse_clique(const std::string &str, IdSet &clique) {
    std::istringstream iss(str);
    unsigned int vertex;
    while (iss >> vertex) {
        clique.set(vertex);
    }
}

void read_cliques(const std::string &in, std::vector<IdSet> &results) {
    std::ifstream in_file(in);

    std::string line;
    while (std::getline(in_file, line)) {
        IdSet clique;
        parse_clique(line, clique);
        results.push_back(clique);
    }

    d_var(results.size());

    in_file.close();
}

IdSet *check_valid(IdSet &clique, std::vector<IdSet> &graph) {
    IdSet *invalid = new IdSet();
    for (unsigned int i = 0; i < clique.size(); ++i) {
        if (clique[i]) {
            for (unsigned int j = 0; j < clique.size(); ++j) {
                if (clique[j]) {
                    // check that these two vertices are within an epsilon
                    if (!graph[i][j]) {
                        invalid->set(i);
                    }
                }
            }
        }
    }
    return invalid;
}

IdSet *check_missing(IdSet &clique, std::vector<IdSet> &graph) {
    IdSet *missing = new IdSet();
    for (unsigned int i = 0; i < graph.size(); ++i) {
        if (!clique[i]) {
            bool match = true;
            for (unsigned int j = 0; j < clique.size(); ++j) {
                if (clique[j]) {
                    if (!graph[i][j]) {
                        match = false;
                        break;
                    }
                }
            }
            if (match) {
                missing->set(i);
            }
        }
    }
    return missing;
}

IdSet *check_supersets(const unsigned int i, std::vector<IdSet> &cliques) {
    IdSet *supersets = new IdSet();
    for (unsigned int j = 0; j < cliques.size(); ++j) {
        if (i != j) {
            IdSet intersection = (cliques[i] & cliques[j]);
            if (intersection == cliques[i]) {
                supersets->set(i);
            }
        }
    }
    return supersets;
}

void check_cliques(
    std::vector<IdSet> &cliques,
    std::vector<IdSet> &graph,
    const std::string &out) {

    std::ofstream out_file(out, std::ofstream::trunc);

    int invalid = 0;

    // check all objects should be in each clique
    for (unsigned int i = 0; i < cliques.size(); ++i) {
        IdSet *invalid = check_valid(cliques[i], graph);
        IdSet *missing = check_missing(cliques[i], graph);
        IdSet *supersets = check_supersets(i, cliques);

        bool any = invalid->any() || missing->any() || supersets->any();

        if (any) {
            out_file << "Clique " << (i + 1) << std::endl
                << clique_to_string(cliques[i]) << std::endl;
        }

        if (invalid->any()) {
            out_file << "Invalid vertices\n" << clique_to_string(*invalid) << std::endl;
        }
        delete invalid;

        if (missing->any()) {
            out_file << "Missing vertices\n" << clique_to_string(*missing) << std::endl;
        }
        delete missing;

        if (supersets->any()) {
            out_file << "Supersets" << std::endl;
            for (unsigned int j = 0; j < supersets->size(); ++j) {
                if ((*supersets)[j]) {
                    out_file << clique_to_string(cliques[j]) << std::endl;
                }
            }
        }
        delete supersets;

        if (any) {
            ++invalid;
            out_file << std::endl;
        }
    }

    if (invalid == 0) {
        out_file << "No errors found" << std::endl;
    }

    out_file.close();
}

int main(int argc, const  char ** argv) {
    create_timing(total);
    start_timing(total);

    /* GraphChi initialization will read the command line
       arguments and the configuration file. */
    graphchi_init(argc, argv);

    // read args
    std::string features_file = get_option_string("features_file", "");
    std::string clique_file = get_option_string("clique_file", "");
    std::string output = get_option_string("output", "test_output");
    float epsilon = get_option_float("epsilon", 0);
    int features = get_option_int("features", 0);

    // ensure required args given
    assert(features_file != "");
    assert(file_exists(features_file));
    assert(clique_file != "");
    assert(file_exists(clique_file));
    assert(epsilon > 0);
    assert(features > 0);

    // read in features
    std::vector<IdSet> graph;
    read_features(features_file, graph, epsilon, features);

    // read in cliques
    std::vector<IdSet> cliques;
    read_cliques(clique_file, cliques);

    check_cliques(cliques, graph, output);

    stop_timing(total);
    report_timing(total);
}