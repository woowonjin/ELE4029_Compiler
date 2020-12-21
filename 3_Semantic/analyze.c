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
static int compNum = 0;
/* Procedure traverse is a generic recursive 
 * syntax tree traversal routine:
 * it applies preProc in preorder and postProc 
 * in postorder to tree pointed to by t
 */
static void traverse( TreeNode * t,
               void (* preProc) (TreeNode *),
               void (* postProc) (TreeNode *) )
{ if (t != NULL)
  { preProc(t);
    { int i;
      for (i=0; i < MAXCHILDREN; i++){
        traverse(t->child[i],preProc,postProc);
      }
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


static void insertIOFunc(void){ 
   st_insert(globalScope, "output", Void, 0, 0, TRUE); 
   globalScope->location++;
   BucketList output = st_lookat(globalScope, "output");
   output->params[0] = Integer;
   output->paramNumber++;
   st_insert(globalScope, "input", Integer, 0, 1, TRUE); 
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
            t->attr.name = funcName;
            isForFunc = FALSE;
          }
          else{ /*new compound*/
            char t_name = compNum + 48;
            char* temp_name = malloc(2*sizeof(char));
            temp_name[0] = t_name;
            temp_name[1] = '\0';
            ScopeList newScope = create_scope(temp_name);
            compNum++;
            t->attr.name = temp_name;
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
          if(tempBucket == NULL){ /*error*/
            fprintf(listing, "Error at line(%d), name=%s : This Variable is not declared before!! \n", t->lineno, t->attr.name);
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
            fprintf(listing, "Error at line(%d), name=%s : Function Redeclaration Error!!\n", t->lineno, funcName);
            break;
          }
          if(currentScope != globalScope){
            fprintf(listing, "Error at line(%d), name=%s : Function Declaration only in global!!\n", t->lineno, funcName);
            break;
          }
          isForFunc = TRUE;
          ScopeList newScope = create_scope(funcName);
          ExpType type_t;
          if(t->child[0]->attr.type == INT){
             type_t= Integer;
             t->type = Integer;
          }
          else if(t->child[0]->attr.type == VOID){
            type_t = Void;
            t->type = Void;
          }
          else{
            type_t = Void;
            t->type = Void;
          }
          st_insert(currentScope, funcName, type_t, t->lineno, currentScope->location, 1);
          currentScope->location++;
          currentScope = newScope;
          break;
        case VarK:
          tempName = t->attr.name;
          if(st_lookat(currentScope, tempName) == NULL){
              t->type = Integer;
              st_insert(currentScope, tempName, Integer, t->lineno, currentScope->location, 0);
              currentScope->location++;
          }
          else{
            fprintf(listing, "Error at line(%d), name=(%s) : Variable Redeclaration Error!!\n", t->lineno, t->attr.name);
            break;
          }
          tempName = NULL;
          break;
        case ArrVarK:
          tempName = t->attr.arr.name;
          if(st_lookat(currentScope, tempName) == NULL){
              t->type = IntegerArray;
              st_insert(currentScope, tempName, IntegerArray, t->lineno, currentScope->location, 0);
              currentScope->location++;
          }
          else{
            fprintf(listing, "Error at line(%d), name=(%s) : ArrayVariable Redeclaration Error!!\n", t->lineno, tempName);
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

        if(st_lookat(currentScope, t->attr.name) == NULL){
            ExpType temp;
            if(t->kind.param == NonArrParamK){
                temp = Integer;
                t->type = Integer;
            }
            else{
                temp = IntegerArray;
                t->type = IntegerArray;
            }
            st_insert(currentScope, t->attr.name, temp, t->lineno, currentScope->location, 0);
            insertFuncParam(currentScope->name, temp);
            currentScope->location++;
        }
        else{ /*Error*/
            fprintf(listing, "Error at line(%d), name=(%s): Parameter Redeclaration Error!!\n", t->lineno, t->attr.name);
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
  insertIOFunc();
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

static void beforeCheckNode(TreeNode* t){
   switch(t->nodekind){
       case DeclK:
           switch(t->kind.decl){
               case FuncK:
                 funcName = t->attr.name;
                 break;
               default:
                 break;
           }
           break;
       case StmtK:
           switch(t->kind.stmt){
               case CompK:
               {
                 ScopeList scope = find_scope(t->attr.name);
                 currentScope = scope;
                 break;
               }
               default:
                 break;
           }
           break;
       defalut:
           break;
   }
}

static void checkNode(TreeNode * t)
{
  switch (t->nodekind)
  { case ExpK:
      switch (t->kind.exp)
      { case AssignK:
          {//var but type is void
          //fprintf(listing, "AssignK\n");
          ExpType lhsType;
          ExpType rhsType;
          //fprintf(listing, "AssignK\n");
          if(t->child[0]->kind.exp != ConstK && t->child[0]->kind.exp != OpK && t->child[0]->kind.exp != CallK){
            BucketList lhs = st_lookup(currentScope, t->child[0]->attr.name);
            if(lhs->type == IntegerArray){
                lhsType = t->child[0]->type;
            }
            else{
                lhsType = lhs->type;
            }
          }
          else
              lhsType = t->child[0]->type;
          //fprintf(listing, "AssignK\n");
          if(t->child[1]->kind.exp != ConstK && t->child[1]->kind.exp != OpK && t->child[1]->kind.exp != CallK){
            BucketList rhs = st_lookup(currentScope, t->child[1]->attr.name);
            if(rhs->type == IntegerArray){
                rhsType = t->child[1]->type;
            }
            else{
                rhsType = rhs->type;
            }
          }
          else
              rhsType = t->child[1]->type;

          //fprintf(listing, "AssignK\n");
          if(lhsType == Void || rhsType == Void){
            fprintf(listing, "ERROR at line(%d) : Variable type cannot be Void\n", t->lineno);
          }
          // integer array but type is void
          else if(lhsType == IntegerArray && rhsType == Integer){
            fprintf(listing, "ERROR at line(%d) : Variable type does not match\n", t->lineno);
          }
          else if(lhsType == Integer && rhsType == IntegerArray){
            fprintf(listing, "ERROR at line(%d) : Variable type does not match\n", t->lineno);
          }
          else
              t->type = lhsType;
          }
          break;
        case OpK:
          //fprintf(listing, "OpK\n");
          { TreeNode* left = t->child[0];
            TreeNode* right = t->child[1];
            if(left->type == Void || right->type == Void){
                fprintf(listing, "ERROR at line(%d): Operand type cannot be Void\n", t->lineno);
                break;
            }
            ExpType leftType = left->type;
            ExpType rightType = right->type;
            if(left->type == IntegerArray && left->child[0] != NULL){
                // Element of Array
                leftType = Integer;
            }
            if(right->type == IntegerArray && right->child[0] != NULL){
                // Element of Array
                rightType = Integer;
            }

            if(leftType != rightType){
                fprintf(listing, "ERROR at line(%d) : Operand Type does not match\n", t->lineno);
                break;
            }
            t->type = Integer;
          }
          break;
        case ConstK:
          t->type = Integer;
          break;
        case IdK:
          //fprintf(listing, "IdK\n");
          { BucketList id = st_lookup(currentScope, t->attr.name);
            if(id == NULL){
                fprintf(listing, "ERROR : Something Wrong\n");
                break;
            }
            t->type = id->type; 
          }
          break;
        case ArrIdK:
          //fprintf(listing, "ArrIdK\n");
          { BucketList arrId = st_lookup(currentScope, t->attr.name);
            if(arrId == NULL){
                fprintf(listing, "ERROR : Something Wrong\n");
                break;
            }
            if(t->child[0] == NULL) // array
                t->type = IntegerArray;
            else{ //element of arr
                t->type = Integer;
            }
          }
          break;
        case CallK:
          { BucketList func = st_lookat(globalScope, t->attr.name);
            if(func == NULL){
                fprintf(listing, "ERROR at line(%d) : Function not declared before", t->lineno);
                break;
            }
            int argCnt = 0;
            TreeNode* arg = t->child[0];
            int cntError = 0;
            while(arg != NULL){
                if(arg->type != func->params[argCnt]){
                    fprintf(listing, "ERROR at line(%d) : Argument type does not match\n", t->lineno);
                    break;
                }
                argCnt++;
                arg = arg->sibling;
                if(argCnt > func->paramNumber){
                    fprintf(listing, "ERROR at line(%d) : Argument Count does not match\n", t->lineno);
                    cntError = 1;
                    break;
                }
            }
            if(cntError == 0){
                if(argCnt != func->paramNumber){
                    fprintf(listing, "ERROR at line(%d) : Argument Count does not match\n", t->lineno);
                    break;
                }
                else{
                    t->type = func->type;
                }
            }
          }
          break;
        default:
          break;
      }
      break;
    case StmtK:
          //fprintf(listing, "StmtK\n");
      switch (t->kind.stmt)
      {case IfK:
          if(t->child[0] == NULL){
            fprintf(listing, "ERROR at line(%d) : Conditional Expression is needed\n", t->lineno);
            break;
          }
          if(t->child[0]->type == Void){
            fprintf(listing, "ERROR at line(%d) : Conditional Expression cannot be VOID\n", t->lineno);
            break;
          }
          break;
        case IfEK:
          if(t->child[0] == NULL){
            fprintf(listing, "ERROR at line(%d) : If Conditional Expression is need\n", t->lineno);
            break;
          }
          if(t->child[0]->type == Void){
            fprintf(listing, "ERROR at line(%d) : If Conditional Expression cannot be VOID\n", t->lineno);
            break;
          }
          break;
        case IterK:
          if(t->child[0] == NULL){
            fprintf(listing, "ERROR at line(%d) : LOOP Conditional Expression is need\n", t->lineno);
            break;
          }
          if(t->child[0]->type == Void){
            fprintf(listing, "ERROR at line(%d) : LOOP Conditional Expression cannot be VOID\n", t->lineno);
            break;
          }
          break;
        case CompK:
          currentScope = currentScope->parent;
          break;
        case RetK: 
          // error case return type, return value
          //            {void, int}, {void, integerArray}, {int, void}
          //            {int, integerArray}, {integerArray, int}, {integerArray, void}
          {
          BucketList func = st_lookat(globalScope, funcName);
          if(func->type == Void){
            if(t->child[0] != NULL){
                fprintf(listing, "ERROR at line(%d) : Should Return Nothing Error\n", t->lineno);
            }
          }
          else{ //type matching
              if(t->child[0] == NULL){
                    fprintf(listing, "ERROR at line(%d) : Should Return Something Error\n", t->lineno); 
              }
              else{
                if(t->child[0]->nodekind == ExpK && t->child[0]->kind.exp == ConstK){
                    if(func->type != Integer)
                        fprintf(listing, "ERROR at line(%d) : Function type and Return type Does not match\n", t->lineno);
                }
                else{
                    if(func->type != t->child[0]->type){
                        fprintf(listing, "ERROR at line(%d) : Function type and Return type Does not match\n", t->lineno);
                    }
                }
            }
          }
          }
          break;
        default:
          break;
      }
      break;
    case DeclK:
      switch(t->kind.decl){
          case VarK:
              if(t->child[0] == NULL){
                fprintf(listing, "ERROR at line(%d), name(%s) : Variable type cannot be NULL\n", t->lineno, t->attr.name);
                break;
              }
              if(t->child[0]->attr.type == VOID){
                  fprintf(listing, "ERROR at line(%d), name=%s : Variable type cannot be Void\n", t->lineno, t->attr.name);
              }
              break;
          case ArrVarK:
              if(t->child[0] == NULL){
                fprintf(listing, "ERROR at line(%d), name(%s) : Variable type cannot be NULL\n", t->lineno, t->attr.arr.name);
                break;
              }
              if(t->child[0]->attr.type == VOID){
                  fprintf(listing, "ERROR at line(%d), name=%s : Variable type cannot be Void\n", t->lineno, t->attr.arr.name);
              }
              break;
      }
        break;
    case ParamK:
        /* All Function is declared so it is not necessary*/
        break;
    default:
      break;

  }
}

/* Procedure typeCheck performs type checking 
 * by a postorder syntax tree traversal
 */
void typeCheck(TreeNode * syntaxTree)
{ traverse(syntaxTree,beforeCheckNode,checkNode);
}
