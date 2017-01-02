/*****************************************************************************\
*                                                                             *
*   File name	    dict.h						      *
*									      *
*   Description	    A general purpose dictionary "class" for C programs       *
*									      *
*   Notes:	    A dictionary is an associative array, where the key is    *
*		    a string, and the value is arbitrary.             	      *
*		    Also known as a map in the C++ standard template library. *
*									      *
*		    Implemented as a self-balancing binary tree.              *
*		    							      *
*   Usage:	    #include "dict.h"	   // Include in every module.	      *
*		    DICT_DEFINE_PROCS();   // Define in one of the modules.   *
*		    main() {                                                  *
*		      dict_t *dict = NewDict();				      *
*		      NewDictValue(dict, "one", "Number 1 definition");       *
*		      NewDictValue(dict, "two", "Number 2 definition");       *
*		      printf(DictValue(&dict, "one");			      *
*		    }							      *
*		    							      *
*   History:								      *
*    2010-07-09 JFL Created this module. 				      *
*    2016-12-31 JFL Renamed AddDictValue as NewDictValue, as documented above.*
*    2017-01-02 JFL Adapted to use SysToolsLib's debugm.h definitions.	      *
*                                                                             *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

/* Implemented as a self-balancing binary tree */
#include "tree.h"

/* Private structure used to implement the dictionary nodes */
typedef struct _dictnode {
  TREE_FIELDS(struct _dictnode);
  char *pszKey;
  void *pData;
} dictnode;

/* Define types and declare functions for handling a tree of such structures */
TREE_DEFINE_TYPES(dictnode);

/* Define a convenient type name for a tree of such structures */
typedef TREE(dictnode) dict_t;

/* Declare private static routines defined in the DICT_DEFINE_PROCS() macro. */
int TREE_CMP(dictnode)(dictnode *pn1, dictnode *pn2);
void *dictnode_tree_callback(dictnode *node, void *ref);

/* Declare public static routines */
extern void *NewDictValue(dict_t *dict, char *key, void *value);
extern void DeleteDictValue(dict_t *dict, char *key, void (*cb)(void *value));
extern void *DictValue(dict_t *dict, char *key);

/* Define private types used by the Foreach routine */
typedef void *DICT_CALLBACK_PROC(char *key, void *data, void *ref);
typedef struct {
  DICT_CALLBACK_PROC *cb;
  void *ref;
} DICT_CB_STRUCT;

/* Static routines that need to be defined once somewhere. */
#define DICT_DEFINE_PROCS()							\
TREE_DEFINE_PROCS(dictnode);							\
										\
int TREE_CMP(dictnode)(dictnode *pn1, dictnode *pn2) {				\
  TREE_ENTRY((TREE_V(TREE_CMP(dictnode)) "(%p, %p)\n", pn1, pn2));		\
  TREE_IF_DEBUG({                                                               \
    char key1[80] = "NULL";			                                \
    char key2[80] = "NULL";			                                \
    if (iDebug) {								\
      if (pn1) _snprintf(key1, sizeof(key1), "\"%s\"", pn1->pszKey);		\
      if (pn2) _snprintf(key2, sizeof(key2), "\"%s\"", pn2->pszKey);		\
      printf("%*sstrcmp(%s, %s)\n", iIndent, "", key1, key2);			\
    }										\
  })                                                                            \
  TREE_RETURN_INT(strcmp(pn1->pszKey, pn2->pszKey));				\
}										\
                                                                                \
TREE_IF_DEBUG(                                                                  \
int TREE_SPRINT(dictnode)(char *buf, int len, dictnode *n) {			\
  if (!n) return _snprintf(buf, len, "NULL");					\
  return _snprintf(buf, len, "\"%s\"", n->pszKey);	                        \
}                                                                               \
)                                                                               \
                                                                                \
void *dictnode_tree_callback(dictnode *node, void *ref) {			\
  DICT_CB_STRUCT *s = ref;                                                      \
  return (s->cb)(node->pszKey, node->pData, s->ref);                            \
}                                                                               \
                                                                                \
void *NewDictValue(dict_t *dict, char *key, void *value) {                      \
  dictnode *node = calloc(1, sizeof(dictnode));                                 \
  if (node) {                                                                   \
    node->pszKey = _strdup(key);                                                \
    node->pData = value;                                                        \
    add_dictnode(dict, node);                                                   \
  }                                                                             \
  return node;                                                                  \
}                                                                               \
                                                                                \
void DeleteDictValue(dict_t *dict, char *key, void (*cb)(void *value)) {        \
  dictnode *pNodeFound;                                                         \
  dictnode refNode = {0};                                                       \
  refNode.pszKey = key;                                                         \
  pNodeFound = get_dictnode(dict, &refNode);                                    \
  if (pNodeFound) {								\
    char *pszKey = pNodeFound->pszKey;						\
    if (cb) { /* If defined, call the destructor for the value. */              \
      cb(pNodeFound->pData);                                                    \
    }                                                                           \
    remove_dictnode(dict, pNodeFound);                                          \
    free(pszKey); /* Must be freed afterwards, because it's used by remove()! */\
  }                                                                             \
}                                                                               \
                                                                                \
void *DictValue(dict_t *dict, char *key) {                                      \
  dictnode *pNodeFound;                                                         \
  dictnode refNode = {0};                                                       \
  refNode.pszKey = key;                                                         \
  pNodeFound = get_dictnode(dict, &refNode);                                    \
  if (!pNodeFound) {                                                            \
    return 0;                                                                   \
  }                                                                             \
  return pNodeFound->pData;                                                     \
}                                                                               \

inline dict_t *NewDict() {
  return new_dictnode_tree();
}

inline void *ForeachDictValue(dict_t *dict, DICT_CALLBACK_PROC cb, void *ref) {
  DICT_CB_STRUCT s;
  s.cb = cb;
  s.ref = ref;
  return foreach_dictnode(dict, dictnode_tree_callback, &s);
}

inline dictnode *FirstDictValue(dict_t *dict) {
  return first_dictnode(dict);
}

inline dictnode *NextDictValue(dict_t *dict, dictnode *pn) {
  return next_dictnode(dict, pn);
}

inline dictnode *LastDictValue(dict_t *dict) {
  return last_dictnode(dict);
}

inline dictnode *PrevDictValue(dict_t *dict, dictnode *pn) {
  return prev_dictnode(dict, pn);
}

inline int GetDictSize(dict_t *dict) {
  return num_dictnode(dict);
}


