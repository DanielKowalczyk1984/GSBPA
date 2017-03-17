////////////////////////////////////////////////////////////////
//                                                            //
//  main.c                                                    //
//  PMC                                                       //
//                                                            //
//  Created by Daniel on 20/02/14.                            //
//  Copyright (c) 2014 Daniel Kowalczyk. All rights reserved. //
//                                                            //
////////////////////////////////////////////////////////////////

#include <defs.h>
#include <wct.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void usage(char *f) {
    fprintf(stderr, "Usage %s: [-see below-] adjlist_file NrMachines\n", f);
    fprintf(stderr, "   -d      turn on the debugging\n");
    fprintf(stderr,
            "   -f n    Number of feasible solutions that have to be "
            "constructed\n");
    fprintf(stderr,
            "   -s int  Node selection: 0 = none, 1= minimum lower "
            "bound(default), 2 = DFS\n");
    fprintf(stderr, "   -l dbl  Cpu time limit for branching\n");
    fprintf(stderr, "   -L dbl  Cpu time limit for scatter search\n");
    fprintf(stderr,
            "   -C int  Combine method scatter search: 0 = Pathrelinking, 1 = "
            "PM\n");
    fprintf(stderr,
            "   -r int  Scatter search use: 0 = no scatter "
            "search(default), 1 = scatter search\n");
    fprintf(stderr,
            "   -B int  Branch and Bound use: 0 = no branch and bound, 1 "
            "= use branch and bound(default)\n");
    fprintf(stderr,
            "   -S int  Stabilization technique: 0 = no "
            "stabilization(default), 1 = stabilization wentgnes, 2 = "
            "stabilization dynamic\n");
    fprintf(stderr,
            "   -z int  Pricing solver technique: 0 = BDD(default), 1 = "
            "ZDD, 2 = DP\n");
    fprintf(stderr,
            "   -c int  Construct heuristically solutions: 0 = "
            "yes(default), 1 = no\n");
    fprintf(stderr, "   -t int  Use ahv test: 0 = no(default), 1 = yes\n");
    fprintf(stderr, "   -p int  Print csv-files: 0 = no(default), 1 = yes\n");
    fprintf(stderr,
            "   -b int  Branching strategy: 0 = conflict(default), 1 = ahv\n");
    fprintf(stderr,
            "   -Z int  Use strong branching: 0 = use strong "
            "branching(default), 1 = no strong branching\n");
}

static int parseargs(int ac, char **av, wctparms *parms) {
    int c;
    int val = 0;
    int debug = dbg_lvl();

    while ((c = getopt(ac, av, "dr:f:s:l:L:C:B:z:S:c:D:t:p:b:Z:")) != EOF) {
        switch (c) {
            case 'd':
                ++(debug);
                set_dbg_lvl(debug);
                break;

            case 'r':
                val = wctparms_set_scatter_search(parms, atoi(optarg));
                CCcheck_val(val, "Failed cclasses_infile");
                break;

            case 'f':
                val = wctparms_set_nb_feas_sol(parms, atoi(optarg));
                CCcheck_val(val, "Failed number feasible solutions");
                break;

            case 's':
                val = wctparms_set_search_strategy(parms, atoi(optarg));
                CCcheck_val(val, "Failed set_branching_strategy");
                break;

            case 'l':
                val = wctparms_set_branching_cpu_limit(parms, atof(optarg));
                CCcheck_val(val, "Failed wctparms_set_branching_cpu_limit");
                break;

            case 'L':
                val =
                    wctparms_set_scatter_search_cpu_limit(parms, atof(optarg));
                CCcheck_val(val,
                            "Failed wctparms_set_scatter_search_cpu_limit");
                break;

            case 'C':
                val = wctparms_set_combine_method(parms, atoi(optarg));
                CCcheck_val(val, "Failed wctparms_set_combine_method");
                break;

            case 'B':
                val = wctparms_set_branchandbound(parms, atoi(optarg));
                CCcheck_val(val, "Failed wctparms_set_branchandbound");
                break;

            case 'S':
                val = wctparms_set_stab_technique(parms, atoi(optarg));
                CCcheck_val(val, "Failed in wctparms_set_stab_technique");
                break;

            case 'z':
                val = wctparms_set_solver(parms, atoi(optarg));
                CCcheck_val(val, "Failed in wctparms_set_solver");
                break;

            case 'c':
                val = wctparms_set_construct(parms, atoi(optarg));
                CCcheck_val(val, "Failed in construct sol");
                break;

            case 't':
                val = wctparms_set_test_ahv(parms, atoi(optarg));
                CCcheck_val(val, "Failed in use_test");
                break;

            case 'p':
                val = wctparms_set_print(parms, atoi(optarg));
                CCcheck_val(val, "Failed in print");
                break;

            case 'b':
                val = wctparms_set_branching_strategy(parms, atoi(optarg));
                CCcheck_val(val, "Failed in set branching strategy");
                break;

            case 'Z':
                val = wctparms_set_strong_branching(parms, atoi(optarg));
                CCcheck_val(val, "Failed in set strong branching");
                break;

            default:
                usage(av[0]);
                val = 1;
                goto CLEAN;
        }
    }

    if (ac <= optind) {
        val = 1;
        goto CLEAN;
    } else {
        val = wctparms_set_file(parms, av[optind++]);
        CCcheck_val(val, "Failed in wctparms_set_file");

        if (ac <= optind) {
            val = 1;
            goto CLEAN;
        }

        val = wctparms_set_nmachines(parms, atoi(av[optind++]));
        CCcheck_val(val, "Failed in wctparms_set_nmachines");
    }

CLEAN:

    if (val) {
        usage(av[0]);
    }

    return val;
}

MAYBE_UNUSED
static int print_to_csv(wctproblem *problem) {
    int       val = 0;
    wctdata * pd = &(problem->root_pd);
    wctparms *parms = &(problem->parms);
    FILE *    file = (FILE *)NULL;
    char      filenm[128];
    int       size;
    GDate     date;
    g_date_set_time_t(&date, time(NULL));
    problem->real_time = getRealTime() - problem->real_time;
    CCutil_stop_timer(&(problem->tot_cputime), 0);

    switch (parms->bb_branch_strategy) {
        case conflict_strategy:
        case cbfs_conflict_strategy:
            sprintf(filenm, "WCT_CONFLICT_%d_%d.csv", pd->nmachines, pd->njobs);
            break;

        case ahv_strategy:
        case cbfs_ahv_strategy:
            sprintf(filenm, "WCT_AHV_%d_%d.csv", pd->nmachines, pd->njobs);
            break;
    }

    file = fopen(filenm, "a+");

    if (file == NULL) {
        printf("We couldn't open %s in %s at line %d\n", filenm, __FILE__,
               __LINE__);
        val = 1;
        goto CLEAN;
    }

    fseek(file, 0, SEEK_END);
    size = ftell(file);

    if (size == 0) {
        fprintf(file,
                "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%"
                "s,%s\n",
                "NameInstance", "tot_real_time", "tot_cputime", "tot_lb",
                "tot_lb_root", "tot_lb_lp", "tot_branch_and_bound",
                "tot_scatter_search", "tot_build_dd", "tot_pricing",
                "rel_error", "status", "global_lower_bound",
                "global_upper_bound", "first_lower_bound", "first_upper_bound",
                "first_rel_error", "solved_at_root", "nb_explored_nodes",
                "nb_generated_col", "nb_generated_col_root", "date");
    }

    fprintf(file,
            "%s,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%d,%d,%d,%d,%d,%f,%d,%d,%d,%d,%u/"
            "%u/%u\n",
            pd->pname, problem->real_time, problem->tot_cputime.cum_zeit,
            problem->tot_lb.cum_zeit, problem->tot_lb_lp_root.cum_zeit,
            problem->tot_lb_lp.cum_zeit, problem->tot_branch_and_bound.cum_zeit,
            problem->tot_scatter_search.cum_zeit,
            problem->tot_build_dd.cum_zeit, problem->tot_pricing.cum_zeit,
            problem->rel_error, problem->status, problem->global_lower_bound,
            problem->global_upper_bound, problem->first_lower_bound,
            problem->first_upper_bound, problem->first_rel_error,
            problem->found, problem->nb_explored_nodes,
            problem->nb_generated_col, problem->nb_generated_col_root, date.day,
            date.month, date.year);
    fclose(file);
CLEAN:
    return val;
}

MAYBE_UNUSED
static int print_to_screen(wctproblem *problem) {
    int val = 0;

    switch (problem->status) {
        case no_sol:
            printf(
                "We didn't decide if this instance is feasible or "
                "infeasible\n");
            break;

        case feasible:
        case lp_feasible:
        case meta_heur:
            printf("A suboptimal schedule with relative error %f is found.\n",
                   (double)(problem->global_upper_bound -
                            problem->global_lower_bound) /
                       (problem->global_lower_bound));
            break;

        case optimal:
            printf("The optimal schedule is found.\n");
            break;
    }

    printf(
        "Compute_schedule took %f seconds(tot_scatter_search %f, "
        "tot_branch_and_bound %f, tot_lb_lp_root %f, tot_lb_lp %f, tot_lb %f, "
        "tot_pricing %f, tot_build_dd %f) and %f seconds in real time\n",
        problem->tot_cputime.cum_zeit, problem->tot_scatter_search.cum_zeit,
        problem->tot_branch_and_bound.cum_zeit,
        problem->tot_lb_lp_root.cum_zeit, problem->tot_lb_lp.cum_zeit,
        problem->tot_lb.cum_zeit, problem->tot_pricing.cum_zeit,
        problem->tot_build_dd.cum_zeit, problem->real_time);
    return val;
}

int main(int ac, char **av) {
    int           val = 0;
    wctproblem    problem;
    wctdata *     pd;
    wctparms *    parms;
    PricerSolver *solver;
    int *         a = CC_SAFE_MALLOC(1000, int);
    for (unsigned i = 0; i < 100; ++i) {
        a[2 * i] = 2;
        a[2 * i + 1] = 4 + 50 * i;
    }
    double start_time;
    val = program_header(ac, av);
    CCcheck_val_2(val, "Failed in program_header") wctproblem_init(&problem);
    CCutil_start_timer(&(problem.tot_cputime));
    start_time = CCutil_zeit();
    problem.real_time = getRealTime();
    problem.nwctdata = 1;
    pd = &(problem.root_pd);
    wctdata_init(pd);
    pd->id = 0;
    parms = &(problem.parms);
    val = parseargs(ac, av, parms);
    CCcheck_val_2(val, "Failed in parseargs");

    if (dbg_lvl() > 1) {
        printf("Debugging turned on\n");
    }

    fflush(stdout);
    /** Reading and preprocessing the data */
    val = read_problem(&problem);
    CCcheck_val_2(val, "read_adjlist failed");
    val = preprocess_data(&problem);
    printf("%d \n", problem.njobs * problem.T - problem.psum);
    CCcheck_val_2(val, "Failed at preprocess_data");
    // heuristic_rpup(&problem);
    solver = newSolver(problem.jobarray, problem.njobs, pd->H_min, pd->H_max);
    for (int i = 0; i < 100; ++i) {
        add_one_conflict(solver, parms, a[2 * i], a[2 * i + 1], 0);
    }
    printf("test %zu\n", get_datasize(solver));
    printf("Reading and preprocessing of the data took %f seconds\n",
           CCutil_zeit() - start_time);

// /** Computing initial lowerbound */
// CCutil_start_timer(&(problem.tot_lb));

// CCutil_stop_timer(&(problem.tot_lb), 0);
// printf("Computing lowerbound EEI, CP and CW took %f seconds\n",
//        problem.tot_lb.cum_zeit);
// /** Construction Pricersolver at the root node */
// CCutil_start_resume_time(&(problem.tot_build_dd));

// CCutil_suspend_timer(&(problem.tot_build_dd));
// /** Construct Feasible solutions */
// if (parms->nb_feas_sol > 0) {
//     construct_feasible_solutions(&problem);
// }
// /** Compute Schedule with Branch and Price */
// compute_schedule(&problem);
// /** Print all the information to screen and csv */
// if (problem.parms.print) {
//     print_to_csv(&problem);
// }
// print_to_screen(&problem);
CLEAN:
    wctproblem_free(&problem);
    return val;
}
