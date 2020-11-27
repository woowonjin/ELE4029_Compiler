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
static int savedLineNo;  /* ditto */
static int savedNum;
static TreeNode * savedTree; /* stores syntax tree for later return */
static int yylex(void); // added 11/2/11 to ensure no conflict with lex

%}

%token IF ELSE WHILE RETURN INT VOID
%token NUM ID
%token ASSIGN LT LE GT GE EQ NE PLUS MINUS TIMES OVER
%token LPAREN RPAREN LCURLY RCURLY LBRACE RBRACE SEMI COMMA
%token ERROR 

%nonassoc NO_ELSE
%nonassoc ELSE

%% /* Grammar for TINY */

program             : declaration_list{ savedTree = $1;} 
                    ;
declaration_list    : declaration_list declaration {
                        YYSTYPE temp = $1;
                        if(temp == NULL){
                            $$ = $2;
                        }
                        else{ /* temp is not null*/
                            while(temp->sibling != NULL){
                              temp = temp->sibling;
                            }
                            temp->sibling = $2;
                            $$ = $1;
                        }
                      }
                    | declaration
                    ;

declaration         : var_declaration
                    | fun_declaration
                    ;

saveName            : ID {
                        savedName = copyString(tokenString);
                        savedLineNo = lineno;
                      }
                    ;

saveNum             : NUM {
                        savedNum = atoi(tokenString);
                        savedLineNo = lineno;
                      }

var_declaration     : type_specifier saveName SEMI {
                         $$ = newDeclNode(VarK);
                         $$->child[0] = $1;
                         $$->lineno = savedLineNo;
                         $$->attr.name = savedName;
                      }
                    | type_specifier saveName LBRACE NUM RBRACE SEMI {
                         $$ = newDeclNode(ArrVarK);
                         $$->child[0] = $1;
                         $$->lineno = savedLineNo;
                         $$->attr.name = savedName;
                         $$->attr.arr.size = savedNum;
                      }
                    ;

type_specifier      : INT{
                          $$ = newTypeNode(TypeNameK);
                          $$->attr.type = INT;
                      }
                    | VOID{
                        $$ = newTypeNode(TypeNameK);
                        $$->attr.type = VOID;
                    }
                    ;

fun_declaration     : type_specifier saveName{
                        $$ = newDeclNode(FuncK);
                        $$->attr.name = savedName;
                        $$->lineno = savedLineNo;
                      }
                      LPAREN params RPAREN compound_stmt {
                        $$ = $3;
                        $$->child[0] = $1;
                        $$->child[1] = $5;
                        $$->child[2] = $7;
                      }
                    ;
params              : param_list{$$ = $1;}
                    | VOID{
                        $$ = newTypeNode(TypeNameK);
                        $$->attr.type = VOID;
                      }
                    ;

param_list          : param_list COMMA param{
                        YYSTYPE temp = $1;
                        if(temp == NULL){
                            $$ = $3;
                        }
                        else{
                            while(temp->sibling != NULL){
                                temp = temp->sibling;
                            }
                            temp->sibling = $3;
                            $$ = $1;
                        }
                      }
                    | param
                    ;

param               : type_specifier saveName{
                        $$ = newParamNode(NonArrParamK);
                        $$->child[0] = $1;
                        $$->attr.name = savedName;
                      }
                    | type_specifier saveName LBRACE RBRACE{
                        $$ = newParamNode(ArrParamK);
                        $$->child[0] = $1;
                        $$->attr.name = savedName;
                      }
                    ;

compound_stmt       : LCURLY local_declarations statement_list RCURLY{
                        $$ = newStmtNode(CompK);
                        $$->child[0] = $2;
                        $$->child[1] = $3;
                      }
                    ;

local_declarations  : local_declarations var_declaration{
                        YYSTYPE temp = $1;
                        if(temp == NULL){
                            temp = $2;
                        }
                        else{
                            while(temp->sibling != NULL){
                                temp = temp->sibling;
                            }
                            temp->sibling = $2;
                            $$ = $1;
                        }
                      }
                    | { $$ = NULL; }
                    ;

statement_list      : statement_list statement{
                        YYSTYPE temp = $1;
                        if(temp == NULL){
                            temp = $2;
                        }
                        else{
                            while(temp->sibling != NULL){
                                temp = temp->sibling;
                            }
                            temp->sibling = $2;
                            $$ = $1;
                        
                        }
                      }
                    | {$$ = NULL;}
                    ;

statement           : expression_stmt{$$ = $1;}
                    | compound_stmt{$$ = $1;}
                    | selection_stmt{$$ = $1;}
                    | iteration_stmt{$$ = $1;}
                    | return_stmt{$$ = $1;}
                    ;

expression_stmt     : expression SEMI{
                        $$ = $1;
                      }
                    | SEMI{
                        $$ = NULL;
                      }
                    ;

selection_stmt      : IF LPAREN expression RPAREN statement{
                        $$ = newStmtNode(IfK);
                        $$->child[0] = $3;
                        $$->child[1] = $5;
                        $$->child[2] = NULL;
                      }
                    | IF LPAREN expression RPAREN statement ELSE statement{ 
                        $$ = newStmtNode(IfEK);
                        $$->child[0] = $3;
                        $$->child[1] = $5;
                        $$->child[2] = $7;
                      }
                    ;

iteration_stmt      : WHILE LPAREN expression RPAREN statement{
                        $$ = newStmtNode(IterK);
                        $$->child[0] = $3; 
                        $$->child[1] = $5; 
                      }
                    ;

return_stmt         : RETURN SEMI{
                        $$ = newStmtNode(RetK);
                        $$->child[0] = NULL;
                      }
                    | RETURN expression SEMI{
                        $$ = newStmtNode(RetK);
                        $$->child[0] = $2;
                        }
                    ;

expression          : var ASSIGN expression{
                        $$ = newExpNode(AssignK);
                        $$->child[0] = $1;
                        $$->child[1] = $3;
                      }
                    | simple_expression {$$ = $1;}
                    ;

var                 : saveName{
                        $$ = newExpNode(IdK);
                        $$->attr.name = savedName;
                      }
                    | saveName{
                        $$ = newExpNode(ArrIdK);
                        $$->attr.name = savedName;
                      }
                      LBRACE expression RBRACE{
                        $$->child[0] = $4;
                      }
                    ;

simple_expression   : additive_expression LE additive_expression{
                          $$ = newExpNode(OpK);
                          $$->child[0] = $1;
                          $$->child[1] = $3;
                          $$->attr.op = LE;
                      }
                    | additive_expression LT additive_expression{
                          $$ = newExpNode(OpK);
                          $$->child[0] = $1;
                          $$->child[1] = $3;
                          $$->attr.op = LT;
                      }
                    | additive_expression GT additive_expression{
                          $$ = newExpNode(OpK);
                          $$->child[0] = $1;
                          $$->child[1] = $3;
                          $$->attr.op = GT;
                      }
                    | additive_expression GE additive_expression{
                          $$ = newExpNode(OpK);
                          $$->child[0] = $1;
                          $$->child[1] = $3;
                          $$->attr.op = GE;
                      }
                    | additive_expression EQ additive_expression{
                          $$ = newExpNode(OpK);
                          $$->child[0] = $1;
                          $$->child[1] = $3;
                          $$->attr.op = EQ;
                      }
                    | additive_expression NE additive_expression{
                          $$ = newExpNode(OpK);
                          $$->child[0] = $1;
                          $$->child[1] = $3;
                          $$->attr.op = NE;
                      }
                    | additive_expression
                    ;

additive_expression : additive_expression PLUS term{
                          $$ = newExpNode(OpK);
                          $$->child[0] = $1;
                          $$->child[1] = $3;
                          $$->attr.op = PLUS;
                      }
                      additive_expression MINUS term{
                          $$ = newExpNode(OpK);
                          $$->child[0] = $1;
                          $$->child[1] = $3;
                          $$->attr.op = MINUS;
                      }
                    | term
                    ;

term                : term TIMES factor{
                        $$ = newExpNode(OpK);
                        $$->child[0] = $1;
                        $$->child[1] = $3;
                        $$->attr.op = TIMES;
                      }
                      term OVER factor{
                        $$ = newExpNode(OpK);
                        $$->child[0] = $1;
                        $$->child[1] = $3;
                        $$->attr.op = OVER;
                      }
                    | factor
                    ;

factor              : LPAREN expression RPAREN{
                        $$ = $2;
                      }
                    | var{$$ = $1;}
                    | call{$$ = $1;}
                    | saveNum{
                        $$ = newExpNode(ConstK);
                        $$->attr.val = savedNum;
                      }
                    ;

call                : saveName{
                          $$ = newExpNode(CallK);
                          $$->attr.name = savedName;
                      }
                      LPAREN args RPAREN{
                        $$ = $2;
                        $$->child[0] = $4;
                      }
                    ;

args                : arg_list
                    | {$$ = NULL;}
                    ;

arg_list            : arg_list COMMA expression{
                        YYSTYPE temp = $1;
                        if(temp == NULL){
                            $$ = $3;
                        }
                        else{
                            while(temp->sibling != NULL){
                                temp = temp->sibling;
                            }
                            temp->sibling = $3;
                            $$ = $1;
                        }
                      }
                    | expression{$$ = $1;}
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

