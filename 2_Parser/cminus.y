/****************************************************/
/* File: tiny.y                                     */
/* The TINY Yacc/Bison specification file           */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/
%{
#define YYPARSER /* distinguishes Yacc output from other code files */

#include "globals.h"
#include "util.h"
#include "scan.h"
#include "parse.h"

#define YYSTYPE TreeNode *
static char * savedName; /* for use in assignments */
static int savedNumber;
static int savedLineNo;  /* ditto */
static TreeNode * savedTree; /* stores syntax tree for later return */
static int yylex(void); // added 11/2/11 to ensure no conflict with lex

%}

%token IF ELSE WHILE RETURN INT VOID
%token ID NUM 
%token ASSIGN EQ NE LT LE GT GE PLUS MINUS TIMES OVER
%token LPAREN RPAREN LBRACE RBRACE LCURLY RCURLY SEMI COMMA
%token ERROR 

%nonassoc NO_ELSE
%nonassoc ELSE

%% /* Grammar for TINY */

program     : decl_list{
                 savedTree = $1;
              } 
            ;
decl_list   : decl_list decl{
                   YYSTYPE temp = $1;
                   if (temp == NULL){
                        $$ = $2; 
                   }
                   else{
                        while (temp->sibling != NULL){
                            temp = temp->sibling;
                        }
                        temp->sibling = $2;
                        $$ = $1;
                   }
               }
            | decl
            ;
decl        : var_decl
            | fun_decl
            ;
saveName    : ID{
                savedName = copyString(tokenString);
                savedLineNo = lineno;
              }
            ;
saveNumber  : NUM{
                savedNumber = atoi(tokenString);
                savedLineNo = lineno;
              }
            ;
var_decl    : type_spec saveName SEMI{
                   $$ = newDeclNode(VarK);
                   $$->child[0] = $1;
                   $$->lineno = savedLineNo;
                   $$->attr.name = savedName;
              }
            | type_spec saveName LBRACE saveNumber RBRACE SEMI{
                   $$ = newDeclNode(ArrVarK);
                   $$->child[0] = $1;
                   $$->lineno = savedLineNo;
                   $$->attr.arr.name = savedName;
                   $$->attr.arr.size = savedNumber;
              }
            ;            
type_spec   : INT{
                $$ = newTypeNode(TypeNameK);
                $$->attr.type = INT;
              }
            | VOID{
                $$ = newTypeNode(TypeNameK);
                $$->attr.type = VOID;
              }
            ;
fun_decl    : type_spec saveName{ 
                   $$ = newDeclNode(FuncK);
                   $$->lineno = savedLineNo;
                   $$->attr.name = savedName;
              }
              LPAREN params RPAREN comp_stmt{
                   $$ = $3;
                   $$->child[0] = $1;
                   $$->child[1] = $5;
                   $$->child[2] = $7;
              }
            ;
params      : param_list
            | type_spec{
                   $$ = newParamNode(NonArrParamK);
                   $$->child[0] = $1;
                   $$->attr.name = copyString("(null)");
              }
            ;
param_list  : param_list COMMA param{
                   YYSTYPE temp = $1;
                   if(temp == NULL){
                       $$ = $3;
                   }
                   else{
                       while (temp->sibling != NULL){
                           temp = temp->sibling;
                       }
                       temp->sibling = $3;
                       $$ = $1; 
                   }
              }
            | param
            ;
param       : type_spec saveName{
                   $$ = newParamNode(NonArrParamK);
                   $$->child[0] = $1;
                   $$->attr.name = savedName;
              }
            | type_spec saveName LBRACE RBRACE{
                   $$ = newParamNode(ArrParamK);
                   $$->child[0] = $1;
                   $$->attr.name = savedName;
              }
            ;
comp_stmt   : LCURLY local_decls stmt_list RCURLY{
                   $$ = newStmtNode(CompK);
                   $$->child[0] = $2;
                   $$->child[1] = $3;
              }
            ;
local_decls : local_decls var_decl{
                   YYSTYPE temp = $1;
                   if(temp == NULL){
                        $$ = $2;
                   }
                   else{
                        while (temp->sibling != NULL){
                            temp = temp->sibling;
                        }
                        temp->sibling = $2;
                        $$ = $1; 
                   }
              }
            | { $$ = NULL; }
            ;
stmt_list   : stmt_list stmt{
                   YYSTYPE temp = $1;
                   if(temp == NULL){
                        $$ = $2;
                   }
                   else{
                        while (temp->sibling != NULL)
                            temp = temp->sibling;
                        temp->sibling = $2;
                        $$ = $1; 
                   }
              }
            | { $$ = NULL; }
            ;
stmt        : exp_stmt
            | comp_stmt
            | sel_stmt
            | iter_stmt
            | ret_stmt
            ;
exp_stmt    : exp SEMI { $$= $1; }
            | SEMI  { $$ = NULL; }
            ;
sel_stmt    : IF LPAREN exp RPAREN stmt %prec NO_ELSE{
                   $$ = newStmtNode(IfK);
                   $$->child[0] = $3;
                   $$->child[1] = $5;
                   $$->child[2] = NULL;
              }
            | IF LPAREN exp RPAREN stmt ELSE stmt{
                   $$ = newStmtNode(IfEK);
                   $$->child[0] = $3;
                   $$->child[1] = $5;
                   $$->child[2] = $7;
              }
            ;
iter_stmt   : WHILE LPAREN exp RPAREN stmt{
                   $$ = newStmtNode(IterK);
                   $$->child[0] = $3;
                   $$->child[1] = $5;
              }
            ;
ret_stmt    : RETURN SEMI{
                   $$ = newStmtNode(RetK);
                   $$->child[0] = NULL;
              }
            | RETURN exp SEMI{
                   $$ = newStmtNode(RetK);
                   $$->child[0] = $2;
              }
            ;
exp         : var ASSIGN exp{
                   $$ = newExpNode(AssignK);
                   $$->child[0] = $1;
                   $$->child[1] = $3;
              }
            | simple_exp
            ;
var         : saveName{
                   $$ = newExpNode(IdK);
                   $$->attr.name = savedName;
              }
            | saveName{
                   $$ = newExpNode(ArrIdK);
                   $$->attr.name = savedName;
              }
              LBRACE exp RBRACE{
                   $$ = $2;
                   $$->child[0] = $4;
              }
            ;
/*relop -> LE | LT | GT | GE | EQ | NE
  simple_exp -> add_exp relop add_exp | add_exp
  composigion
 */
simple_exp  : add_exp LE add_exp{
                   $$ = newExpNode(OpK);
                   $$->child[0] = $1;
                   $$->child[1] = $3;
                   $$->attr.op = LE;
              }
            | add_exp LT add_exp{
                   $$ = newExpNode(OpK);
                   $$->child[0] = $1;
                   $$->child[1] = $3;
                   $$->attr.op = LT;
              }
            | add_exp GT add_exp{
                   $$ = newExpNode(OpK);
                   $$->child[0] = $1;
                   $$->child[1] = $3;
                   $$->attr.op = GT;
              }
            | add_exp GE add_exp{
                   $$ = newExpNode(OpK);
                   $$->child[0] = $1;
                   $$->child[1] = $3;
                   $$->attr.op = GE;
              }
            | add_exp EQ add_exp{
                   $$ = newExpNode(OpK);
                   $$->child[0] = $1;
                   $$->child[1] = $3;
                   $$->attr.op = EQ;
              }
            | add_exp NE add_exp{
                   $$ = newExpNode(OpK);
                   $$->child[0] = $1;
                   $$->child[1] = $3;
                   $$->attr.op = NE;
              }
            | add_exp
            ;
/*add_op -> PLUS | MINUS
  add_exp -> add_exp add_op term | term
  composition
 */
add_exp     : add_exp PLUS term{
                   $$ = newExpNode(OpK);
                   $$->child[0] = $1;
                   $$->child[1] = $3;
                   $$->attr.op = PLUS;
              }
            | add_exp MINUS term{
                   $$ = newExpNode(OpK);
                   $$->child[0] = $1;
                   $$->child[1] = $3;
                   $$->attr.op = MINUS;
              }
            | term
            ;
/*mulop -> TIMES | OVER
  term -> term mulop factor | factor
  composition
 */
term        : term TIMES factor{
                   $$ = newExpNode(OpK);
                   $$->child[0] = $1;
                   $$->child[1] = $3;
                   $$->attr.op = TIMES;
              }
            | term OVER factor{
                   $$ = newExpNode(OpK);
                   $$->child[0] = $1;
                   $$->child[1] = $3;
                   $$->attr.op = OVER;
              }
            | factor
            ;
factor      : LPAREN exp RPAREN { $$ = $2; }
            | var
            | call
            | saveNumber{
                   $$ = newExpNode(ConstK);
                   $$->attr.val = savedNumber;
              }
            ;
call        : saveName{
                   $$ = newExpNode(CallK);
                   $$->attr.name = savedName;
              }
              LPAREN args RPAREN{
                   $$ = $2;
                   $$->child[0] = $4;
              }
            ;
args        : arg_list
            | { $$ = NULL; }
            ;
arg_list    : arg_list COMMA exp{
                   YYSTYPE temp = $1;
                   if(temp == NULL){
                        $$ = $3;
                   }
                   else{
                        while (temp->sibling != NULL)
                            temp = temp->sibling;
                        temp->sibling = $3;
                        $$ = $1; 
                   }
              }
            | exp
            ;

%%

int yyerror(char * message)
{ fprintf(listing,"Syntax error at line %d: %s\n",lineno,message);
  fprintf(listing,"Current token: ");
  printToken(yychar,tokenString);
  Error = TRUE;
  return 0;
}

/* yylex calls getToken to make Yacc/Bison output
 * compatible with ealier versions of the TINY scanner
 */
static int yylex(void)
{ return getToken(); }

TreeNode * parse(void)
{ yyparse();
  return savedTree;
}

