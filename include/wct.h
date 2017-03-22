#ifndef _WCT_H
#define _WCT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "defs.h"
#include "wctprivate.h"

/**
 * print.c
 */

int print_to_screen(wctproblem *problem);
int print_to_csv(wctproblem *problem);

/**
 * greedy.c
 */

void update_bestschedule(wctproblem *problem, solution *sol);

int construct_wspt(Job *jobarray, int njobs, int nmachines, solution *sol);
int construct_feasible_solutions(wctproblem *problem);
int construct_edd(wctproblem *prob, solution *sol);
int construct_spt(wctproblem *prob, solution *sol);
int construct_random(wctproblem *prob, solution *sol, GRand *rand_uniform);

int heuristic_rpup(wctproblem *prob);
int partlist_to_Scheduleset(
    partlist *part, int nbpart, int njobs, Scheduleset **classes, int *ccount);

/*
compare_functions
 */
gint compare_func1(gconstpointer a, gconstpointer b, void *user_data);

/**
 * compare_functions
 */

int compare_cw(BinomialHeapValue a, BinomialHeapValue b);

/**
 * wct.c
 */

int compute_schedule(wctproblem *problem);

/*Computation functions*/
int compute_lower_bound(wctproblem *problem, wctdata *pd);
int sequential_branching(wctproblem *problem);
int create_branches(wctdata *pd, wctproblem *problem);
int check_integrality(wctdata *pd, int nmachine, int *result);
int build_lp(wctdata *pd, int construct);
int heur_colors_with_stable_sets(wctdata *pd);
int compute_objective(wctdata *pd, wctparms *parms);
void make_pi_feasible(wctdata *pd);
void make_pi_feasible_farkas_pricing(wctdata *pd);
/** Wide Branching functions */
int create_branches_wide(wctdata *pd, wctproblem *problem);
int sequential_branching_wide(wctproblem *problem);
/** Conflict Branching functions */
int create_branches_conflict(wctdata *pd, wctproblem *problem);
int sequential_branching_conflict(wctproblem *problem);
/** Conflict Branching CBFS exploration */
int sequential_cbfs_branch_and_bound_conflict(wctproblem *problem);
/** Initialize BB tree */
void init_BB_tree(wctproblem *problem);

/*Help functions for branching*/
int insert_into_branching_heap(wctdata *pd, wctproblem *problem);
int skip_wctdata(wctdata *pd, wctproblem *problem);
int branching_msg(wctdata *pd, wctproblem *problem);
int branching_msg_wide(wctdata *pd, wctproblem *problem);
int branching_msg_cbfs(wctdata *pd, wctproblem *problem);
int collect_same_children(wctdata *pd);
int collect_diff_children(wctdata *pd);
void temporary_data_free(wctdata *pd);
int new_eindex(int v, int v1, int v2);
int prune_duplicated_sets(wctdata *pd);
int double2int(int *kpc_pi, int *scalef, const double *pi, int vcount);
double safe_lower_dbl(int numerator, int denominator);
int add_newsets(wctdata *pd);
int add_to_init(wctproblem *problem, Scheduleset *sets, int nbsets);
int delete_to_bigcclasses(wctdata *pd, int capacity);
int add_some_maximal_stable_sets(wctdata *pd, int number);
int insert_into_branching_heap(wctdata *pd, wctproblem *problem);
/** help function for cbfs */
void insert_node_for_exploration(wctdata *pd, wctproblem *problem);
wctdata *get_next_node(wctproblem *problem);

int remove_finished_subtreebis(wctdata *child);

void make_pi_feasible(wctdata *pd);

/*Backup*/
int backup_wctdata(wctdata *pd, wctproblem *problem);

/**
 * wct.c
 */

int read_problem(wctproblem *problem);

/** Preprocess data */
gint compare_readytime(gconstpointer a, gconstpointer b);
int calculate_ready_due_times(Job *  jobarray,
                              int    njobs,
                              int    nmachines,
                              double Hmin);
int calculate_Hmax(Job *jobarray, int nmachines, int njobs);
int calculate_Hmin(
    int *durations, int nmachines, int njobs, int *perm, double *H);
int preprocess_data(wctproblem *problem);

/**
 * solverwrapper.cc
 */

/**
 * Stabilization techniques
 */
int solve_stab(wctdata *pd, wctparms *parms);
int solve_stab_dynamic(wctdata *pd, wctparms *parms);

/**
 * solver zdd
 */
int solvedblzdd(wctdata *pd);
int solvedblbdd(wctdata *pd);
int solve_dynamic_programming_ahv(wctdata *pd);
int solve_weight_dbl_bdd(wctdata *pd);
int solve_weight_dbl_zdd(wctdata *pd);
int solve_pricing(wctdata *pd, wctparms *parms);
int solve_farkas_dbl(wctdata *pd);
int solve_farkas_dbl_DP(wctdata *pd);
void print_dot_file(PricerSolver *solver, char *name);

#ifdef __cplusplus
}
#endif

#endif
