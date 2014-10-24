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

#ifndef RECURSIVE_ALGORITHM
#define RECURSIVE_ALGORITHM

#include "boost/function.hpp"
#include "boost/bind.hpp"
#include "maximal_clique_basic_includes.hpp"

/**
 * Find the candidate with the greatest neighbourhoods in candidates.
 * @param  cands [The candidates to pick from]
 * @param  graph [The graph]
 * @return       [The vertex id of the candidate with the most neighbours within
 *               cands]
 */
int greatest_cand(IdSet &cands, std::vector<IdSet> &graph) {
    int fixp = NONE;
    int num_neighbours = -1;

    for (unsigned int i = 0; i < graph.size(); ++i) {
        if (cands[i]) {
            int iteration_neighbours = (cands & graph[i]).count();
            if (iteration_neighbours > num_neighbours) {
                fixp = i;
                num_neighbours = iteration_neighbours;
            }
        }
    }

    return fixp;
}

/**
 * Find the next vertex id to move to.
 * @param  cands [The candidates to pick from]
 * @param  fixp  [The starting vertex id]
 * @param  graph [The graph]
 * @return       [The next vertex id to move to]
 */
int remaining_v(IdSet &cands, const int fixp, std::vector<IdSet> &graph) {
    int cur_v = NONE;

    if (cands.any()) { // hack to skip loop if there are no more cands
        for (unsigned int i = 0; i < graph.size(); ++i) {
            if (cands[i] && !graph[fixp][i]) {
                cur_v = i;
                break;
            }
        }
    }

    return cur_v;
}

void clique_enumerate(
    IdSet &clique,
    IdSet &cands,
    IdSet &nots,
    std::vector<IdSet> &graph,
    boost::function<void(IdSet)> const &callback) {

    if (cands.none()) {
        if (nots.none()) {
            callback(clique);
        }
    }
    else {
        int fixp = greatest_cand(cands, graph);
        int cur_v = fixp;

        do {
            IdSet new_clique = clique;
            new_clique.set(cur_v);
            IdSet new_nots = graph[cur_v] & nots;
            IdSet new_cands = graph[cur_v] & cands;
            clique_enumerate(new_clique, new_cands, new_nots, graph, callback);
            nots.set(cur_v);
            cands.reset(cur_v);
            cur_v = remaining_v(cands, fixp, graph);
        } while (cur_v != NONE);
    }
}


void record_results(std::vector<IdSet> &results, IdSet clique) {
    results.push_back(clique);
}

/**
 * Find the maximal cliques of the given graph.
 * @param graph   [The graph]
 * @param results [The vector to write the maximal cliques to]
 */
void clique_enumerate(
    std::vector<IdSet> &graph,
    boost::function<void(IdSet)> const &callback) {

    #ifdef DYNAMIC_BITSET
        IdSet start_cands(graph.size());
    #else
        IdSet start_cands;
    #endif

    for (unsigned int i = 0; i < graph.size(); ++i) {
        start_cands.set(i);
    }

    #ifdef DYNAMIC_BITSET
        IdSet clique(graph.size()), nots(graph.size());
    #else
        IdSet clique, nots;
    #endif

    clique_enumerate(clique, start_cands, nots, graph, callback);
}

/**
 * Find the maximal cliques of the given graph.
 * @param graph   [The graph]
 * @param results [The vector to write the maximal cliques to]
 */
void clique_enumerate(
    std::vector<IdSet> &graph,
    std::vector<IdSet> &results) {

    clique_enumerate(graph, boost::bind(record_results, boost::ref(results), _1));
}

/**
 * State used by the iterative function.
 */
struct State {
    IdSet clique;
    IdSet cands;
    IdSet nots;

    State(IdSet cnds): clique(), cands(cnds), nots() {}
    State(IdSet clq, IdSet cnds, IdSet nts): clique(clq), cands(cnds), nots(nts) {}
};

/**
 * Find the maximal cliques of the given graph.
 * @param graph   [The graph]
 * @param results [The vector to write the maximal cliques to]
 */
void clique_enumerate_iterative(
    std::vector<IdSet> &graph,
    std::vector<IdSet> &results) {

    std::vector<State> states;
    IdSet start_cands;
    for (unsigned int i = 0; i < graph.size(); ++i) {
        start_cands.set(i);
    }
    states.push_back(State(start_cands));

    while (!states.empty()) {

        State state = states.back();
        states.pop_back();

        if (state.cands.none()) {
            if (state.nots.none()) {
                results.push_back(state.clique);
            }
        }
        else {
            int fixp = greatest_cand(state.cands, graph);
            int cur_v = fixp;

            do {
                IdSet new_clique = state.clique;
                new_clique.set(cur_v);
                IdSet new_nots = graph[cur_v] & state.nots;
                IdSet new_cands = graph[cur_v] & state.cands;
                states.push_back(State(new_clique, new_cands, new_nots));
                state.nots.set(cur_v);
                state.cands.reset(cur_v);
                cur_v = remaining_v(state.cands, fixp, graph);
            } while (cur_v != NONE);
        }
    }
}

#endif