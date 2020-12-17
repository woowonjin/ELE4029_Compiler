/****************************************************/
/* File: analyze.c                                  */
/* Semantic analyzer implementation                 */
/* for the TINY compiler                            */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include "globals.h"
#include "symtab.h"
#include "analyze.h"

/* counter for variable memory locations */
static int location = 0;
static char* funcName = NULL;
static int isForFunc = FALSE;
static char* tempName = NULL;
static BucketList tempBucket = NULL;
extern ScopeList currentScope;
extern ScopeList globalScope;
/* Procedure traverse is a generic recursive 
 * syntax tree traversal routine:
 * it applies preProc in preorder and postProc 
 * in postorder to tree pointed to by t
 */
static void traverse( TreeNode * t,
               void (* preProc) (TreeNode *),
               void (* postProc) (TreeNode *) )
{
  if (t != NULL)
  { preProc(t);
    { int i;
      for (i=0; i < MAXCHILDREN; i++)
        traverse(t->child[i],preProc,postProc);
    }
    postProc(t);
    traverse(t->sibling,preProc,postProc);
  }
}

/* nullProc is a do-nothing procedure to 
 * generate preorder-only or postorder-only
 * traversals from traverse
 */
static void nullProc(TreeNode * t)
{ if (t==NULL) return;
  else return;
}

/* Procedure insertNode inserts 
 * identifiers stored in t into 
 * the symbol table 
 */
static void insertNode( TreeNode * t)
{ switch (t->nodekind)
  { case StmtK:
      switch (t->kind.stmt)
      { case CompK:
          if(isForFunc){ /*func compound*/
            isForFunc = FALSE;
          }
          else{ /*new compound*/
            ScopeList newScope = create_scope("None");
            currentScope = newScope;
          }
          break;
        default:
          break;
      }
      break;
    case ExpK:
      switch (t->kind.exp)
      { case IdK:
        case ArrIdK:
        case CallK:
          tempName = t->attr.name;
          tempBucket = st_lookup(currentScope, tempName);
          for(int i = 0; i < SIZE; i++){
            if(globalScope->bucket[i] != NULL){
                BucketList temp_b = globalScope->bucket[i];
                while(temp_b != NULL){
                    fprintf(listing, "%s\n", temp_b->name);
                    temp_b = temp_b->next;
                }
            }
          }
          if(tempBucket == NULL){ /*error*/
            fprintf(listing, "Error : this id is not declared before!! \n");
            break;
          }
          else{
            add_line(tempBucket ,t->lineno); 
          }
          tempName = NULL;
          tempBucket = NULL;
          break;
        default:
          break;
      }
      break;
    case DeclK:
      switch(t->kind.decl)
      { case FuncK:
          funcName = t->attr.name;
          if(st_lookat(globalScope, funcName)){
            fprintf(listing, "Error : function redeclared!!\n");
            break;
          }
          if(currentScope != globalScope){
            fprintf(listing, "Error : you can declare function only in global!!\n");
            break;
          }
          isForFunc = TRUE;
          ScopeList newScope = create_scope(funcName);
          ExpType type_t;
          if(t->child[0]->attr.type == INT){
             type_t= Integer;
          }
          else if(t->child[0]->attr.type == VOID){
            type_t = Void;
          }
          else{
            type_t = Void;
          }
          st_insert(currentScope, funcName, type_t, t->lineno, currentScope->location, 1);
          currentScope->location++;
          currentScope = newScope;
          break;
        case VarK:
          tempName = t->attr.name;
          if(st_lookup(currentScope, tempName) == NULL){
              st_insert(currentScope, tempName, Integer, t->lineno, currentScope->location, 0);
              currentScope->location++;
          }
          else{
            fprintf(listing, "Error : Variable redeclared!!\n");
            break;
          }
          tempName = NULL;
          break;
        case ArrVarK:
          tempName = t->attr.arr.name;
          if(st_lookup(currentScope, tempName) == NULL){
              st_insert(currentScope, tempName, IntegerArray, t->lineno, currentScope->location, 0);
              currentScope->location++;
          }
          else{
            fprintf(listing, "Error : Array Variable redeclared!!\n");
            break;
          }
          tempName = NULL;
          break;
        default:
          break;
      
      }
      break;

    case ParamK:
        if(t->child[0]->attr.type == VOID){
            break;
        }

        if(st_lookup(currentScope, t->attr.name) == NULL){
            ExpType temp;
            if(t->kind.param == NonArrParamK){
                temp = Integer;
            }
            else{
                temp = IntegerArray;
            }
            st_insert(currentScope, t->attr.name, temp, t->lineno, currentScope->location, 0);
            currentScope->location++;
        }
        else{ /*Error*/
            fprintf(listing, "Error: Parameter redeclared!!\n");
        }
      break;

    default:
      break;
  }
}

static void afterInsertNode(TreeNode* t){
    if(t->nodekind == StmtK && t->kind.stmt == CompK){
        currentScope = currentScope->parent;
    }
}

/* Function buildSymtab constructs the symbol 
 * table by preorder traversal of the syntax tree
 */
void buildSymtab(TreeNode * syntaxTree)
{ globalScope = create_scope("global");
  currentScope = globalScope;
  traverse(syntaxTree,insertNode,afterInsertNode);
  /*if (TraceAnalyze)
  { fprintf(listing,"\nSymbol table:\n\n");
    printSymTab(listing);
  }*/
}

static void typeError(TreeNode * t, char * message)
{ fprintf(listing,"Type error at line %d: %s\n",t->lineno,message);
  Error = TRUE;
}

/* Procedure checkNode performs
 * type checking at a single tree node
 */
static void checkNode(TreeNode * t)
{ /*switch (t->nodekind)
  { case ExpK:
      switch (t->kind.exp)
      { case OpK:
          if ((t->child[0]->type != Integer) ||
              (t->child[1]->type != Integer))
            typeError(t,"Op applied to non-integer");
          if ((t->attr.op == EQ) || (t->attr.op == LT))
            t->type = Boolean;
          else
            t->type = Integer;
          break;
        case ConstK:
        case IdK:
          t->type = Integer;
          break;
        default:
          break;
      }
      break;
    case StmtK:
      switch (t->kind.stmt)
      { case IfK:
          if (t->child[0]->type == Integer)
            typeError(t->child[0],"if test is not Boolean");
          break;
        case AssignK:
          if (t->child[0]->type != Integer)
            typeError(t->child[0],"assignment of non-integer value");
          break;
        case WriteK:
          if (t->child[0]->type != Integer)
            typeError(t->child[0],"write of non-integer value");
          break;
        case RepeatK:
          if (t->child[1]->type == Integer)
            typeError(t->child[1],"repeat test is not Boolean");
          break;
        default:
          break;
      }
      break;
    default:
      break;

  }*/
}

/* Procedure typeCheck performs type checking 
 * by a postorder syntax tree traversal
 */
void typeCheck(TreeNode * syntaxTree)
{ traverse(syntaxTree,nullProc,checkNode);
}
