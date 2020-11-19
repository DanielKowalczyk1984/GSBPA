////////////////////////////////////////////////////////////////
//                                                            //
//  lp.c                                                      //
//  PMC                                                       //
//                                                            //
//  Created by Daniel on 21/02/14.                            //
//  Copyright (c) 2014 Daniel Kowalczyk. All rights reserved. //
//                                                            //
////////////////////////////////////////////////////////////////

#include <lp.h>
#include <util.h>
#include "gurobi_c.h"

const double int_tolerance = 0.00001;

int lp_interface_init(wctlp** lp, const char* name) {
    int val = 0;
    (*lp) = (wctlp*)CC_SAFE_MALLOC(1, wctlp);
    CCcheck_NULL(lp, "Out of memory for lp");
    (*lp)->env = (GRBenv*)NULL;
    (*lp)->model = (GRBmodel*)NULL;
    val = GRBloadenv(&((*lp)->env), NULL);
    CCcheck_val(val, "GRBloadenv failed");

    if (val) {
        printf("val = %d\n", val);
    }

    val = GRBsetintparam((*lp)->env, GRB_INT_PAR_OUTPUTFLAG,
                         (dbg_lvl() > 1) ? 1 : 0);
    CHECK_VAL_GRB(val, "GRBsetintparam OUTPUTFLAG failed", (*lp)->env);
    val = GRBsetintparam((*lp)->env, GRB_INT_PAR_THREADS, 1);
    CHECK_VAL_GRB(val, "GRBsetintparam TREADS failed", (*lp)->env);
    val = GRBsetdblparam((*lp)->env, GRB_DBL_PAR_FEASIBILITYTOL, 1e-9);
    CHECK_VAL_GRB(val, "GRBsetdblparam FEASIBILITYTOL failed", (*lp)->env);
    val = GRBsetintparam((*lp)->env, GRB_INT_PAR_METHOD, GRB_METHOD_PRIMAL);
    CHECK_VAL_GRB(val, "GRBsetintparam LPMETHOD failed", (*lp)->env);
    val = GRBsetintparam((*lp)->env, GRB_INT_PAR_INFUNBDINFO, 1);
    CHECK_VAL_GRB(val, "GRBsetintparam INFUNBDINFO", (*lp)->env);
    val = GRBsetintparam((*lp)->env, GRB_INT_PAR_PRESOLVE, 0);
    CHECK_VAL_GRB(val, "Failed in setting parameter", (*lp)->env);
    val = GRBnewmodel((*lp)->env, &((*lp)->model), name, 0, (double*)NULL,
                      (double*)NULL, (double*)NULL, (char*)NULL, NULL);
    CHECK_VAL_GRB(val, "GRBnewmodel failed", (*lp)->env);
    return val;
}

void lp_interface_free(wctlp** lp) {
    if (*lp) {
        if ((*lp)->model) {
            GRBfreemodel((*lp)->model);
        }

        if ((*lp)->env) {
            GRBfreeenv((*lp)->env);
        }

        free(*lp);
        *lp = (wctlp*)NULL;
    }
}

int lp_interface_optimize(wctlp* lp, int* status) {
    int val;
    val = GRBoptimize((lp)->model);
    CHECK_VAL_GRB(val, "GRBoptimize failed", lp->env);
    val = GRBgetintattr(lp->model, GRB_INT_ATTR_STATUS, status);
    CHECK_VAL_GRB(val, "GRBgetinattr failed", lp->env);

    if (*status != GRB_OPTIMAL && *status != GRB_INFEASIBLE) {
        printf("Failed to solve the model to optimality. status= ");

        switch (*status) {
            case GRB_LOADED:
                printf("GRB_LOADED");
                val = 1;
                break;

            case GRB_UNBOUNDED:
                printf("GRB_UNBOUNDED");
                val = 1;
                break;

            case GRB_INF_OR_UNBD:
                printf("GRB_INF_OR_UNBD");
                val = 1;
                break;

            default:
                printf("%d", *status);
                break;
        }

        printf("\n");

        if (val) {
            goto CLEAN;
        }
    } else if (*status == GRB_INFEASIBLE) {
        lp_interface_compute_IIS(lp);
    }

CLEAN:
    return val;
}

int lp_interface_status(wctlp* lp, int* status) {
    int val = 0;
    val = GRBgetintattr(lp->model, GRB_INT_ATTR_STATUS, status);
    CHECK_VAL_GRB(val, "GRBgetintattr failed", lp->env);
    return val;
}

int lp_interface_objval(wctlp* lp, double* obj) {
    int val = 0;
    val = GRBgetdblattr(lp->model, GRB_DBL_ATTR_OBJVAL, obj);
    CHECK_VAL_GRB(val, "GRBgetdblattr OBJVAL failed", lp->env);
    return val;
}

int lp_interface_change_obj(wctlp* lp, int start, int len, double* values) {
    int val = 0;
    val = GRBsetdblattrarray(lp->model, GRB_DBL_ATTR_OBJ, start, len, values);
    CHECK_VAL_GRB(val, "Failed in GRBsetdblattrarray", lp->env);
    val = GRBupdatemodel(lp->model);
    CHECK_VAL_GRB(val, "Failed to update model", lp->env);
    return val;
}

int lp_interface_addrow(wctlp*  lp,
                        int     nb_zero,
                        int*    column_indices,
                        double* cval,
                        char    sense,
                        double  rhs,
                        char*   name) {
    int val = 0;
    // char inequality_sense;

    // switch (sense) {
    //     case lp_interface_EQUAL:
    //         inequality_sense = GRB_EQUAL;
    //         break;

    //     case lp_interface_LESS_EQUAL:
    //         inequality_sense = GRB_LESS_EQUAL;
    //         break;

    //     case lp_interface_GREATER_EQUAL:
    //         inequality_sense = GRB_GREATER_EQUAL;
    //         break;

    //     default:
    //         fprintf(stderr, "Unknown variable sense: %c\n", sense);
    //         val = 1;
    //         return val;
    // }

    val = GRBaddconstr(lp->model, nb_zero, column_indices, cval, sense, rhs,
                       name);
    CHECK_VAL_GRB(val, "Failed GRBadd", lp->env);
    val = GRBupdatemodel(lp->model);
    CHECK_VAL_GRB(val, "Failed updating the model", lp->env);
    return val;
}

int lp_interface_addrows(wctlp*  lp,
                         int     nb_rows,
                         int     nb_zero,
                         int*    start,
                         int*    column_indices,
                         double* coeff_val,
                         char*   sense,
                         double* rhs,
                         char**  name) {
    int val = 0;
    // char inequality_sense;

    // switch (sense) {
    //     case wctlp_EQUAL:
    //         inequality_sense = GRB_EQUAL;
    //         break;

    //     case lp_interface_LESS_EQUAL:
    //         inequality_sense = GRB_LESS_EQUAL;
    //         break;

    //     case lp_interface_GREATER_EQUAL:
    //         inequality_sense = GRB_GREATER_EQUAL;
    //         break;

    //     default:
    //         fprintf(stderr, "Unknown variable sense: %c\n", sense);
    //         val = 1;
    //         return val;
    // }

    val = GRBaddconstrs(lp->model, nb_rows, nb_zero, start, column_indices,
                        coeff_val, sense, rhs, name);
    CHECK_VAL_GRB(val, "Failed GRBadd", lp->env);
    val = GRBupdatemodel(lp->model);
    CHECK_VAL_GRB(val, "Failed updating the model", lp->env);
    return val;
}

int lp_interface_addcol(wctlp*  lp,
                        int     nb_zero,
                        int*    column_indices,
                        double* cval,
                        double  obj,
                        double  lb,
                        double  ub,
                        char    sense,
                        char*   name) {
    int  val = 0;
    char inequality_sense;

    switch (sense) {
        case lp_interface_CONT:
            inequality_sense = GRB_CONTINUOUS;
            break;

        case lp_interface_BIN:
            inequality_sense = GRB_BINARY;
            break;

        case lp_interface_INT:
            inequality_sense = GRB_INTEGER;
            break;

        default:
            fprintf(stderr, "Unknown variable sense: %c\n", sense);
            val = 1;
            return val;
    }

    val = GRBaddvar(lp->model, nb_zero, column_indices, cval, obj, lb, ub,
                    inequality_sense, name);
    CHECK_VAL_GRB(val, "Failed adding GRBaddvar", lp->env);
    val = GRBupdatemodel(lp->model);
    CHECK_VAL_GRB(val, "Failed updating the model", lp->env);
    return val;
}

int lp_interface_addcols(wctlp*  lp,
                         int     num_vars,
                         int     nb_zero,
                         int*    start,
                         int*    row_indices,
                         double* coeff_val,
                         double* obj,
                         double* lb,
                         double* ub,
                         char*   vtype,
                         char**  name) {
    int val = 0;
    // char inequality_sense;

    // switch (sense) {
    //     case lp_interface_CONT:
    //         inequality_sense = GRB_CONTINUOUS;
    //         break;

    //     case lp_interface_BIN:
    //         inequality_sense = GRB_BINARY;
    //         break;

    //     case lp_interface_INT:
    //         inequality_sense = GRB_INTEGER;
    //         break;

    //     default:
    //         fprintf(stderr, "Unknown variable sense: %c\n", sense);
    //         val = 1;
    //         return val;
    // }

    val = GRBaddvars(lp->model, num_vars, nb_zero, start, row_indices,
                     coeff_val, obj, lb, ub, vtype, name);
    CHECK_VAL_GRB(val, "Failed adding GRBaddvar", lp->env);
    val = GRBupdatemodel(lp->model);
    CHECK_VAL_GRB(val, "Failed updating the model", lp->env);
    return val;
}

int lp_interface_chgcoeff(wctlp*  lp,
                          int     cnt,
                          int*    column_indices,
                          int*    var_indices,
                          double* cval) {
    int val = 0;
    val = GRBchgcoeffs(lp->model, cnt, column_indices, var_indices, cval);
    CHECK_VAL_GRB(val, "Failed to change the coefficient", lp->env);
    val = GRBupdatemodel(lp->model);
    CHECK_VAL_GRB(val, "Failed to update model", lp->env);
    return val;
}

int lp_interface_getcoeff(wctlp*  lp,
                          int*    column_indices,
                          int*    var_indices,
                          double* cval) {
    int val;
    val = GRBgetcoeff(lp->model, *column_indices, *var_indices, cval);
    CHECK_VAL_GRB(val, "Failed to change the coefficient", lp->env);

    return val;
}

int lp_interface_deletecols(wctlp* lp, int first, int last) {
    int  val = 0;
    int  nb_del = last - first + 1;
    int  i;
    int* dellist = CC_SAFE_MALLOC(nb_del, int);
    CCcheck_NULL_2(dellist, "Failed to allocated memory to dellist");

    for (i = 0; i < nb_del; ++i) {
        dellist[i] = first + i;
    }

    val = GRBdelvars(lp->model, nb_del, dellist);
    CHECK_VAL_GRB2(val, "Failed to delete cols", lp->env);
    GRBupdatemodel(lp->model);
    CHECK_VAL_GRB2(val, "Failed to update the model", lp->env);
CLEAN:
    CC_IFFREE(dellist, int);
    return val;
}

int lp_interface_deleterows(wctlp* lp, int first, int last) {
    int  val = 0;
    int  nb_del = last - first + 1;
    int  i;
    int* dellist = CC_SAFE_MALLOC(nb_del, int);
    CCcheck_NULL_2(dellist, "Failed to allocated memory to dellist");

    for (i = 0; i < nb_del; ++i) {
        dellist[i] = first + i;
    }

    val = GRBdelconstrs(lp->model, nb_del, dellist);
    CHECK_VAL_GRB2(val, "Failed to delete rows", lp->env);
    GRBupdatemodel(lp->model);
    CHECK_VAL_GRB2(val, "Failed to update the model", lp->env);
CLEAN:
    CC_IFFREE(dellist, int);
    return val;
}

int lp_interface_chg_upperbounds(wctlp* lp, int first, int last, double ub) {
    int     val = 0;
    int     nb_del = last - first + 1;
    int     i;
    double* ub_array = CC_SAFE_MALLOC(nb_del, double);
    CCcheck_NULL_2(ub_array, "Failed to allocated memory to dellist");

    for (i = 0; i < nb_del; ++i) {
        ub_array[i] = ub;
    }

    val =
        GRBsetdblattrarray(lp->model, GRB_DBL_ATTR_UB, first, nb_del, ub_array);
    CHECK_VAL_GRB2(val, "Failed to delete cols", lp->env);
    GRBupdatemodel(lp->model);
    CHECK_VAL_GRB2(val, "Failed to update the model", lp->env);
CLEAN:
    CC_IFFREE(ub_array, double);
    return val;
}

int lp_interface_pi(wctlp* lp, double* pi) {
    int val = 0;
    int nrows;
    int solstat;
    val = GRBgetintattr(lp->model, GRB_INT_ATTR_STATUS, &solstat);
    CHECK_VAL_GRB(val, "failed to get attribute status gurobi", lp->env);
    val = GRBgetintattr(lp->model, GRB_INT_ATTR_NUMCONSTRS, &nrows);
    CHECK_VAL_GRB(val, "Failed to get nrows", lp->env);

    if (nrows == 0) {
        fprintf(stderr, "No rows in the LP\n");
        val = 1;
        return val;
    }

    val = GRBgetdblattrarray(lp->model, GRB_DBL_ATTR_PI, 0, nrows, pi);
    CHECK_VAL_GRB(val, "Failed to get the dual prices", lp->env);
    return val;
}

int lp_interface_slack(wctlp* lp, double* slack) {
    int val = 0;
    int nrows;
    int solstat;

    val = GRBgetintattr(lp->model, GRB_INT_ATTR_STATUS, &solstat);
    CHECK_VAL_GRB(val, "failed to get atribute status gurobi", lp->env);
    val = GRBgetintattr(lp->model, GRB_INT_ATTR_NUMCONSTRS, &nrows);
    CHECK_VAL_GRB(val, "failed to get the number of rows in gurobi", lp->env);

    if (nrows == 0) {
        fprintf(stderr, "No rows in the model\n");
        val = 1;
        return val;
    }

    val = GRBgetdblattrarray(lp->model, GRB_DBL_ATTR_SLACK, 0, nrows, slack);
    CHECK_VAL_GRB(val, "Failed to get the slack in gurobi\n", lp->env);

    return val;
}

int lp_interface_pi_inf(wctlp* lp, double* pi) {
    int val = 0;
    int nrows;
    int solstat;
    val = GRBgetintattr(lp->model, GRB_INT_ATTR_STATUS, &solstat);
    CHECK_VAL_GRB(val, "failed to get attribute status gurobi", lp->env);
    val = GRBgetintattr(lp->model, GRB_INT_ATTR_NUMCONSTRS, &nrows);
    CHECK_VAL_GRB(val, "Failed to get nrows", lp->env);

    if (nrows == 0) {
        fprintf(stderr, "No rows in the LP\n");
        val = 1;
        return val;
    }

    val = GRBgetdblattrarray(lp->model, GRB_DBL_ATTR_PI, 0, nrows, pi);
    CHECK_VAL_GRB(val, "Failed to get the dual prices", lp->env);
    return val;
}

int lp_interface_x(wctlp* lp, double* x, int first) {
    int val = 0;
    int nb_cols;
    int solstat;
    val = GRBgetintattr(lp->model, GRB_INT_ATTR_STATUS, &solstat);
    CHECK_VAL_GRB(val, "Failed to the status of model", lp->env);

    if (solstat == GRB_INFEASIBLE) {
        fprintf(stderr, "Problem is infeasible\n");
        val = 1;
        return val;
    }

    val = GRBgetintattr(lp->model, GRB_INT_ATTR_NUMVARS, &nb_cols);
    CHECK_VAL_GRB(val, "", lp->env);

    if (nb_cols == 0) {
        fprintf(stderr, "Lp has no variables\n");
        val = 1;
        return val;
    }

    val = GRBgetdblattrarray(lp->model, GRB_DBL_ATTR_X, first, nb_cols - first,
                             x);
    CHECK_VAL_GRB(val, "Failed in GRB_DBL_ATTR_X", lp->env);
    return val;
}

int lp_interface_rc(wctlp* lp, double* rc, int first) {
    int val = 0;
    int nb_cols;
    int solstat;
    val = GRBgetintattr(lp->model, GRB_INT_ATTR_STATUS, &solstat);
    CHECK_VAL_GRB(val, "Failed to the status of model", lp->env);

    if (solstat == GRB_INFEASIBLE) {
        fprintf(stderr, "Problem is infeasible\n");
        val = 1;
        return val;
    }

    val = GRBgetintattr(lp->model, GRB_INT_ATTR_NUMVARS, &nb_cols);
    CHECK_VAL_GRB(val, "", lp->env);

    if (nb_cols == 0) {
        fprintf(stderr, "Lp has no variables\n");
        val = 1;
        return val;
    }

    val = GRBgetdblattrarray(lp->model, GRB_DBL_ATTR_RC, first, nb_cols - first,
                             rc);
    CHECK_VAL_GRB(val, "Failed in GRB_DBL_ATTR_X", lp->env);
    return val;
}

int lp_interface_basis_cols(wctlp* lp, int* column_status, int first) {
    int val = 0;
    int nb_cols, i;
    val = GRBgetintattr(lp->model, GRB_INT_ATTR_NUMVARS, &nb_cols);
    CHECK_VAL_GRB(val, "Failed to get", lp->env);
    val = GRBgetintattrarray(lp->model, GRB_INT_ATTR_VBASIS, first,
                             nb_cols - first, column_status);
    CHECK_VAL_GRB(val, "Failed to get", lp->env);

    for (i = 0; i < nb_cols - first; ++i) {
        switch (column_status[i]) {
            case GRB_BASIC:
                column_status[i] = lp_interface_BASIC;
                break;

            case GRB_NONBASIC_LOWER:
                column_status[i] = lp_interface_LOWER;
                break;

            case GRB_NONBASIC_UPPER:
                column_status[i] = lp_interface_UPPER;
                break;

            case GRB_SUPERBASIC:
                column_status[i] = lp_interface_FREE;
                break;

            default:
                val = 1;
                CHECK_VAL_GRB(val, "ERROR: Received unknwn cstat", lp->env);
                break;
        }
    }

    return val;
}

int lp_interface_set_coltypes(wctlp* lp, char sense) {
    int  nb_vars, i, val = 0;
    char inequality_sense;

    switch (sense) {
        case lp_interface_CONT:
            inequality_sense = GRB_CONTINUOUS;
            break;

        case lp_interface_BIN:
            inequality_sense = GRB_BINARY;
            break;

        case lp_interface_INT:
            inequality_sense = GRB_INTEGER;
            break;

        default:
            fprintf(stderr, "Unknown variable sense: %c\n", sense);
            val = 1;
            return val;
    }

    val = GRBgetintattr(lp->model, GRB_INT_ATTR_NUMVARS, &nb_vars);
    CHECK_VAL_GRB(val, "Failed to get number of variables", lp->env);

    for (i = 0; i < nb_vars; ++i) {
        val = GRBsetcharattrelement(lp->model, GRB_CHAR_ATTR_VTYPE, i,
                                    inequality_sense);
        CHECK_VAL_GRB(val, "Failed to set variable types", lp->env);
    }

    val = GRBupdatemodel(lp->model);
    CHECK_VAL_GRB(val, "Failed to update model", lp->env);
    return val;
}

int lp_interface_get_rhs(wctlp* lp, double* rhs) {
    int val = 0;
    int nb_constr;
    val = GRBgetintattr(lp->model, GRB_INT_ATTR_NUMCONSTRS, &nb_constr);
    CHECK_VAL_GRB(val, "Failed in getting the number of variables", lp->env);
    val = GRBgetdblattrarray(lp->model, GRB_DBL_ATTR_RHS, 0, nb_constr, rhs);
    CHECK_VAL_GRB(val, "Failed in getting the RHS", lp->env);
    return val;
}

int lp_interface_setbound(wctlp* lp, int col, char lb_or_ub, double bound) {
    int val = 0;

    if (lb_or_ub == 'L') {
        val = GRBsetdblattrelement(lp->model, GRB_DBL_ATTR_LB, col, bound);
    } else {
        val = GRBsetdblattrelement(lp->model, GRB_DBL_ATTR_UB, col, bound);
    }

    CHECK_VAL_GRB(val, "Failed to set bound", lp->env);
    val = GRBupdatemodel(lp->model);
    CHECK_VAL_GRB(val, "Failed to update model", lp->env);
    return val;
}

int lp_interface_obj_sense(wctlp* lp, int sense) {
    int val = 0;
    val = GRBsetintattr(lp->model, GRB_INT_ATTR_MODELSENSE, sense);
    CHECK_VAL_GRB(val, "Failed to set obj sense", lp->env);
    return val;
}

int lp_interface_write(wctlp* lp, const char* fname) {
    int val = 0;
    val = GRBwrite(lp->model, fname);
    CHECK_VAL_GRB(val, "failed GRBwrite", lp->env);
    return val;
}

int lp_interface_setnodelimit(wctlp* lp, int mip_node_limit) {
    int rval = GRBsetdblparam(GRBgetenv(lp->model), GRB_DBL_PAR_NODELIMIT,
                              mip_node_limit);
    CHECK_VAL_GRB2(rval, "GRBsetdblparam NODELIMIT failed", lp->env);
CLEAN:
    return rval;
}

void lp_interface_warmstart_free(lp_interface_warmstart** w) {
    if (*w != (lp_interface_warmstart*)NULL) {
        CC_IFFREE((*w)->column_status, int);
        CC_IFFREE((*w)->rstat, int);
        CC_IFFREE((*w)->d_norm, double);
        CC_FREE(*w, lp_interface_warmstart);
    }
}

int lp_interface_get_nb_rows(wctlp* lp, int* nb_rows) {
    int val = 0;
    val = GRBgetintattr(lp->model, GRB_INT_ATTR_NUMCONSTRS, nb_rows);
    CHECK_VAL_GRB(val, "Failed to get the number of variables", lp->env);
    return val;
}

int lp_interface_get_nb_cols(wctlp* lp, int* nb_cols) {
    int val = 0;
    val = GRBgetintattr(lp->model, GRB_INT_ATTR_NUMVARS, nb_cols);
    CHECK_VAL_GRB(val, "Failed to get the number of variables", lp->env);
    return val;
}

void lp_interface_printerrorcode(int c) {
    switch (c) {
        case GRB_ERROR_OUT_OF_MEMORY:
            printf("Available memory was exhausted\n");
            break;

        case GRB_ERROR_NULL_ARGUMENT:
            printf("NULL input value provided for a required argument\n");
            break;

        case GRB_ERROR_INVALID_ARGUMENT:
            printf("An invalid value was provided for a routine argument\n");
            break;

        case GRB_ERROR_UNKNOWN_ATTRIBUTE:
            printf("Tried to query or set an unknown attribute\n");
            break;

        case GRB_ERROR_DATA_NOT_AVAILABLE:
            printf("Attempted to query or set an attribute that could\n");
            printf("not be acessed at that time\n");
            break;

        case GRB_ERROR_UNKNOWN_PARAMETER:
            printf("Tried to query or set an unknown parameter\n");
            break;

        case GRB_ERROR_VALUE_OUT_OF_RANGE:
            printf("Tried to set a parameter to value that is outside\n");
            printf("the parameter's range\n");
            break;

        case GRB_ERROR_NO_LICENSE:
            printf("Failed to obtain a valid license\n");
            break;

        case GRB_ERROR_SIZE_LIMIT_EXCEEDED:
            printf("Attempted to solve a model that is larger than the\n");
            printf("limit for a demo version\n");
            break;

        case GRB_ERROR_CALLBACK:
            printf("Problem with callback\n");
            break;

        case GRB_ERROR_FILE_READ:
            printf("Failed to read the requested file\n");
            break;

        case GRB_ERROR_FILE_WRITE:
            printf("Failed to write the requested file\n");
            break;

        case GRB_ERROR_NUMERIC:
            printf("Numerical error during the requested operation\n");
            break;

        case GRB_ERROR_IIS_NOT_INFEASIBLE:
            printf(
                "Attempted to perform infeasibility analysis on a feasible "
                "model\n");
            break;

        case GRB_ERROR_NOT_FOR_MIP:
            printf("Requested model not valid for a mip model\n");
            break;

        case GRB_ERROR_OPTIMIZATION_IN_PROGRESS:
            printf(
                "Tried to query or modify a model while optimization was in "
                "progress\n");
            break;

        default:
            printf("Unknown error code: %d", c);
            break;
    }
}

double lp_int_tolerance() {
    return int_tolerance;
}

int lp_interface_compute_IIS(wctlp* lp) {
    int val = 0;
    int status;

    GRBgetintattr(lp->model, GRB_INT_ATTR_STATUS, &status);

    if (status == GRB_INFEASIBLE) {
        GRBcomputeIIS(lp->model);
        GRBwrite(lp->model, "compute_iis.ilp");
    }

    return val;
}