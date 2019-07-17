#include "PricerSolverZddForward.hpp"

/**
 *  bdd solver pricersolver for the flow formulation
 */
PricerSolverSimple::PricerSolverSimple(GPtrArray *_jobs, int _num_machines, GPtrArray *_ordered_jobs) :
    PricerSolverZdd(_jobs, _num_machines, _ordered_jobs) {
    std::cout << "Constructing ZDD with Forward Simple evaluator" << '\n';
    std::cout << "size BDD = " << get_size_graph() << '\n';
    evaluator = ForwardZddSimpleDouble(njobs);
    reversed_evaluator = BackwardZddSimpleDouble(njobs);
}

OptimalSolution<double> PricerSolverSimple::pricing_algorithm(double *_pi) {
    evaluator.initializepi(_pi);
    return decision_diagram->evaluate_forward(evaluator);
}

void PricerSolverSimple::compute_labels(double *_pi) {
    evaluator.initializepi(_pi);
    reversed_evaluator.initializepi(_pi);

    decision_diagram->compute_labels_forward(evaluator);
    decision_diagram->compute_labels_backward(reversed_evaluator);
}

void PricerSolverSimple::evaluate_nodes(double *pi, int UB, double LB) {
    NodeTableEntity<NodeZdd<>>& table = decision_diagram->getDiagram().privateEntity();
    compute_labels(pi);
    double reduced_cost = table.node(decision_diagram->root()).list[0]->backward_label[0].get_f();

    nb_removed_edges = 0;

    // /** check for each node the Lagrangian dual */
    for (int i = decision_diagram->topLevel(); i > 0; i--) {
        for (auto &it : table[i]) {
            for(auto &iter : it.list) {
                int w = iter->get_weight();
                Job *job = it.get_job();
                double result = iter->forward_label[0].get_f() + iter->y->backward_label[0].get_f() - value_Fj(w + job->processing_time, job) + pi[job->job] + pi[njobs];

                if (LB - (double)(num_machines - 1)*reduced_cost - result > UB - 1 + 0.0001 && (iter->calc_yes)) {
                    iter->calc_yes = false;
                    nb_removed_edges++;
                }
            }
        }
    }

    printf("removed edges = %d\n", nb_removed_edges);
}

PricerSolverZddCycle::PricerSolverZddCycle(GPtrArray *_jobs, int _num_machines, GPtrArray *_ordered_jobs) : 
    PricerSolverZdd(_jobs, _num_machines, _ordered_jobs) {
    std::cout << "Constructing ZDD with Forward ZddCycle evaluator" << '\n';
    std::cout << "size BDD = " << get_size_graph() << '\n';
    evaluator = ForwardZddCycleDouble(njobs);
    reversed_evaluator = ForwardZddCycleDouble(njobs);
}

OptimalSolution<double> PricerSolverZddCycle::pricing_algorithm(double *_pi) {
    evaluator.initializepi(_pi);
    return decision_diagram->evaluate_forward(evaluator);
}

void PricerSolverZddCycle::compute_labels(double *_pi) {
    evaluator.initializepi(_pi);
    reversed_evaluator.initializepi(_pi);

    decision_diagram->compute_labels_forward(evaluator);
    decision_diagram->compute_labels_backward(reversed_evaluator);
}

void PricerSolverZddCycle::evaluate_nodes(double *pi, int UB, double LB) {
    // NodeTableEntity<>& table = decision_diagram->getDiagram().privateEntity();
    // compute_labels(pi);
    // double reduced_cost = table.node(1).forward_label[0].get_f();
    // nb_removed_edges = 0;

    // /** check for each node the Lagrangian dual */
    // for (int i = decision_diagram->topLevel(); i > 0; i--) {
    //     for (auto &it : table[i]) {
    //         int w = it.get_weight();
    //         Job *job = it.get_job();
    //         double result = it.forward_label[0].get_f() + it.child[1]->backward_label[0].get_f() - value_Fj(w + job->processing_time, job) + pi[job->job] + pi[njobs];

    //         if (LB - (double)(num_machines - 1)*reduced_cost - result > UB - 1 + 0.0001 && (it.calc_yes)) {
    //             it.calc_yes = false;
    //             nb_removed_edges++;
    //         }
    //     }
    // }

    // printf("removed edges = %d\n", nb_removed_edges);
}