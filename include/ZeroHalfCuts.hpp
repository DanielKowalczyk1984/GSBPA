#ifndef __ZEROHALFCUTS_H__
#define __ZEROHALFCUTS_H__

#include <memory>
#include <vector>
#include "ModelInterface.hpp"
#include "NodeBddTable.hpp"
#include "NodeId.hpp"
#include "gurobi_c++.h"

class ZeroHalfCuts {
   public:
    ZeroHalfCuts(int _nb_jobs, int _nb_machines, ReformulationModel* _rmp_model,
                 NodeId& _root, NodeTableEntity<>* _table);
    ZeroHalfCuts(ZeroHalfCuts&&) = default;
    ZeroHalfCuts(const ZeroHalfCuts&) = delete;
    ZeroHalfCuts& operator=(ZeroHalfCuts&&) = default;
    ZeroHalfCuts& operator=(const ZeroHalfCuts&) = delete;

    ~ZeroHalfCuts() = default;
    bool add_cuts();
    void generate_cuts();

   private:
    std::unique_ptr<GRBEnv>   env;
    std::unique_ptr<GRBModel> model;
    int                       nb_jobs;
    int                       nb_machines;
    ReformulationModel*       rmp_model;
    NodeId&                   root;
    NodeTableEntity<>*        table;
    std::vector<GRBVar>       sigma{};
    std::vector<NodeId>       node_ids{};
    std::vector<GRBVar>       jobs_var;
    int                       terminal_key;

    void generate_model();
    void dfs(const NodeId& v);
};

#endif  // __ZEROHALFCUTS_H__
