/****************************************************/
/* File: symtab.c                                   */
/* Symbol table implementation for the TINY compiler*/
/* (allows only one symbol table)                   */
/* Symbol table is implemented as a chained         */
/* hash table                                       */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtab.h"

/* SHIFT is the power of two used as multiplier
   in hash function  */
#define SHIFT 4

ScopeList currentScope = NULL;
ScopeList globalScope = NULL;

/* the hash function */
static int hash ( char * key )
{ int temp = 0;
  int i = 0;
  while (key[i] != '\0')
  { temp = ((temp << SHIFT) + key[i]) % SIZE;
    ++i;
  }
  return temp;
}

/*create new Scope*/
ScopeList create_scope(char* name){
    ScopeList temp = globalScope;
    if(temp == NULL){
        temp = (ScopeList)malloc(sizeof(struct ScopeListRec));
        temp->name = name;
        temp->next = NULL;
        temp->location = 0;
        currentScope = temp;
        return temp;
    }
    else{
        while(temp->next != NULL)
            temp = temp->next;
        ScopeList newScope = (ScopeList)malloc(sizeof(struct ScopeListRec));
        newScope->name = name;
        newScope->next = NULL;
        newScope->location = 0;
        newScope->parent = currentScope;
        temp->next = newScope;
        return newScope;
    }
}

/* Procedure st_insert inserts line numbers and
 * memory locations into the symbol table
 * loc = memory location is inserted only the
 * first time, otherwise ignored
 */
ScopeList find_scope(char* name){
    ScopeList temp = globalScope;
    while(strcmp(temp->name, name) != 0 && temp != NULL)
        temp = temp->next;
    return temp;
}

/*insert bucket in scope*/
void st_insert(ScopeList scope ,char * name, ExpType type, int lineno, int loc, int isFunc )
{ ScopeList target_scope = scope;
  if(target_scope == NULL){
    fprintf(listing, "ERROR : %s scope does not exist\n", name);
    return;
  }
  int h = hash(name);
  BucketList l =  target_scope->bucket[h];
  while ((l != NULL) && (strcmp(name,l->name) != 0))
    l = l->next;
  if (l == NULL) /* variable not yet in table */
  { l = (BucketList) malloc(sizeof(struct BucketListRec));
    l->name = name;
    l->lines = (LineList) malloc(sizeof(struct LineListRec));
    l->lines->lineno = lineno;
    l->memloc = loc;
    l->isFunc = isFunc;
    l->type = type;
    l->lines->next = NULL;
    l->next = NULL;
    target_scope->bucket[h] = l; }
  else /* found in table, so just add line number */
  { LineList t = l->lines;
    while (t->next != NULL) t = t->next;
    t->next = (LineList) malloc(sizeof(struct LineListRec));
    t->next->lineno = lineno;
    t->next->next = NULL;
  }
} /* st_insert */

/* Function st_lookup returns the memory 
 * location of a variable or -1 if not found
 */
void add_line(BucketList bucket ,int lineno){
    LineList l = bucket->lines;
    while(l->next != NULL)
        l = l->next;
    LineList temp = (LineList)malloc(sizeof(struct LineListRec));
    temp->lineno = lineno;
    temp->next = NULL;
    l->next = temp;

}

BucketList st_lookup (ScopeList scope, char * name )
{ ScopeList target_scope = scope;
  if(target_scope == NULL){
    fprintf(listing, "ERROR : %s scope does not exist\n", name);
    return NULL;
  }
  int h = hash(name);
  while(target_scope != NULL){
    BucketList l =  target_scope->bucket[h];
    while ((l != NULL) && (strcmp(name,l->name) != 0))
        l = l->next;
    if (l != NULL)
        return l;
    target_scope = target_scope->parent;
  }
  if(target_scope == NULL)
      return NULL;
}

BucketList st_lookat (ScopeList scope, char * name ){
  ScopeList target_scope = scope;
  if(target_scope == NULL){
    fprintf(listing, "ERROR : %s scope does not exist\n", name);
    return NULL;
  }
  int h = hash(name);
  BucketList l =  target_scope->bucket[h];
  while ((l != NULL) && (strcmp(name,l->name) != 0))
      l = l->next;
  return l;
}
/* Procedure printSymTab prints a formatted 
 * listing of the symbol table contents 
 * to the listing file
 */
void printSymTab(FILE * listing)
{   fprintf(listing,"Variable Name  Variable Type  Scope Name  Location   Line Numbers\n");
    fprintf(listing,"-------------  -------------  ----------  --------   ------------\n");
    ScopeList scope = globalScope;
    while(scope != NULL){
        for(int i = 0; i < SIZE; i++){
            if(scope->bucket[i] == NULL){
                continue;
            }
            BucketList bucket = scope->bucket[i];
            while(bucket != NULL){
                fprintf(listing, "%s     ", bucket->name);
                if(bucket->isFunc == 1)
                    fprintf(listing, "Function");
                else{
                    if(bucket->type == Integer)
                        fprintf(listing, "Integer");
                    else if(bucket->type == IntegerArray)
                        fprintf(listing, "IntegerArray");
                    else
                        fprintf(listing, "Void");
                }
                fprintf(listing, "     %s     %d     ", scope->name, bucket->memloc);
                LineList line = bucket->lines;
                while(line != NULL){
                    fprintf(listing, "%d ", line->lineno);
                    line = line->next;
                }
                fprintf(listing, "\n");
                bucket = bucket->next;
            }
        }
        scope = scope->next;
    }
}
/* printSymTab */
