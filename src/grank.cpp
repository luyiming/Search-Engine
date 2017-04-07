/**
 * @file
 * @author  Lu Yiming <luyimingchn@gmail.com>
 * @version 1.0
 * @date 2017-1-4

 * @section DESCRIPTION
 *
 * Pagerank algorithm
 * Using Graphchi-cpp library
 */
#include <string>
#include <fstream>
#include <cmath>

#define GRAPHCHI_DISABLE_COMPRESSION

#include "graphchi_basic_includes.hpp"
#include "util/toplist.hpp"
#include "api/vertex_aggregator.hpp"

using namespace graphchi;
using namespace std;

#define THRESHOLD 1e-1
#define RANDOMRESETPROB 0.15

using VertexDataType = float;
using EdgeDataType = float;

struct PagerankProgram : public GraphChiProgram<VertexDataType, EdgeDataType> {
    void update(graphchi_vertex<VertexDataType, EdgeDataType> &v, graphchi_context &ginfo) {
        float sum=0;
        if (ginfo.iteration == 0) {
            /* initialize vertex and out-edges. */
            for (int i=0; i < v.num_outedges(); i++) {
                graphchi_edge<float> * edge = v.outedge(i);
                edge->set_data(1.0 / v.num_outedges());
            }
            v.set_data(RANDOMRESETPROB);
        }
        else {
            /* Compute the sum of neighbors' weighted pageranks by
               reading from the in-edges. */
            for (int i=0; i < v.num_inedges(); i++) {
                float val = v.inedge(i)->get_data();
                sum += val;
            }
            /* Compute pagerank */
            float pagerank = RANDOMRESETPROB + (1 - RANDOMRESETPROB) * sum;
            /* Write pagerank divided by the number of out-edges to
               each of out-edges. */
            if (v.num_outedges() > 0) {
                float pagerankcont = pagerank / v.num_outedges();
                for(int i=0; i < v.num_outedges(); i++) {
                    graphchi_edge<float> * edge = v.outedge(i);
                    edge->set_data(pagerankcont);
                }
            }
            ginfo.log_change(abs(pagerank - v.get_data()));
            /* Set new pagerank as the vertex value */
            v.set_data(pagerank);
        }
    }
};


struct PagerankProgramInmem : public GraphChiProgram<VertexDataType, EdgeDataType> {
    vector<EdgeDataType> pr;
    PagerankProgramInmem(int nvertices) :   pr(nvertices, RANDOMRESETPROB) {}
    void update(graphchi_vertex<VertexDataType, EdgeDataType> &v, graphchi_context &ginfo) {
        if (ginfo.iteration > 0) {
            float sum=0;
            for (int i=0; i < v.num_inedges(); i++) {
              sum += pr[v.inedge(i)->vertexid];
            }
            if (v.outc > 0) {
                pr[v.id()] = (RANDOMRESETPROB + (1 - RANDOMRESETPROB) * sum) / v.outc;
            }
            else {
                pr[v.id()] = (RANDOMRESETPROB + (1 - RANDOMRESETPROB) * sum);
            }
        }
        else if (ginfo.iteration == 0) {
            if (v.outc > 0) pr[v.id()] = 1.0f / v.outc;
        }
        if (ginfo.iteration == ginfo.num_iterations - 1) {
            /* On last iteration, multiply pr by degree and store the result */
            v.set_data(v.outc > 0 ? pr[v.id()] * v.outc : pr[v.id()]);
        }
    }
};

/**
 * TODO: for in-degree computation
 */
template <typename VertexDataType>
class Callback: public VCallback<VertexDataType> {
    void callback(vid_t vertex_id, VertexDataType &value) {
        cout << "vertex_id: " << vertex_id << "value: " << value << endl;
    }
};

int main(int argc, const char ** argv) {
    graphchi_init(argc, argv);
    metrics m("pagerank");
    global_logger().set_log_level(LOG_DEBUG);

    string filename = get_option_string("file");   // input vertex file
    int niters      = get_option_int("niters", 4); // iteration time
    bool scheduler  = false;
    int ntop        = get_option_int("top", 20);   // output vertices number
    int nshards     = convert_if_notexists<EdgeDataType>(filename, get_option_string("nshards", "auto"));

    graphchi_engine<float, float> engine(filename, nshards, scheduler, m);
    engine.set_modifies_inedges(false);
    bool inmemmode = engine.num_vertices() * sizeof(EdgeDataType) < (size_t)engine.get_membudget_mb() * 1024L * 1024L;
    if (inmemmode) {
        logstream(LOG_INFO) << "Running Pagerank by holding vertices in-memory mode!" << endl;
        engine.set_modifies_outedges(false);
        engine.set_disable_outedges(true);
        engine.set_only_adjacency(true);
        PagerankProgramInmem program(engine.num_vertices());
        engine.run(program, niters);
    }
    else {
        PagerankProgram program;
        engine.run(program, niters);
    }

    if (ntop == 0) {
        ntop = engine.num_vertices();
    }
    // ntop = 10;

    vector<vertex_value<float> > top = get_top_vertices<float>(filename, ntop);
    cout << "Print top " << ntop << " vertices:" << endl;
    for(int i=0; i < (int)top.size(); i++) {
        cout << (i+1) << ". " << top[i].vertex << "\t" << top[i].value << endl;
    }
    metrics_report(m);

/* TODO:
    Callback<VertexDataType> callback;
    foreach_vertices<VertexDataType>(filename, 0, 5, callback);
*/
    return 0;
}
