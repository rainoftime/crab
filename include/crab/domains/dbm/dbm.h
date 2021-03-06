/*******************************************************************************
 *
 * Difference Bounds Matrix domain
 *
 * Based on the paper "Fast and Flexible Difference Constraint
 * Propagation for DPLL(T) by Cotton and Maler.
 ******************************************************************************/

#ifndef DBM_H__
#define DBM_H__

#include <cstdio>
#include <iostream>
#include <crab/domains/dbm/expr.h>

/* 
 * Header file for differenct logic domain (intended for sparse
 * matrices)
 * 
 */

// unused types  
typedef val_t dbm_val_t;
typedef int dbm_var_t;

typedef struct {
  unsigned short i_inv;
  unsigned short j_inv;
  val_t val;
} einfo;

typedef struct {
  unsigned short inv;    // Cross reference into live_{srcs,dests}
  short sz;     // Number of elements
  short elt[0]; // Values
} adjlist;

typedef struct {
  int sz;
  int arg_offset;

  int checked;
  int feasible;
  int closed;

  // Potential function
  val_t* pi;

  // Live sources & dests
  short num_srcs;
  short* live_srcs;

  short num_dests;
  short* live_dests;

  // The array of adjacency lists.
  short* srcs;
  short* dests;
  
  // Matrix of edges
  einfo* mtx;  
} dbm_data;

typedef dbm_data *dbm;

dbm dbm_copy(dbm abs);

dbm dbm_bottom();
dbm dbm_top(unsigned int sz);
int dbm_is_bottom(dbm abs);
int dbm_is_top(dbm abs);

int dbm_is_leq(dbm x, dbm y);

int dbm_implies(dbm x, dexpr c);

dbm dbm_join(dbm x, dbm y);
dbm dbm_meet(dbm x, dbm y);
dbm dbm_widen(dbm x, dbm y);
dbm dbm_narrowing(dbm x, dbm y);

void dbm_canonical(dbm x);
 
void dbm_print_to(std::ostream &o, dbm x);

dbm dbm_assign(int v, exp_t expr, dbm x);

dbm dbm_cond(ucon con, dbm x);

dbm dbm_apply_dexpr(dexpr d, dbm x);

dbm dbm_forget(int v, dbm x);
dbm dbm_forget_array(int* vs, int sz, dbm x);
dbm dbm_rename(rmap* subs, int sz, dbm x);
dbm dbm_extract(int* vs, int sz, dbm x);

void dbm_dealloc(dbm d);
void dbm_dealloc_ptr(dbm* d);

bool in_graph(dbm x, int i, int j);
bool src_is_live(dbm abs, int i);
bool dest_is_live(dbm abs, int i);
dbm_var_t copy_var(dbm abs, dbm_var_t x);
void dbm_add_edge(dbm x, int i, int j, val_t val);

dbm dbm_expand(int v, int new_v, dbm x);
dbm dbm_resize(dbm x, int sz);

#endif
