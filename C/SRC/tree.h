/*****************************************************************************\
*                                                                             *
*   File name	    tree.h						      *
*									      *
*   Description	    A general purpose self-balancing binary tree      	      *
*									      *
*   Notes:	    For use in C programs, in the absence of C++ templates.   *
*		    Implemented as a set of macros, for generating types,     *
*		    structures, and methods for handling an arbitrary tree.   *
*		    The user defines the tree node type, and a comparison     *
*		    routine called TREE_CMP(node_t).                          *
*		    A macro inserts tree-specific fields in that tree node    *
*		    structure. These fields all have the sbbt_ prefix.        *
*		    							      *
*		    Known issues:					      *
*		    - The TREE_DEFINE_* macros will fail if the node type     *
*		      passed by the user has the same name as one of the      *
*                     variables used internally by the macro.                 *
*		    - The TREE_ADD macro does not check for duplicates.	      *
*		    							      *
*		    							      *
*		    Usage:						      *
* "public" macros:                                                            *
*   NODE_FIELDS(struct_node_t) Called once in each node structure definition. *
*   TREE_FIELDS(struct_node_t) Called once in each tree structure definition. *
*   TREE_DEFINE_TYPES(node_t)  Called once for each type of tree, in all files.
*   TREE_DEFINE_PROCS(node_t)  Called once for each type of tree, in ONE file.*
*                                                                             *
* inline public functions: (With NODE_T changed to the node type name)        *
*   new_NODE_T_tree() 		  Create a new node tree.   	              *
*   add_NODE_T(tree, node)	  Insert a new node into the tree.            *
*   remove_NODE_T(tree, node)	  Remove a node from a tree.                  *
*   get_NODE_T(tree, node)	  Search a node in a tree.                    *
*   foreach_NODE_T(tree, func, ref)  Call func(node, ref) for each node.      *
*   rforeach_NODE_T(tree, func, ref) Reverse foreach = Idem, backwards.       *
*   Both foreach routines break out immediately if func() returns non NULL.   *
*   num_NODE_T(tree)	  	  Get the total number of nodes in the tree.  *
*   first_NODE_T(tree)		  Get the first node of the tree.             *
*   next_NODE_T(tree, node)	  Search the first node following a given one.*
*   last_NODE_T(tree)		  Get the last node of the tree.              *
*   prev_NODE_T(tree, node)	  Search the first node preceding a given one.*
*                                                                             *
* Example:                                                                    *
*   #include "tree.h"                                                         *
*   typedef struct _node {                                                    *
*     NODE_FIELDS(struct _node); // The tree-specific fields.                 *
*     char *key; // Any number of fields. For example this will be a key,     *
*     int data;  // and this will be an integer data field.                   *
*   } node;                                                                   *
*   typedef struct _tree {                                                    *
*     TREE_FIELDS(struct _node); // The tree-specific fields.		      *
*     // Optional global property fields				      *
*   } tree;                                                                   *
*   TREE_DEFINE_TYPES(tree, node);                                            *
*   TREE_DEFINE_PROCS(tree, node);                                            *
*   int TREE_CMP(node)(node *n1, node *n2) {return strcmp(n1->key, n2->key);} *
*   node *new_node() {node = calloc(1, sizeof(node)); ...; return node; }     *
*   main() {                                                                  *
*     TREE(node) *tree = new_node_tree();		                      *
*     add_node(tree, new_node("one", 1));                                     *
*     ...                                                                     *
*   }                                                                         *
*		    							      *
*   History:								      *
*    2010-07-06 JFL Created this module. 				      *
*    2017-01-02 JFL Adapted to use SysToolsLib's debugm.h definitions.	      *
*                   Do not use _flushall(), as it also flushes input files!   *
*    2017-01-04 JFL Allow defining global tree properties.            	      *
*                                                                             *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#ifndef _sbb_tree_h_
#define _sbb_tree_h_

#if defined(_MSC_VER) && (_MSC_VER <= 0x800) && (!defined(inline))
  #define inline __inline
#endif

/* Debug macros */
#define TREE_S(arg) #arg /* Stringizing operator (Convert a token to a string) */
#define TREE_V(arg) TREE_S(arg)  /* Valueizing operator (Get the value of a macro expression) */
#define TREE_CAT(arg1, arg2) arg1##arg2 /* Concatenation operator */
#if defined(_DEBUG) && !defined(_MSDOS) /* These macros cause error "out of macro expansion space" with VC++ 1.52 */
#define TREE_IF_DEBUG(tokens) tokens
#define TREE_ENTRY(args) TREE_LOG_ENTRY args
#define TREE_RETURN(result) return TREE_LOG_RETURN(result)
#define TREE_RETURN_PVOID(result) return TREE_LOG_RETURN_PVOID(result)
#define TREE_RETURN_INT(result) return TREE_LOG_RETURN_INT(result)
#include <stdarg.h>
#else
#define TREE_IF_DEBUG(tokens)
#define TREE_ENTRY(args)
#define TREE_RETURN(result) return result
#define TREE_RETURN_PVOID(result) return result
#define TREE_RETURN_INT(result) return result
#endif

/* A balanced tree can never be perfectly balanced */
#define TREE_DELTA_MAX 1 /* The maximum unbalance between sides */
#define TREE_DELTA(node) /* Compute that unbalance */				\
  (  (((node)->sbbt_left)  ? (node)->sbbt_left->sbbt_depth  : 0)		\
   - (((node)->sbbt_right) ? (node)->sbbt_right->sbbt_depth : 0) )		\

/* The tree-specific fields to add to a tree node */
#define NODE_FIELDS(struct_tree_t, struct_node_t)				\
  struct_node_t	*sbbt_left;							\
  struct_node_t	*sbbt_right;							\
  struct_tree_t *sbbt_tree;							\
  int		 sbbt_depth							\

/* The tree-specific fields to add to a tree object */
#define TREE_FIELDS(struct_tree_t, struct_node_t)				\
  struct_node_t *sbbt_root;							\
  int            sbbt_length							\

/* The tree descriptor type */
#define TREE(node_t) TREE_OF_##node_t
/* One-line definition of a new tree */
#define TREE_DEFINE(node_t, tree) TREE(node_t) tree = {0}

/* The comparison routine name */
#define TREE_CMP(node_t) TREE_COMPARE_##node_t##s

/* The optional node print routines names */
#define TREE_PRINT(node_t) TREE_PRINT_##node_t
#define TREE_SPRINT(node_t) TREE_SPRINT_##node_t

#define TREE_DEFINE_TYPES(tree_t, node_t)					\
										\
typedef void *TREE_##node_t##_CB(node_t *n, void *ref);				\
										\
/* The user-provided comparison routine */					\
										\
extern int TREE_CMP(node_t)(node_t *n1, node_t *n2);				\
TREE_IF_DEBUG(									\
extern int TREE_PRINT(node_t)(node_t *n);					\
extern int TREE_SPRINT(node_t)(char *buf, int len, node_t *n);			\
)										\
										\
/* Declare private static functions used by public inline functions. */		\
										\
extern node_t *TREE_ADD_##node_t(node_t *root, node_t *n);			\
extern node_t *TREE_REMOVE_##node_t(node_t *root, node_t *n);			\
extern node_t *TREE_ROTL_##node_t(node_t *root);				\
extern node_t *TREE_ROTR_##node_t(node_t *root);				\
extern node_t *TREE_BALANCE_##node_t(node_t *root);				\
extern node_t *TREE_MERGE_HALVES##node_t(node_t *root, node_t *right);		\
extern node_t *TREE_GET_##node_t(node_t *root, node_t *n);			\
extern node_t *TREE_FIRST_##node_t(node_t *root);				\
extern node_t *TREE_NEXT_##node_t(node_t *root, node_t *n, node_t *next);	\
extern node_t *TREE_LAST_##node_t(node_t *root);				\
extern node_t *TREE_PREV_##node_t(node_t *root, node_t *n, node_t *prev);	\
extern void *TREE_FOREACH_##node_t(node_t *root, TREE_##node_t##_CB *func, void *ref);	\
extern void *TREE_RFOREACH_##node_t(node_t *root, TREE_##node_t##_CB *func, void *ref);	\
TREE_IF_DEBUG(									\
extern void TREE_LOG_ENTRY(char *format, ...);					\
extern node_t *TREE_LOG_RETURN(node_t *result);					\
extern void *TREE_LOG_RETURN_PVOID(void *result);				\
extern int TREE_LOG_RETURN_INT(int result);					\
)										\
										\
/* Declare and define public inline functions. */				\
										\
extern inline tree_t *new_##node_t##_tree() {					\
  tree_t *tree = calloc(1, sizeof(node_t));					\
  return tree;									\
}										\
										\
extern inline void add_##node_t(tree_t *tree, node_t *n) {			\
  n->sbbt_tree = tree; /* Back link into the tree this node belongs to now */	\
  tree->sbbt_root = TREE_ADD_##node_t(tree->sbbt_root, n);			\
  tree->sbbt_length += 1;							\
}										\
										\
extern inline void remove_##node_t(tree_t *tree, node_t *n) {			\
  tree->sbbt_length -= 1;							\
  tree->sbbt_root = TREE_REMOVE_##node_t(tree->sbbt_root, n);			\
}										\
										\
extern inline node_t *get_##node_t(tree_t *tree, node_t *n) {			\
  return TREE_GET_##node_t(tree->sbbt_root, n);					\
}										\
										\
extern inline node_t *first_##node_t(tree_t *tree) {				\
  return TREE_FIRST_##node_t(tree->sbbt_root);					\
}										\
										\
extern inline node_t *next_##node_t(tree_t *tree, node_t *n) {			\
  return TREE_NEXT_##node_t(tree->sbbt_root, n, 0);			    	\
}										\
										\
extern inline node_t *last_##node_t(tree_t *tree) {				\
  return TREE_LAST_##node_t(tree->sbbt_root);					\
}										\
										\
extern inline node_t *prev_##node_t(tree_t *tree, node_t *n) {			\
  return TREE_PREV_##node_t(tree->sbbt_root, n, 0);				\
}										\
										\
extern inline int num_##node_t(tree_t *tree) {					\
  return tree->sbbt_length;							\
}										\
										\
extern inline int depth_##node_t(tree_t *tree) {				\
  return tree->sbbt_root->sbbt_depth;						\
}										\
										\
extern inline void *foreach_##node_t(tree_t *tree, TREE_##node_t##_CB *function, void *ref) {	\
  return TREE_FOREACH_##node_t(tree->sbbt_root, function, ref);			\
}										\
										\
extern inline void *rforeach_##node_t(tree_t *tree, TREE_##node_t##_CB *function, void *ref) {	\
  return TREE_RFOREACH_##node_t(tree->sbbt_root, function, ref);		\
}										\

/* Private routines, to be defined once in one file for each tree type. */

#define TREE_DEFINE_PROCS(tree_t, node_t)					\
										\
node_t *TREE_ADD_##node_t(node_t *root, node_t *n) {				\
  TREE_ENTRY((TREE_S(TREE_ADD_##node_t) "(%p, %p)\n", root, n));		\
  if (!root) TREE_RETURN(n);							\
  if (TREE_CMP(node_t)(n, root) < 0)						\
    root->sbbt_left = TREE_ADD_##node_t(root->sbbt_left, n);			\
  else										\
    root->sbbt_right = TREE_ADD_##node_t(root->sbbt_right, n);			\
  TREE_RETURN(TREE_BALANCE_##node_t(root));					\
}										\
										\
node_t *TREE_REMOVE_##node_t(node_t *root, node_t *n) {				\
  int diff;									\
  TREE_ENTRY((TREE_S(TREE_REMOVE_##node_t) "(%p, %p)\n", root, n));		\
  if (!root) TREE_RETURN(0);							\
  diff = TREE_CMP(node_t)(n, root);						\
  if (diff == 0) {								\
    node_t *tmp = TREE_MERGE_HALVES##node_t(root->sbbt_left, root->sbbt_right);	\
    root->sbbt_left = 0;							\
    root->sbbt_right = 0;							\
    TREE_RETURN(tmp);								\
  }										\
  if (diff < 0)									\
    root->sbbt_left = TREE_REMOVE_##node_t(root->sbbt_left, n);			\
  else										\
    root->sbbt_right = TREE_REMOVE_##node_t(root->sbbt_right, n);		\
  TREE_RETURN(TREE_BALANCE_##node_t(root));					\
}										\
										\
node_t *TREE_GET_##node_t(node_t *root, node_t *n) {				\
  int diff;									\
  TREE_ENTRY((TREE_S(TREE_GET_##node_t) "(%p, %p)\n", root, n));		\
  if (!root) TREE_RETURN(0);							\
  diff = TREE_CMP(node_t)(n, root);						\
  if (diff == 0) TREE_RETURN(root);						\
  if (diff < 0)									\
    TREE_RETURN(TREE_GET_##node_t(root->sbbt_left, n));				\
  else										\
    TREE_RETURN(TREE_GET_##node_t(root->sbbt_right, n));			\
}										\
										\
node_t *TREE_ROTL_##node_t(node_t *root) {					\
  node_t *r = root->sbbt_right;							\
  TREE_ENTRY((TREE_S(TREE_ROTL_##node_t) "(%p)\n", root));			\
  root->sbbt_right = r->sbbt_left;						\
  r->sbbt_left = TREE_BALANCE_##node_t(root);					\
  TREE_RETURN(TREE_BALANCE_##node_t(r));					\
}										\
										\
node_t *TREE_ROTR_##node_t(node_t *root) {					\
  node_t *l = root->sbbt_left;							\
  TREE_ENTRY((TREE_S(TREE_ROTR_##node_t) "(%p)\n", root));			\
  root->sbbt_left = l->sbbt_right;						\
  l->sbbt_right = TREE_BALANCE_##node_t(root);					\
  TREE_RETURN(TREE_BALANCE_##node_t(l));					\
}										\
										\
node_t *TREE_BALANCE_##node_t(node_t *root) {					\
  int delta = TREE_DELTA(root);							\
  TREE_ENTRY((TREE_S(TREE_BALANCE_##node_t) "(%p)\n", root));			\
  if (delta < -TREE_DELTA_MAX) {						\
    if (TREE_DELTA(root->sbbt_right) > 0)					\
      root->sbbt_right = TREE_ROTR_##node_t(root->sbbt_right);			\
    TREE_RETURN(TREE_ROTL_##node_t(root));					\
  } else if (delta > TREE_DELTA_MAX) {						\
    if (TREE_DELTA(root->sbbt_left) < 0)					\
      root->sbbt_left = TREE_ROTL_##node_t(root->sbbt_left);			\
    TREE_RETURN(TREE_ROTR_##node_t(root));					\
  }										\
  root->sbbt_depth = 0;								\
  if (root->sbbt_left && (root->sbbt_left->sbbt_depth > root->sbbt_depth))	\
    root->sbbt_depth = root->sbbt_left->sbbt_depth;				\
  if (root->sbbt_right && (root->sbbt_right->sbbt_depth > root->sbbt_depth))	\
    root->sbbt_depth = root->sbbt_right->sbbt_depth;				\
  root->sbbt_depth += 1;							\
  TREE_RETURN(root);								\
}										\
										\
/* Merge the two halves of a tree split at the head */         			\
/* Move the right tree to the bottom-right of the left tree, and balance it */	\
node_t *TREE_MERGE_HALVES##node_t(node_t *left, node_t *right) {		\
  /* node_t *tip;						*/		\
  TREE_ENTRY((TREE_S(TREE_MERGE_HALVES_##node_t) "(%p, %p)\n", left, right));	\
  if (!left) TREE_RETURN(right);						\
  /* for (tip = left; tip->sbbt_right; tip = tip->sbbt_right) ; */		\
  /* tip->sbbt_right = right;					*/		\
  left->sbbt_right = TREE_MERGE_HALVES##node_t(left->sbbt_right, right);	\
  TREE_RETURN(TREE_BALANCE_##node_t(left));					\
}										\
										\
node_t *TREE_FIRST_##node_t(node_t *root) {					\
  TREE_ENTRY((TREE_S(TREE_FIRST_##node_t) "(%p)\n", root));			\
  if (root) while (root->sbbt_left) root = root->sbbt_left;			\
  TREE_RETURN(root);								\
}										\
										\
node_t *TREE_NEXT_##node_t(node_t *root, node_t *n, node_t *next) {		\
  int diff;									\
  TREE_ENTRY((TREE_S(TREE_NEXT_##node_t) "(%p, %p, %p)\n", root, n, next));	\
  if (!root) TREE_RETURN(next);							\
  diff = TREE_CMP(node_t)(n, root);						\
  if (diff == 0) {								\
    if (root->sbbt_right) next = TREE_FIRST_##node_t(root->sbbt_right);		\
    TREE_RETURN(next);								\
  }										\
  if (diff < 0)									\
    TREE_RETURN(TREE_NEXT_##node_t(root->sbbt_left, n, root));			\
  else										\
    TREE_RETURN(TREE_NEXT_##node_t(root->sbbt_right, n, next));			\
}										\
										\
node_t *TREE_LAST_##node_t(node_t *root) {					\
  TREE_ENTRY((TREE_S(TREE_LAST_##node_t) "(%p)\n", root));			\
  if (root) while (root->sbbt_right) root = root->sbbt_right;			\
  TREE_RETURN(root);								\
}										\
										\
node_t *TREE_PREV_##node_t(node_t *root, node_t *n, node_t *prev) {		\
  int diff;									\
  TREE_ENTRY((TREE_S(TREE_PREV_##node_t) "(%p, %p, %p)\n", root, n, prev));	\
  if (!root) TREE_RETURN(prev);							\
  diff = TREE_CMP(node_t)(n, root);						\
  if (diff == 0) {								\
    if (root->sbbt_left) prev = TREE_LAST_##node_t(root->sbbt_left);		\
    TREE_RETURN(prev);								\
  }										\
  if (diff < 0)									\
    TREE_RETURN(TREE_PREV_##node_t(root->sbbt_left, n, prev));			\
  else										\
    TREE_RETURN(TREE_PREV_##node_t(root->sbbt_right, n, root));			\
}										\
										\
void *TREE_FOREACH_##node_t(node_t *root, TREE_##node_t##_CB *function, void *ref) {	\
  void *result;									\
  TREE_ENTRY((TREE_S(TREE_FOREACH_##node_t) "(%p, %p, %p)\n", root, function, ref));	\
  if (!root) TREE_RETURN_PVOID(0);						\
  result = TREE_FOREACH_##node_t(root->sbbt_left, function, ref);		\
  if (result) TREE_RETURN_PVOID(result);					\
  result = function(root, ref);							\
  if (result) TREE_RETURN_PVOID(result);					\
  result = TREE_FOREACH_##node_t(root->sbbt_right, function, ref);		\
  TREE_RETURN_PVOID(result);							\
}										\
										\
void *TREE_RFOREACH_##node_t(node_t *root, TREE_##node_t##_CB *function, void *ref) {	\
  void *result;									\
  TREE_ENTRY((TREE_S(TREE_RFOREACH_##node_t) "(%p, %p, %p)\n", root, function, ref));	\
  if (!root) TREE_RETURN_PVOID(0);						\
  result = TREE_RFOREACH_##node_t(root->sbbt_right, function, ref);		\
  if (result) TREE_RETURN_PVOID(result);					\
  result = function(root, ref);							\
  if (result) TREE_RETURN_PVOID(result);					\
  result = TREE_RFOREACH_##node_t(root->sbbt_left, function, ref);		\
  TREE_RETURN_PVOID(result);							\
}										\
										\
TREE_IF_DEBUG(									\
void TREE_LOG_ENTRY(char *format, ...) {					\
  va_list args;									\
  if (iDebug) {									\
    va_start(args, format);							\
    printf("%*s", iIndent, "");							\
    vprintf(format, args);							\
    fflush(stdout);								\
    va_end(args);								\
  }										\
  iIndent += 2;									\
}										\
										\
node_t *TREE_LOG_RETURN(node_t *result) {					\
  if (iDebug) {									\
    char buf[80];								\
    TREE_SPRINT(node_t)(buf, sizeof(buf), result);				\
    printf("%*sreturn %p (%s)\n", iIndent, "", result, buf);			\
    fflush(stdout);								\
  }										\
  iIndent -= 2;									\
  return result;								\
}										\
										\
void *TREE_LOG_RETURN_PVOID(void *result) {					\
  if (iDebug) {									\
    printf("%*sreturn %p\n", iIndent, "", result);				\
    fflush(stdout);								\
  }										\
  iIndent -= 2;									\
  return result;								\
}										\
										\
int TREE_LOG_RETURN_INT(int result) {						\
  if (iDebug) {									\
    printf("%*sreturn %d\n", iIndent, "", result);				\
    fflush(stdout);								\
  }										\
  iIndent -= 2;									\
  return result;								\
}										\
)										\

#endif /* _sbb_tree_h_ */

