#include "PricerSolverBddForward.hpp"

/**
 *  bdd solver pricersolver for the flow formulation
 */
PricerSolverBddSimple::PricerSolverBddSimple(GPtrArray* _jobs,
                                             int        _num_machines,
                                             GPtrArray* _ordered_jobs)
    : PricerSolverBdd(_jobs, _num_machines, _ordered_jobs) {
    std::cout << "Constructing BDD with Forward Simple evaluator" << '\n';
    std::cout << "size BDD = " << get_size_graph() << '\n';
    evaluator = ForwardBddSimpleDouble(njobs);
    reversed_evaluator = BackwardBddSimpleDouble(njobs);
}

OptimalSolution<double> PricerSolverBddSimple::pricing_algorithm(double* _pi) {
    evaluator.initializepi(_pi);
    return decision_diagram->evaluate_forward(evaluator);
}

void PricerSolverBddSimple::compute_labels(double* _pi) {
    evaluator.initializepi(_pi);
    reversed_evaluator.initializepi(_pi);

    decision_diagram->compute_labels_forward(evaluator);
    decision_diagram->compute_labels_backward(reversed_evaluator);
}

void PricerSolverBddSimple::evaluate_nodes(double* pi, int UB, double LB) {
    NodeTableEntity<>& table = decision_diagram->getDiagram().privateEntity();
    compute_labels(pi);
    double reduced_cost = table.node(1).forward_label[0].get_f();
    nb_removed_edges = 0;

    /** check for each node the Lagrangian dual */
    for (int i = decision_diagram->topLevel(); i > 0; i--) {
        for (auto& it : table[i]) {
            int    w = it.get_weight();
            Job*   job = it.get_job();
            double result = it.forward_label[0].get_f() +
                            it.child[1]->backward_label[0].get_f() -
                            value_Fj(w + job->processing_time, job) +
                            pi[job->job] + pi[njobs];

            if (LB - (double)(num_machines - 1) * reduced_cost - result >
                    UB - 1 + 0.0001 &&
                (it.calc_yes)) {
                it.calc_yes = false;
                nb_removed_edges++;
            }
        }
    }

    printf("removed edges = %d\n", nb_removed_edges);
}

/**
 * bdd solver pricersolver for the flow formulation that takes care of the
 * consecutive jobs
 */
PricerSolverBddCycle::PricerSolverBddCycle(GPtrArray* _jobs, int _num_machines,
                                           GPtrArray* _ordered_jobs)
    : PricerSolverBdd(_jobs, _num_machines, _ordered_jobs) {
    std::cout << "Constructing BDD with Forward Cycle evaluator" << '\n';
    std::cout << "size BDD = " << get_size_graph() << '\n';
    evaluator = ForwardBddCycleDouble(njobs);
    reversed_evaluator = BackwardBddCycleDouble(njobs);
}

OptimalSolution<double> PricerSolverBddCycle::pricing_algorithm(double* _pi) {
    evaluator.initializepi(_pi);
    return decision_diagram->evaluate_forward(evaluator);
}

void PricerSolverBddCycle::compute_labels(double* _pi) {
    evaluator.initializepi(_pi);
    reversed_evaluator.initializepi(_pi);

    decision_diagram->compute_labels_forward(evaluator);
    decision_diagram->compute_labels_backward(reversed_evaluator);
}

void PricerSolverBddCycle::evaluate_nodes(double* pi, int UB, double LB) {
    NodeTableEntity<>& table = decision_diagram->getDiagram().privateEntity();
    compute_labels(pi);
    double reduced_cost = table.node(1).forward_label[0].get_f();
    nb_removed_edges = 0;

    /** check for each node the Lagrangian dual */
    for (int i = decision_diagram->topLevel(); i > 0; i--) {
        for (auto& it : table[i]) {
            int  w = it.get_weight();
            Job* job = it.get_job();

            if (it.forward_label[0].get_previous_job() != job &&
                it.child[1]->backward_label[0].get_prev_job() != job) {
                double result = it.forward_label[0].get_f() +
                                it.child[1]->backward_label[0].get_f() -
                                value_Fj(w + job->processing_time, job) +
                                pi[job->job] + pi[njobs];
                if (LB - (double)(num_machines - 1) * reduced_cost - result >
                        UB - 1 + 0.0001 &&
                    (it.calc_yes)) {
                    it.calc_yes = false;
                    nb_removed_edges++;
                }
            } else if (it.forward_label[0].get_previous_job() == job &&
                       it.child[1]->backward_label[0].get_prev_job() != job) {
                double result = it.forward_label[1].get_f() +
                                it.child[1]->backward_label[0].get_f() -
                                value_Fj(w + job->processing_time, job) +
                                pi[job->job] + pi[njobs];
                if (LB - (double)(num_machines - 1) * reduced_cost - result >
                        UB - 1 + 0.0001 &&
                    (it.calc_yes)) {
                    it.calc_yes = false;
                    nb_removed_edges++;
                }
            } else if (it.forward_label[0].get_previous_job() != job &&
                       it.child[1]->backward_label[0].get_prev_job() == job) {
                double result = it.forward_label[0].get_f() +
                                it.child[1]->backward_label[1].get_f() -
                                value_Fj(w + job->processing_time, job) +
                                pi[job->job] + pi[njobs];
                if (LB - (double)(num_machines - 1) * reduced_cost - result >
                        UB - 1 + 0.0001 &&
                    (it.calc_yes)) {
                    it.calc_yes = false;
                    nb_removed_edges++;
                }
            } else {
                double result = it.forward_label[1].get_f() +
                                it.child[1]->backward_label[1].get_f() -
                                value_Fj(w + job->processing_time, job) +
                                pi[job->job] + pi[njobs];
                if (LB - (double)(num_machines - 1) * reduced_cost - result >
                        UB - 1 + 0.0001 &&
                    (it.calc_yes)) {
                    it.calc_yes = false;
                    nb_removed_edges++;
                }
            }
        }
    }

    printf("removed edges = %d\n", nb_removed_edges);
}