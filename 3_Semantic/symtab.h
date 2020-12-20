/****************************************************/
/* File: symtab.h                                   */
/* Symbol table interface for the TINY compiler     */
/* (allows only one symbol table)                   */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#ifndef _SYMTAB_H_
#define _SYMTAB_H_

#define SIZE 211
#define MAX_SCOPES 1000

#include "globals.h"
/* the list of line numbers of the source 
 * code in which a variable is referenced
 */
typedef struct LineListRec
   { int lineno;
     struct LineListRec * next;
   } * LineList;

/* The record in the bucket lists for
 * each variable, including name, 
 * assigned memory location, and
 * the list of line numbers in which
 * it appears in the source code
 */
typedef struct BucketListRec
   { char * name;
     ExpType type;
     LineList lines;
     int memloc ; /* memory location for variable */
     int isFunc;
     struct BucketListRec * next;
     ExpType params[10];
     int paramNumber;
   } * BucketList;

/* The record for each scope,
 * including name, its bucket,
 * and parent scope
*/
typedef struct ScopeListRec
    { char* name;
      BucketList bucket[SIZE];
      struct ScopeListRec* next;
      struct ScopeListRec* parent;
      int location;
    }* ScopeList;

/* the hash table */
/*static BucketList hashTable[SIZE];
*/


/* Procedure st_insert inserts line numbers and
 * memory locations into the symbol table
 * loc = memory location is inserted only the
 * first time, otherwise ignored
 */
void st_insert(ScopeList scope, char * name, ExpType type, int lineno, int loc, int isFunc );

ScopeList create_scope(char* name);
ScopeList find_scope(char* name);
/* Function st_lookup returns BucketList 
 *
 */
void add_line(BucketList bucket ,int lineno);
BucketList st_lookup(ScopeList scope, char * name );
BucketList st_lookat(ScopeList scope, char* name);
BucketList st_lookup_excluding_parent(ScopeList* scope, char* name);
void insertFuncParam(char* func, ExpType type);
/* Procedure printSymTab prints a formatted 
 * listing of the symbol table contents 
 * to the listing file
 */
void printSymTab(FILE * listing);

#endif
