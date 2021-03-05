#ifndef LOCALSEARCH_H
#define LOCALSEARCH_H
#include "util.h"
#ifdef __cplusplus
extern "C" {
#endif
#include <glib.h>
#include <solution.h>

/* local search methods */
typedef struct _slope_t {
    int b1;    /* begin of segment*/
    int b2;    /* end of segment*/
    int c;     /* value of the function at b1*/
    int alpha; /* slope of the function */
} slope_t;

typedef struct _processing_list_data {
    int pos;
    int p;
} processing_list_data;

typedef struct _local_search_data {
    int                    nmachines;
    int                    njobs;
    int**                  W;
    GList***               g;
    processing_list_data** processing_list_1;
    processing_list_data** processing_list_2;
    int                    updated;
    int                    iterations;
    CCutil_timer           test_swap;
} local_search_data;

void alloc_all(Solution* sol);
void free_all(Solution* sol);

local_search_data* local_search_data_init(int n_jobs, int n_machines);
void               local_search_data_free(local_search_data** data);

/** Preperation of the data */
int local_search_create_W(Solution* sol, local_search_data* data);
int local_search_create_g(Solution* sol, local_search_data* data);
/** Search for the best intra insertion */
void local_search_forward_insertion(Solution*          sol,
                                    local_search_data* data,
                                    int                l);
void local_search_backward_insertion(Solution*          sol,
                                     local_search_data* data,
                                     int                l);
void local_search_swap_intra(Solution*          sol,
                             local_search_data* data,
                             int                l1,
                             int                l2);
void local_search_insertion_inter(Solution*          sol,
                                  local_search_data* data,
                                  int                l);
void local_search_swap_inter(Solution*          sol,
                             local_search_data* data,
                             int                l1,
                             int                l2);
void RVND(Solution* sol, local_search_data* data);

#ifdef __cplusplus
}
#endif

#endif  // LOCALSEARCH_H
