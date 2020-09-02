#include "PricerSolverBase.hpp"
#include <limits>

/**
 * PricerSolverBase default COnstructor
 **/
PricerSolverBase::PricerSolverBase(GPtrArray*  _jobs,
                                   int         _num_machines,
                                   const char* p_name,
                                   double      _UB)
    : jobs(_jobs),
      convex_constr_id(_jobs->len),
      convex_rhs(_num_machines),
      problem_name(p_name),
      env(new GRBEnv()),
      model(new GRBModel(*env)),
      reformulation_model(jobs->len, _num_machines),
      is_integer_solution(false),
      UB(_UB) {
    model->set(GRB_IntParam_Method, GRB_METHOD_AUTO);
    model->set(GRB_IntParam_Threads, 1);
    model->set(GRB_IntAttr_ModelSense, GRB_MINIMIZE);
    model->set(GRB_IntParam_Presolve, 2);
    model->set(GRB_DoubleParam_MIPGap, 1e-6);
    model->set(GRB_DoubleParam_TimeLimit, 1800);
}

PricerSolverBase::~PricerSolverBase() {}

void PricerSolverBase::print_num_paths() {}

double PricerSolverBase::get_UB() {
    return UB;
}

void PricerSolverBase::update_UB(double _UB) {
    if (_UB < UB) {
        UB = _UB;
    }
}

int PricerSolverBase::get_int_attr_model(enum MIP_Attr c) {
    int val = -1;
    switch (c) {
        case MIP_Attr_Nb_Vars:
            val = model->get(GRB_IntAttr_NumVars);
            break;
        case MIP_Attr_Nb_Constr:
            val = model->get(GRB_IntAttr_NumConstrs);
            break;
        case MIP_Attr_Status:
            val = model->get(GRB_IntAttr_Status);
            break;
        default:
            break;
    }

    return val;
}

double PricerSolverBase::get_dbl_attr_model(enum MIP_Attr c) {
    double val = -1.0;
    int    status = model->get(GRB_IntAttr_Status);
    if (status != GRB_INF_OR_UNBD && status != GRB_INFEASIBLE &&
        status != GRB_UNBOUNDED) {
        switch (c) {
            case MIP_Attr_Obj_Bound:
                val = model->get(GRB_DoubleAttr_ObjBound);
                break;
            case MIP_Attr_Obj_Bound_LP:
                val = model->get(GRB_DoubleAttr_ObjBoundC);
                break;
            case MIP_Attr_Mip_Gap:
                val = model->get(GRB_DoubleAttr_MIPGap);
                break;
            case MIP_Attr_Run_Time:
                val = model->get(GRB_DoubleAttr_Runtime);
                break;
            case MIP_Attr_Nb_Simplex_Iter:
                val = model->get(GRB_DoubleAttr_IterCount);
                break;
            case MIP_Attr_Nb_Nodes:
                val = model->get(GRB_DoubleAttr_NodeCount);
                break;
            default:
                val = std::numeric_limits<double>::max();
                break;
        }
    } else {
        switch (c) {
            case MIP_Attr_Run_Time:
                val = model->get(GRB_DoubleAttr_Runtime);
                break;
            case MIP_Attr_Nb_Simplex_Iter:
                val = model->get(GRB_DoubleAttr_IterCount);
                break;
            case MIP_Attr_Nb_Nodes:
                val = model->get(GRB_DoubleAttr_NodeCount);
                break;
            default:
                val = std::numeric_limits<double>::max();
                break;
        }
    }
    return val;
}

double PricerSolverBase::compute_reduced_cost(const OptimalSolution<>& sol,
                                              double*                  pi,
                                              double*                  lhs) {
    double result = sol.cost;
    std::fill(lhs, lhs + reformulation_model.get_nb_constraints(), 0.0);

    for (guint j = 0; j < sol.jobs->len; j++) {
        Job*            tmp_j = (Job*)g_ptr_array_index(sol.jobs, j);
        VariableKeyBase k(tmp_j->job, 0);
        for (int c = 0; c < reformulation_model.get_nb_constraints(); c++) {
            if (c == convex_constr_id) {
                continue;
            }
            double          dual = pi[c];
            ConstraintBase* constr = reformulation_model.get_constraint(c);
            double          coeff = constr->get_var_coeff(&k);

            if (fabs(coeff) > 1e-10) {
                result -= coeff * dual;
                lhs[c] += coeff;
            }
        }
    }

    double          dual = pi[convex_constr_id];
    ConstraintBase* constr =
        reformulation_model.get_constraint(convex_constr_id);
    VariableKeyBase k(0, 0, true);
    double          coeff = constr->get_var_coeff(&k);
    result -= coeff * dual;
    lhs[convex_constr_id] += coeff;

    return result;
}

double PricerSolverBase::compute_lagrange(const OptimalSolution<>& sol,
                                          double*                  pi) {
    double result = sol.cost;
    double dual_bound = 0.0;

    for (guint j = 0; j < sol.jobs->len; j++) {
        Job*            tmp_j = (Job*)g_ptr_array_index(sol.jobs, j);
        VariableKeyBase k(tmp_j->job, 0);
        double          dual = pi[tmp_j->job];
        ConstraintBase* constr = reformulation_model.get_constraint(tmp_j->job);
        double          coeff = constr->get_var_coeff(&k);

        if (fabs(coeff) > 1e-10) {
            result -= coeff * dual;
        }

        for (int c = convex_constr_id + 1;
             c < reformulation_model.get_nb_constraints(); c++) {
            double          dual_ = pi[c];
            ConstraintBase* constr_ = reformulation_model.get_constraint(c);
            double          coeff_ = constr->get_var_coeff(&k);

            if (fabs(coeff_) > 1e-10) {
                result -= coeff_ * dual_;
            }
        }
    }

    result = CC_MIN(0, result);

    for (int c = 0; c < reformulation_model.get_nb_constraints(); c++) {
        if (c == convex_constr_id) {
            continue;
        }
        double          dual = pi[c];
        ConstraintBase* constr = reformulation_model.get_constraint(c);
        double          rhs = constr->get_rhs();

        dual_bound += rhs * dual;
    }

    result = -reformulation_model.get_constraint(convex_constr_id)->get_rhs() *
             result;
    result = dual_bound + result;

    return result;
}

double PricerSolverBase::compute_subgradient(const OptimalSolution<>& sol,
                                             double* subgradient) {
    auto nb_constraints = reformulation_model.get_nb_constraints();
    auto convex_rhs =
        -reformulation_model.get_constraint(convex_constr_id)->get_rhs();

    for (size_t i = 0; i < nb_constraints; i++) {
        auto constr = reformulation_model.get_constraint(i);
        subgradient[i] = constr->get_rhs();
    }

    for (guint j = 0; j < sol.jobs->len; j++) {
        Job*            tmp_j = (Job*)g_ptr_array_index(sol.jobs, j);
        VariableKeyBase k(tmp_j->job, 0);
        ConstraintBase* constr = reformulation_model.get_constraint(tmp_j->job);
        double          coeff = constr->get_var_coeff(&k);

        if (fabs(coeff) > 1e-10) {
            subgradient[k.get_j()] -= coeff * convex_rhs;
        }

        for (int c = convex_constr_id + 1;
             c < reformulation_model.get_nb_constraints(); c++) {
            ConstraintBase* constr_ = reformulation_model.get_constraint(c);
            double          coeff_ = constr->get_var_coeff(&k);

            if (fabs(coeff_) > 1e-10) {
                subgradient[c] -= coeff_ * convex_rhs;
            }
        }
    }

    subgradient[convex_constr_id] += convex_rhs;

    return 0.0;
}

void PricerSolverBase::calculate_constLB(double* pi) {
    constLB = 0.0;

    for (int i = 0; i < reformulation_model.get_nb_constraints(); i++) {
        if (i == convex_constr_id) {
            continue;
        }
        auto constr = reformulation_model.get_constraint(i);
        constLB += constr->get_rhs() * pi[i];
    }
}

extern "C" {
double call_get_UB(PricerSolverBase* solver) {
    return solver->get_UB();
}

void call_update_UB(PricerSolverBase* solver, double _UB) {
    solver->update_UB(_UB);
}

void call_evaluate_nodes(PricerSolverBase* solver, double* pi) {
    solver->calculate_constLB(pi);
    solver->evaluate_nodes(pi);
}
}
