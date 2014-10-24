
/**
 * @file	maximal_clique.cpp
 * @author  Garrett Smith <garrettwhsmith@gmail.com>
 * @version 1.0
 *
 * @section LICENSE
 *
 * Copyright [2013] [Garrett Smith, Christopher Henry / The University of Winnipeg]
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permline_streamions and
 * limitations under the License.

 *
 * @section DESCRIPTION
 *
 * Template for GraphChi applications. To create a new application, duplicate
 * this template.
 */

#define DEBUG
#define TIMING
#define OUTPUTLEVEL 4 // fatal logging only

#include <string>
#include <bitset>
#include <iostream>
#include <vector>
#include <assert.h>
#include <fstream>

#include "graphchi_basic_includes.hpp"

#include "maximal_clique_basic_includes.hpp"
#include "convert_features.hpp"

using namespace graphchi;

const static vid_t NONE = -1;

// argument set if singletons are allowed
bool singletons;

// Stores the found cliques
std::vector<IdSet> results;
std::mutex results_mutex;

// cycle breaking messages
std::vector<Message> cycle_messages;
std::mutex cycle_mutex;

/**
 * GraphChi programs need to subclass GraphChiProgram<vertex-type, edge-type>
 * class. The main logic is usually in the update function.
 */
struct MaximalClique : public GraphChiProgram<VertexDataType, EdgeDataType> {

	vid_t start_id = NONE;
	IdSet start_neighbours;
	IdSet start_cands;
	std::mutex start_mutex;

	/**
	 *  Update routing function.
	 */
	void update(Vertex &vertex, graphchi_context &gcontext) {
		if(gcontext.iteration == 0) {
			initialize(vertex);
		}
		else {
			handle_messages(vertex, gcontext.scheduler);
			if (gcontext.iteration == 1 && !start_neighbours[vertex.id()]) {
				start(vertex, gcontext.scheduler);
			}
		}
	}

	/**
	 * Initialize the edges of the given vertex.
	 * @param vertex [description]
	 */
	void initialize(Vertex &vertex) {
		bool has_neighbours = (vertex.num_edges() > 0);
		// check if this vertex has neighbours
		if (has_neighbours) {
			IdSet neighbours = get_neighbours(vertex);
			VertexData data = vertex.get_data();
			data.set_neighbours(neighbours);
			vertex.set_data(data);

			// write neighbours to inedges
			for (int i = 0; i < vertex.num_inedges(); i++) {
				graphchi_edge<EdgeData>* edge = vertex.inedge(i);
				EdgeDataType data = edge->get_data();
				data._neighbours = neighbours;
				edge->set_data(data);
			}

			// check if this is the vertex with the most neighbours
			start_mutex.lock();
			if ( start_id == NONE || neighbours.count() > start_neighbours.count()) {
				start_id = vertex.id();
				start_neighbours = neighbours;
			}
			start_mutex.unlock();

		}
		else {
			// Mark not to start from this vertex if it is its only neighbour
			start_mutex.lock();
			start_cands.reset(vertex.id());
			start_mutex.unlock();
			// record if singletons are allowed
			if (singletons) {
				IdSet clique;
				clique.set(vertex.id());
				save_clique(clique);
			}
		}
	}

	/**
	 * Get all the neighbours of the given vertex.
	 * @param  vertex [description]
	 * @return        [description]
	 */
	IdSet get_neighbours(Vertex &vertex) {
		// first check if we have already found the neighbours
		VertexData data = vertex.get_data();
		if (data._neighbours_set) {
			return data._neighbours;
		}
		else {
			// find all the neighbours of this vertex
			IdSet neighbours;
			for (int i = 0; i < vertex.num_outedges(); i++) {
				graphchi_edge<EdgeData>* edge = vertex.outedge(i);
				neighbours.set(edge->vertex_id());
			}
			return neighbours;
		}
	}

	/**
	 * Starts finding a clique from the given vertex.
	 * @param vertex    [description]
	 * @param scheduler [description]
	 */
	void start(Vertex &vertex, ischeduler * scheduler) {
		d_var(vertex.id());

		IdSet empty_clique;
		IdSet neighbours = get_neighbours(vertex);
		IdSet candidates;
		IdSet not_set;
		IdSet block_trace;

		// First vertex starts with all neighbours as candidates
		if (vertex.id() == start_id) {
			candidates = neighbours;
		}
		// Subsequent start with neighbours - previous started vertices
		else {
			start_mutex.lock();
			candidates = neighbours & start_cands;
			not_set = ~start_cands & neighbours;
			start_cands.reset(vertex.id());
			start_mutex.unlock();
			// not_set.reset(vertex.id());
		}
		d_clique_var(not_set);
		clique_enumerate(vertex, empty_clique, candidates, not_set, block_trace, scheduler);
	}

	/**
	 * Uses messages to continue finding clique.
	 * @param  vertex    [description]
	 * @param  scheduler [description]
	 * @return           [description]
	 */
	bool handle_messages(Vertex &vertex, ischeduler * scheduler) {
		bool found = false;
		bool collision = false;
		// read the message from each in edge
		for (int i = 0; i < vertex.num_inedges(); i++) {
			graphchi_edge<EdgeData>* edge = vertex.inedge(i);
			EdgeDataType data = edge->get_data();
			Message * msg = &data._message;
			// If the message is valid
			if (msg->_set) {
				bool success = clique_enumerate(vertex, msg->_current_clique,
					msg->_candidates, msg->_not, data._block_trace, scheduler);
				// only unset the message if we were able to forward it
				if (success) {
					data._message.unset();
					edge->set_data(data);
				}
				else {
					collision = true;
				}
				found = true;
			}
		}
		// scheduling
		if (collision) {
			// reschedule on failure
			scheduler->add_task(vertex.id());
		}
		else {
			// prevent over scheduling
			scheduler->remove_tasks(vertex.id(), vertex.id());
		}
		return found;
	}

	/**
	 * Main clique enumerate function.
	 * @param  vertex      [description]
	 * @param  clique      [description]
	 * @param  cand        [description]
	 * @param  not_set     [description]
	 * @param  block_trace [description]
	 * @param  scheduler   [description]
	 * @return             [description]
	 */
	bool clique_enumerate(
		Vertex &vertex,
		IdSet &clique,
		IdSet &cand,
		IdSet &not_set,
		IdSet &block_trace,
		ischeduler * scheduler) {

		d_var(vertex.id());

		// mark this vertex in the clique
		clique.set(vertex.id());

		bool handled = true;

		// if there are no more candidate verexs this is the final clique
		if (cand.none()) {
			if (not_set.none()) {
				save_clique(clique);
			}
			else {
				d("Rejected");
				d_clique_var(not_set);
				d_clique_var(clique);
			}
		}
		else {
			IdSet orig_cand = cand;

			// message
			IdSet destinations;

			// fixp
			vid_t fixp = NONE;
			size_t fixp_num_candidates = 0;
			EdgeData fixp_data;
			graphchi_edge<EdgeData> * fixp_edge;
			IdSet fixp_neighbours;

			// cur_v
			vid_t cur_v;
			EdgeData cur_v_data;
			graphchi_edge<EdgeData> * cur_v_edge;
			IdSet cur_v_neighbours;

			// find first destination
			for (int i = 0; i < vertex.num_outedges(); ++i) {
				graphchi_edge<EdgeData> * outedge = vertex.outedge(i);

				// check if this is to a candidate vertex
				if (cand[outedge->vertexid]) {
					EdgeData data = outedge->get_data();
					IdSet neighbours = data._neighbours;
					IdSet intersection = cand & neighbours;

					// either this is the first or check if it has the most
					// neighbours so far
					size_t num_candidates = intersection.count();
					if (fixp == NONE || num_candidates >= fixp_num_candidates) {
						fixp = outedge->vertexid;
						fixp_num_candidates = num_candidates;
						fixp_data = data;
						fixp_edge = outedge;
						fixp_neighbours = neighbours;
					}
				}
			}

			cur_v = fixp;
			cur_v_data = fixp_data;
			cur_v_edge = fixp_edge;
			cur_v_neighbours = fixp_neighbours;

			int i = 0;
			while (cur_v != NONE) {

				// check for collisions
				if (cur_v_data._message._set) {
					d("Collision " << vertex.id() << " -> " << cur_v);

					// check if this results in a cycle
					if (block_trace[vertex.id()]) {
						d("Cycle");
						d_clique_var(block_trace);

						// save message for later
						Message msg;
						msg.set(clique, cand, not_set);
						cycle_mutex.lock();
						cycle_messages.push_back(msg);
						cycle_mutex.unlock();

						// remove message


						// record that we have a message saved
						VertexDataType data = vertex.get_data();
						data._cycles++;
						vertex.set_data(data);
					}
					else {
						// forward block set
						cur_v_data._block_trace = block_trace;
						cur_v_data._block_trace.set(vertex.id());
					}

					handled = false;

					cur_v_edge->set_data(cur_v_data);
					break;
				}
				else {
					// clear block set
					cur_v_data._block_trace.reset();

					// Save this message
					destinations.set(cur_v);
				}

				cand.reset(cur_v);

				// check for remaining destinations
				cur_v = NONE;
				while (i < vertex.num_outedges()) {
					graphchi_edge<EdgeData> * outedge = vertex.outedge(i);

					// check if this is to a candidate vertex that is not a
					// neighbour of fixp
					vid_t id = outedge->vertexid;
					if (cand[id] && !fixp_neighbours[id]) {
						EdgeData data = outedge->get_data();
						cur_v = id;
						cur_v_data = data;
						cur_v_edge = outedge;
						cur_v_neighbours = data._neighbours;
						break;
					}
					++i;
				}

			}

			// Send messages
			if (handled) {
				cand = orig_cand;
				for (int i = 0; i < vertex.num_outedges(); ++i) {
					graphchi_edge<EdgeData> * outedge = vertex.outedge(i);

					// check if the edge is a destination
					vid_t id = outedge->vertexid;
					if (destinations[id]) {
						EdgeData data = outedge->get_data();

						d(vertex.id() << " -> "<< id);

						cur_v_neighbours = data._neighbours;
						IdSet new_not = not_set & cur_v_neighbours;
						IdSet new_cand = cand & cur_v_neighbours;

						// schedule
						scheduler->add_task(id);

						not_set.set(id);
						cand.reset(id);

						// set message
						data._message.set(clique, new_cand, new_not);
						outedge->set_data(data);
					}
				}
			}
		}
		return handled;
	}

	/**
	 * Saves a clique to the results vector.
	 * @param clique [description]
	 */
	void save_clique(const IdSet &clique) {
		d_clique_var(clique);
		results_mutex.lock();
		results.push_back(clique);
		results_mutex.unlock();
	}

	/**
	 * Called before an iteration starts.
	 */
	void before_iteration(int iteration, graphchi_context &gcontext) {
		d_var(iteration);

		if (iteration == 0) {
			// ensure there are not more vertices than the max size
			assert(gcontext.nvertices < MAX_VERTICES);

			// set all bits in start_cands
			// start_cands = ~start_cands;
			for (size_t i = 0; i < gcontext.nvertices; i++) {
				start_cands.set(i);
			}
		}
	}

	/**
	 * Called after an iteration has finished.
	 */
	void after_iteration(int iteration, graphchi_context &gcontext) {
		// if a start point was found
		if (iteration == 0 && start_id != NONE) {
			// schedule this vertex
			gcontext.scheduler->add_task(start_id);

			// remove first vertex from candidates
			start_cands.reset(start_id);

			// schedule all non-neighbours
			for (size_t i = 0; i < gcontext.nvertices; i++) {
				// if this vertex is not a neighbour schedule it
				if (!start_neighbours[i] && start_cands[i]) {
					gcontext.scheduler->add_task(i);
				}
			}
		}
	}

	/**
	 * Called before an execution interval is started.
	 */
	void before_exec_interval(vid_t window_st, vid_t window_en,
		graphchi_context &gcontext) {
	}

	/**
	 * Called after an execution interval has finished.
	 */
	void after_exec_interval(vid_t window_st, vid_t window_en,
		graphchi_context &gcontext) {
	}

};

int main(int argc, const char ** argv) {
	// timing
	create_timing(total);
	start_timing(total);
	create_timing(convert);
	create_timing(algorithm);
	create_timing(preprocess);

	/* GraphChi initialization will read the command line
       arguments and the configuration file. */
	graphchi_init(argc, argv);

	/* Metrics object for keeping track of performance counters
       and other information. Currently required. */
	metrics m("maximal-clique");

	/* Basic arguments for application */
	std::string orig_filename = get_option_string("file", "");  // Base filename
	assert (orig_filename!=""); // ensure a file was given
	std::string filename;


	int niters = get_option_int("niters", 100); // Number of iterations (max)

	bool scheduler = true;// Use selective scheduling
	bool sort_output = get_option_int("sort", 1); // whether to sort output
	singletons = get_option_int("singletons", 0); //  whether we are interested in singletons

	float epsilon = get_option_float("epsilon", 0);
	int features = get_option_int("features", 0);

	// ensure an epsilon and number of features was given to convert
	assert(epsilon > 0);
	assert(features > 0);


	start_timing(convert);
	std::string converted = convert_features(orig_filename, epsilon, features);
	stop_timing(convert);

	/* Detect the number of shards or preprocess an input to create them */
	// TODO only works with adjlist currently
	start_timing(preprocess);
	int nshards = convert_if_notexists<EdgeDataType>(converted,
			get_option_string("nshards", "auto"));
	stop_timing(preprocess);

	/* Run */
	MaximalClique program;
	graphchi_engine<VertexDataType, EdgeDataType> engine(converted,
		nshards, scheduler, m);
	start_timing(algorithm);
	engine.run(program, niters);
	stop_timing(algorithm);

	/* Report execution metrics */
	// metrics_report(m);

	/* record Results */
	std::string output = get_option_string("output", converted + "_output");
	d_var(output);
	if (sort_output) std::sort(results.begin(), results.end(), clique_compare);

	std::ofstream out_file(output, std::ofstream::trunc);
	for (std::vector<IdSet>::iterator it = results.begin(); it != results.end(); ++it) {
		out_file << clique_to_string(*it) << std::endl;
	}
	out_file.close();

	// timing
	stop_timing(total);
	report_timing(convert);
	report_timing(preprocess);
	report_timing(algorithm);
	report_timing(total);
	return 0;
}
