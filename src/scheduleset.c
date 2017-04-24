
////////////////////////////////////////////////////////////////
//                                                            //
//  Scheduleset.c                                                //
//  PMC                                                       //
//                                                            //
//  Created by Daniel on 21/02/14.                            //
//  Copyright (c) 2014 Daniel Kowalczyk. All rights reserved. //
//                                                            //
////////////////////////////////////////////////////////////////

#include <Scheduleset.h>
#include <defs.h>
#include <util.h>

void iterator(gpointer key, gpointer value, gpointer user_data);

void iterator(gpointer key, gpointer value, gpointer user_data) {
    GHashTable *new_table = (GHashTable *)user_data;
    g_hash_table_insert(new_table, key, value);
}

static int copy_Schedulesets(Scheduleset *dst, Scheduleset *src, int nsrc);
#define copy_sets()                          \
    {                                        \
        dst->count = src->count;             \
        dst->totweight = src->totweight;     \
        dst->age = src->age;                 \
        dst->totwct = src->totwct;           \
        if (dst->count == 0) {               \
            dst->members = (int *)NULL;      \
            dst->C = (int *)NULL;            \
            dst->table = (GHashTable *)NULL; \
        } else {                             \
            dst->members = src->members;     \
            src->members = (int *)NULL;      \
            dst->C = src->C;                 \
            src->C = (int *)NULL;            \
            dst->table = src->table;         \
            src->table = (GHashTable *)NULL; \
        }                                    \
        dst++;                               \
        src++;                               \
    }

#define ncopy_sets(x)                          \
    {                                          \
        dst[x].count = src[x].count;           \
        dst[x].totweight = src[x].totweight;   \
        dst[x].age = src[x].age;               \
        dst[x].totwct = src[x].totwct;         \
        if (dst[x].count == 0) {               \
            dst[x].members = (int *)NULL;      \
        } else {                               \
            dst[x].members = src[x].members;   \
            src[x].members = (int *)NULL;      \
            dst[x].C = src[x].C;               \
            src[x].C = (int *)NULL;            \
            dst[x].table = src[x].table;       \
            src[x].table = (GHashTable *)NULL; \
        }                                      \
    }

void Scheduleset_init(Scheduleset *set) {
    if (set) {
        set->members = (int *)NULL;
        set->C = (int *)NULL;
        set->table = (GHashTable *)NULL;
        set->count = 0;
        set->age = 0;
        set->totweight = 0;
        set->totwct = 0;
        set->size = 0;
        set->id = -1;
    }
}

void Scheduleset_free(Scheduleset *set) {
    if (set && set->members) {
        CC_IFFREE(set->members, int);
        CC_IFFREE(set->C, int);

        if (set->table) {
            g_hash_table_destroy(set->table);
        }

        set->count = 0;
        set->totweight = 0;
        set->age = 0;
        set->totwct = 0;
        set->size = 0;
    }
}

void Schedulesets_free(Scheduleset **sets, int *nsets) {
    if (*sets) {
        for (int i = 0; i < *nsets; i++) {
            Scheduleset_free(&(*sets)[i]);
        }

        CC_IFFREE(*sets, Scheduleset);
    }

    *nsets = 0;
}

int COLORcopy_sets(Scheduleset **s,
                   int *         nsets,
                   Scheduleset * src_s,
                   int           src_nsets) {
    int val = 0;

    // int i;
    if (src_nsets == 0) {
        return val;
    }

    Schedulesets_free(s, nsets);
    *nsets = src_nsets;
    *s = (Scheduleset *)CC_SAFE_MALLOC(src_nsets, Scheduleset);
    CCcheck_NULL_2(*s, "Failed to allocate memory *s");
    /*for (i = 0; i < src_nsets; ++i) {
        (*s)[i].count = src_s[i].count;
        (*s)[i].totweight = src_s[i].totweight;
        if((*s)[i].count == 0){
            (*s)[i].members = (int*) NULL;
        } else {
            (*s)[i].members = (int*) CC_SAFE_MALLOC(src_s[i].count,int);
            CCcheck_NULL((*s)[i].members,"Failed to allocate (*s)[i].members");
             memcpy((*s)[i].members,src_s[i].members,src_s[i].count *
    sizeof(int));
        }
        (*s)[i].age = src_s[i].age;
    }*/
    copy_Schedulesets(*s, src_s, src_nsets);
CLEAN:
    return val;
}

int update_Schedulesets(Scheduleset **dst,
                        int *         ndst,
                        Scheduleset * src,
                        int           nsrc) {
    int val = 0;
    Schedulesets_free(dst, ndst);
    val = COLORcopy_sets(dst, ndst, src, nsrc);
    CCcheck_val_2(val, "Failed in COLORcopy_sets") CLEAN : return val;
}

static int copy_Schedulesets(Scheduleset *dst, Scheduleset *src, int nsrc) {
    int val = 0;

    if (nsrc & 1) {
        copy_sets();
    }

    nsrc >>= 1;

    if (nsrc & 1) {
        copy_sets();
        copy_sets();
    }

    nsrc >>= 1;

    while (nsrc--) {
        ncopy_sets(0);
        ncopy_sets(1);
        ncopy_sets(2);
        ncopy_sets(3);
        dst += 4;
        src += 4;
    }

    return val;
}

int add_Schedulesets(Scheduleset **dst, int *ndst, Scheduleset *src, int nsrc) {
    int          val = 0;
    Scheduleset *tmpsets = (Scheduleset *)NULL;

    if (*ndst == 0) {
        tmpsets = CC_SAFE_MALLOC(nsrc, Scheduleset);
        CCcheck_NULL_2(tmpsets, "Failed to allocate memory to tmpsets");
        copy_Schedulesets(tmpsets, src, nsrc);
        *dst = tmpsets;
        *ndst = nsrc;
    } else {
        tmpsets = CC_SAFE_MALLOC(nsrc + *ndst, Scheduleset);
        CCcheck_NULL_2(tmpsets, "Failed to allocate memory to tmpsets");
        copy_Schedulesets(tmpsets, src, nsrc);
        memcpy(tmpsets + nsrc, *dst, *ndst);
        Schedulesets_free(dst, ndst);
        *dst = tmpsets;
        *ndst += nsrc;
    }

CLEAN:

    if (val) {
        CC_IFFREE(tmpsets, Scheduleset);
    }

    return val;
}

void Scheduleset_SWAP(Scheduleset *c1, Scheduleset *c2, Scheduleset *t) {
    if (c1 != c2) {
        memcpy(t, c2, sizeof(Scheduleset));
        memcpy(c2, c1, sizeof(Scheduleset));
        memcpy(c1, t, sizeof(Scheduleset));
    }
}

int Scheduleset_less(Scheduleset *c1, Scheduleset *c2) {
    int i;

    if (c1->count != c2->count) {
        return c1->count < c2->count;
    }

    for (i = 0; i < c1->count; ++i) {
        if (c1->members[i] != c2->members[i]) {
            return c1->members[i] < c2->members[i];
        }
    }

    return 0;
}

int Scheduleset_more(Scheduleset *c1, Scheduleset *c2) {
    int i;

    if (c1->count != c2->count) {
        return c1->count > c2->count;
    }

    for (i = 0; i < c1->count; ++i) {
        if (c1->members[i] != c2->members[i]) {
            return c1->members[i] > c2->members[i];
        }
    }

    return 0;
}

int Scheduleset_less_totweight(Scheduleset *c1, Scheduleset *c2) {
    int i;

    if (c1->totweight != c2->totweight) {
        return c1->totweight < c2->totweight;
    }

    for (i = 0; i < c1->count; ++i) {
        if (c1->members[i] != c2->members[i]) {
            return c1->members[i] < c2->members[i];
        }
    }

    return 0;
}

int Scheduleset_more_totweight(Scheduleset *c1, Scheduleset *c2) {
    int i;

    if (c1->totweight != c2->totweight) {
        return c1->totweight > c2->totweight;
    }

    for (i = 0; i < c1->count; ++i) {
        if (c1->members[i] != c2->members[i]) {
            return c1->members[i] < c2->members[i];
        }
    }

    return 0;
}

int Scheduleset_less_wct(Scheduleset *c1, Scheduleset *c2) {
    int i;

    if (c1->totwct != c2->totwct) {
        return c1->totwct < c2->totwct;
    }

    for (i = 0; i < c1->count; ++i) {
        if (c1->members[i] != c2->members[i]) {
            return c1->members[i] < c2->members[i];
        }
    }

    return 0;
}

void Scheduleset_quicksort(Scheduleset *cclasses,
                           int          ccount,
                           int (*functionPtr)(Scheduleset *, Scheduleset *)) {
    int         i, j;
    Scheduleset temp, t;

    if (ccount <= 1) {
        return;
    }

    Scheduleset_SWAP(&(cclasses[0]), &(cclasses[(ccount - 1) / 2]), &temp);
    i = 0;
    j = ccount;
    memcpy(&t, &(cclasses[0]), sizeof(Scheduleset));

    while (1) {
        do {
            i++;
        } while (i < ccount && (*functionPtr)((&cclasses[i]), &t));

        do {
            j--;
        } while ((*functionPtr)(&t, &(cclasses[j])));

        if (j < i) {
            break;
        }

        Scheduleset_SWAP(&(cclasses[i]), &(cclasses[j]), &temp);
    }

    Scheduleset_SWAP(&(cclasses[0]), &(cclasses[j]), &temp);
    Scheduleset_quicksort(cclasses, j, (*functionPtr));
    Scheduleset_quicksort(cclasses + i, ccount - i, (*functionPtr));
}

void Scheduleset_permquicksort(int *        perm,
                               Scheduleset *cclasses,
                               int          ccount,
                               int (*functionPtr)(Scheduleset *,
                                                  Scheduleset *)) {
    int         i, j, temp;
    Scheduleset t;

    if (ccount <= 1) {
        return;
    }

    CC_SWAP(perm[0], perm[(ccount - 1) / 2], temp);
    i = 0;
    j = ccount;
    memcpy(&t, &(cclasses[perm[0]]), sizeof(Scheduleset));

    while (1) {
        do {
            i++;
        } while (i < ccount && (*functionPtr)(&(cclasses[perm[i]]), &t));

        do {
            j--;
        } while ((*functionPtr)(&t, &(cclasses[perm[j]])));

        if (j < i) {
            break;
        }

        CC_SWAP(perm[i], perm[j], temp);
    }

    CC_SWAP(perm[0], perm[j], temp);
    Scheduleset_permquicksort(perm, cclasses, j, (*functionPtr));
    Scheduleset_permquicksort(perm + i, cclasses, ccount - i, (*functionPtr));
}

int Scheduleset_check_set(Scheduleset *set, int vcount) {
    int  val = 0;
    int  i;
    int *coloring = (int *)CC_SAFE_MALLOC(vcount, int);
    CCcheck_NULL_2(coloring, "Could not allocate *newsets");
    fill_int(coloring, vcount, 0);

    for (i = 0; i < set->count; ++i) {
        coloring[set->members[i]] = 1;

        if ((i >= 1) && (set->members[i] < set->members[i - 1])) {
            CCutil_int_array_quicksort(set->members, set->count);
        }
    }

CLEAN:
    CC_IFFREE(coloring, int);
    return val;
}

int Scheduleset_check(Scheduleset *set, int ccount, int vcount) {
    int  val = 0;
    int  i;
    int *covered = (int *)CC_SAFE_MALLOC(vcount, int);
    CCcheck_NULL_2(covered, "Could not allocate *newsets");
    fill_int(covered, vcount, 0);

    for (i = 0; i < ccount; ++i) {
        int j;
        val = Scheduleset_check_set(&(set[i]), vcount);
        CCcheck_val_2(val, "Failed to verify stable set");

        for (j = 0; j < set[i].count; j++) {
            if (covered[set[i].members[j]]) {
                fprintf(stderr,
                        "Node %d is contained in more than one color!\n",
                        set[i].members[j]);
                val = 1;
                goto CLEAN;
            }

            covered[set[i].members[j]] = 1;
        }
    }

    for (i = 0; i < vcount; ++i) {
        if (!covered[i]) {
            fprintf(stderr, "Node %d is not colored!\n", i);
            val = 1;
            goto CLEAN;
        }
    }

CLEAN:
    CC_IFFREE(covered, int);
    return val;
}

int print_schedule(Scheduleset *cclasses, int ccount) {
    int i, j;
    int sum = 0;

    for (i = 0; i < ccount; i++) {
        printf("Machine %d:", i);

        for (j = 0; j < cclasses[i].count; j++) {
            printf(" %d", cclasses[i].members[j]);
        }

        printf(" with totweight %d and %d jobs\n", cclasses[i].totweight,
               cclasses[i].count);
        sum += cclasses[i].count;
    }

    printf("Total of jobs = %d\n", sum);
    fflush(stdout);
    fflush(stdout);
    return 0;
}

int Scheduleset_max(Scheduleset *cclasses, int ccount) {
    int val = 0;
    int i;

    for (i = 0; i < ccount; i++) {
        if (cclasses[i].totweight > val) {
            val = cclasses[i].totweight;
        }
    }

    return val;
}
