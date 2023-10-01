%{
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string>
#include <map>
#include <algorithm>
#include <stack>

#include "sysy_lex.h"
#include "sysy_yacc.h"
#include "ast.h"

using namespace std;

void yyerror(char * msg);

extern int line_no;
extern int yydebug;
extern int yylex();
extern int yylex_destroy();
extern int yyget_lineno();

//<标识符名称,作用域>
//<标识符名称，<类型,节点指针>>
typedef map<string,pair<int,string>> item;
item initItem;
extern stack<item> idlist;
%}

%union
{
    int token;
    std::string *name ;
    struct ast_node *node;
};

%start  CompUnit


%token <token> ADD SUB NOT
%token <token> MUL DIV MOD 
%token <token> LT LE GT GE
%token <token> EQ NE 
%token <token> AND 
%token <token> OR

%token <token> ASSIGN
%token <token> LPAREN RPAREN LBRACE RBRACE LSQUARE RSQUARE
%token <token> COMMA SEMI
%token <name> IDENT 
%token <token> IF ELSE WHILE FOR BREAK CONTINUE RETURN
%token <token> CONST INT VOID
%token <token> NUMBER

%type <node> Decl ConstDecl ConstList ConstDef ConstExps Exp LVal Exps PrimaryExp UnaryExp UnaryOp FuncRParams MulExp AddExp 
%type <node> CompUnit VarDecl VarDefs VarDef InitVal InitVals FuncDef FuncFParams FuncFParam Block BlockItems BlockItem Stmt Cond RelExp EqExp LAndExp LOrExp Number Ident
%type <node> LBRACE_TEMP LPAREN_TEMP Ident_TEMP Block_in_FuncDef

%left MUL
%left ADD
%left SUB
%left DIV
%left MOD
%left NOT

 
%%
// 编译单元 CompUnit → [ CompUnit ] ( Decl | FuncDef )
CompUnit        :       Decl                    {ast_root->sons.push_back($1);}
                |       FuncDef                 {ast_root->sons.push_back($1);}        
                |       CompUnit Decl           {ast_root->sons.push_back($2);}
                |       CompUnit FuncDef        {ast_root->sons.push_back($2);};
// 声明 Decl → ConstDecl | VarDecl
Decl    :       ConstDecl       {$$=$1;}
        |       VarDecl         {$$=$1;};
// 常量声明 ConstDecl → 'const' BType ConstDef { ',' ConstDef } ';'
ConstDecl       :       CONST INT ConstList SEMI
                {
                        struct ast_leaf_node_val temp_val;
                        temp_val.kind = CONST_;
                        struct ast_node *temp_node;
                        temp_node = new_ast_leaf_node(temp_val,"CONST",line_no);
                        $$=new_ast_node(ConstDecl_,"ConstDecl",temp_node,$3);
                };
// 基本类型 BType → 'int'
ConstList       :       ConstDef        {$$=$1;}
                |       ConstList COMMA ConstDef
                {
                        $1->sons.push_back($3);
                };
// 常数定义 ConstDef → Ident { '[' ConstExp ']' } '=' ConstInitVal
ConstDef        :       Ident ASSIGN InitVal
                {
                        $$=new_ast_node(ConstDef_single_,"ConstDef_single",$1,$3);
                }
                |       Ident LSQUARE ConstExps RSQUARE ASSIGN InitVal
                {
                        $$=new_ast_node(ConstDef_array_,"ConstDef_array",$1,$3,$6);
                };
// 变量声明 VarDecl → BType VarDef { ',' VarDef } ';'
VarDecl         :       INT VarDefs SEMI
                {
                        struct ast_leaf_node_val temp_val;
                        temp_val.kind = INT_;
                        struct ast_node *temp_node;
                        temp_node = new_ast_leaf_node(temp_val,"INT",line_no);
                        $$=new_ast_node(VarDecl_,"VarDecl",temp_node,$2);
                };
// 变量定义 VarDef → Ident { '[' ConstExp ']' } | Ident { '[' ConstExp ']' } '=' InitVal
VarDefs         :       VarDef
                {
                        {$$= new struct ast_node(line_no,VarDefs_,"VarDefs");$$->sons.push_back($1);}
                }
                |       VarDefs COMMA VarDef
                {
                        $1->sons.push_back($3);
                };
VarDef          :        Ident
                {
                        $$=new_ast_node(VarDef_single_,"VarDef_single",$1);
                }
                |       Ident LSQUARE ConstExps RSQUARE
                {
                        $$=new_ast_node(VarDef_array_,"VarDef_array",$1,$3);
                }
                |       Ident ASSIGN InitVal
                {
                        $$=new_ast_node(VarDef_single_init_,"VarDef_single_init",$1,$3);
                }
                |       Ident LSQUARE ConstExps RSQUARE ASSIGN InitVal
                {
                        $$=new_ast_node(VarDef_array_init_,"VarDef_array_init",$1,$3,$6);
                };
// 变量初值 InitVal → Exp | '{' [ InitVal { ',' InitVal } ] '}'
InitVal         :       Exp     {$$=$1;}
                |       LBRACE InitVals RBRACE  {$$=$2;}
                |       LBRACE RBRACE  
                {
                        $$= new struct ast_node(line_no,InitVals_,"InitVals");
                };
InitVals        :       InitVal
                {
                        $$= new struct ast_node(line_no,InitVals_,"InitVal");$$->sons.push_back($1);
                }
                |       InitVals COMMA InitVal
                {
                        $1->sons.push_back($3);                                                                          
                };
// 函数定义 FuncDef → FuncType Ident '(' [FuncFParams] ')' Block
FuncDef         :       VOID Ident_TEMP LPAREN_TEMP FuncFParams RPAREN Block_in_FuncDef
                {
                        struct ast_leaf_node_val temp_val;
                        temp_val.kind = VOID_;
                        struct ast_node *temp_node;
                        temp_node = new_ast_leaf_node(temp_val,"VOID",line_no);
                        $$=new_ast_node(FuncDef_void_para_,"FuncDef_void_para",temp_node,$2,$4,$6);
                }
                |       INT Ident_TEMP LPAREN_TEMP FuncFParams RPAREN Block_in_FuncDef
                {
                        struct ast_leaf_node_val temp_val;
                        temp_val.kind = INT_;
                        struct ast_node *temp_node;
                        temp_node = new_ast_leaf_node(temp_val,"INT",line_no);
                        $$=new_ast_node(FuncDef_int_para_,"FuncDef_int_para",temp_node,$2,$4,$6);
                }
                |       VOID Ident_TEMP LPAREN_TEMP RPAREN Block_in_FuncDef
                {
                        struct ast_leaf_node_val temp_val;
                        temp_val.kind = VOID_;
                        struct ast_node *temp_node;
                        temp_node = new_ast_leaf_node(temp_val,"VOID",line_no);
                        $$=new_ast_node(FuncDef_void_,"FuncDef_void",temp_node,$2,$5);
                }
                |       INT  Ident_TEMP LPAREN_TEMP RPAREN Block_in_FuncDef
                {
                        struct ast_leaf_node_val temp_val;
                        temp_val.kind = INT_;
                        struct ast_node *temp_node;
                        temp_node = new_ast_leaf_node(temp_val,"INT",line_no);
                        $$=new_ast_node(FuncDef_int_,"FuncDef_int",temp_node,$2,$5);
                };
// 函数形参表 FuncFParams → FuncFParam { ',' FuncFParam }
FuncFParams     :       FuncFParam
                {
                        $$=new struct ast_node(line_no,FuncFParam_,"FuncFParam_");$$->sons.push_back($1);
                }
                |       FuncFParams COMMA FuncFParam
                {
                        $1->sons.push_back($3);
                };
// 函数形参 FuncFParam → BType Ident ['[' ']' { '[' Exp ']' }]
FuncFParam      :       INT Ident LSQUARE RSQUARE
                {
                        $$=new_ast_node(FuncFParam_array_,"FuncFParam_array",$2);
                }
                |       INT Ident LSQUARE RSQUARE Exps
                {
                        $$=new_ast_node(FuncFParam_singleArray_,"FuncFParam_singleArray",$2,$5);
                }
                |       INT Ident
                {
                        $$=new_ast_node(FuncFParam_single_,"FuncFParam_single",$2);
                };
// 左小括号
LPAREN_TEMP     :       LPAREN  {};
// 语句块 Block → '{' { BlockItem } '}'
Block           :       LBRACE_TEMP BlockItems RBRACE   {$$=$2;}
                |       LBRACE RBRACE   {};
// 左大括号
LBRACE_TEMP     :       LBRACE
                {
                        struct ast_leaf_node_val temp_val;
                        temp_val.kind = LBRACE_;
                        $$ = new_ast_leaf_node(temp_val,"LBRACE",line_no);    
                };
// 语句块项 BlockItem → Decl | Stmt
BlockItem       :       Decl    {$$=$1;}
                |       Stmt    {$$=$1;};
BlockItems      :       BlockItem
                {
                       $$= new struct ast_node(line_no,Block_,"Block");$$->sons.push_back($1);
                       $1->parent=$$;
                }
                |       BlockItems BlockItem
                {
                        $1->sons.push_back($2);
                        $2->parent=$1;
                };
Block_in_FuncDef:       LBRACE BlockItems RBRACE        {$$=$2;};
// 语句 Stmt → LVal '=' Exp ';' | [Exp] ';' | Block | 'if' '( Cond ')' Stmt [ 'else' Stmt ] 
// | 'while' '(' Cond ')' Stmt | 'break' ';' | 'continue' ';' | 'return' [Exp] ';'
Stmt            :       LVal ASSIGN Exp SEMI
                {
                        $$=new_ast_node(Stmt_Assign_,"Stmt_Assign",$1,$3);
                }
                |       Exp SEMI
                {
                        $$=new_ast_node(Stmt_Exp_,"Stmt_Exp",$1);
                }
                |       Block  {$$=$1;}
                |       IF LPAREN Cond RPAREN Stmt
                {
                        $$=new_ast_node(Stmt_If_,"Stmt_If",$3,$5);   
                }
                |       IF LPAREN Cond RPAREN Stmt ELSE Stmt
                {
                        $$=new_ast_node(Stmt_IfElse_,"Stmt_IfElse",$3,$5,$7);
                }
                |       WHILE LPAREN Cond RPAREN Stmt
                {
                        $$=new_ast_node(Stmt_While_,"Stmt_While",$3,$5);
                        struct ast_node *temp=$$;
                        std::string na=temp->name;
                }
                |       BREAK SEMI
                {
                        $$= new struct ast_node(line_no,Stmt_Break_,"Stmt_Break");
                }
                |       CONTINUE SEMI
                {
                        $$= new struct ast_node(line_no,Stmt_Conitue_,"Stmt_Conitue");
                }
                |       RETURN SEMI
                {
                       $$= new struct ast_node(line_no,Stmt_Return_Void_,"Stmt_Return_Void");
                }
                |       RETURN Exp SEMI
                {
                        $$=new_ast_node(Stmt_Return_,"Stmt_Return",$2);
                }
                |       SEMI   {};
// 表达式 Exp → AddExp
Exps            :       LSQUARE Exp RSQUARE
                {
                        $$= new struct ast_node(line_no,Exps_,"Exps");
                        $$->sons.push_back($2);
                }
                |       Exps LSQUARE Exp RSQUARE
                {
                        $1->sons.push_back($3);
                        $3->parent=$1;
                };
Exp             :       AddExp    {$$=$1;};
// 条件表达式 Cond → LOrExp
Cond            :       LOrExp   {$$=$1;};
// 左值表达式 LVal → Ident {'[' Exp ']'}
LVal            :       Ident
                {
                        $$=new_ast_node(LVal_,"LVal",$1);
                }
                |       Ident Exps
                {
                        $$=new_ast_node( LVal_Array_,"LVal_Array",$1,$2);
                };
// 基本表达式 PrimaryExp → '(' Exp ')' | LVal | Number
PrimaryExp      :       LPAREN Exp RPAREN       {$$=$2;}
                |       LVal                    {$$=$1;}
                |       Number                  {$$=$1;};
// 一元表达式 UnaryExp → PrimaryExp | Ident '(' [FuncRParams] ')' | UnaryOp UnaryExp
UnaryExp        :       PrimaryExp      {$$=$1;}
                |       UnaryOp UnaryExp
                {
                        $$=new_ast_node(UnaryExp_,"UnaryExp_",$1,$2);
                }
                |       Ident LPAREN FuncRParams RPAREN
                {
                        $$=new_ast_node(UnaryExp_func_,"UnaryExp_func",$1,$3);
                }
                |       Ident LPAREN RPAREN
                {
                        $$=new_ast_node(UnaryExp_func_,"UnaryExp_func",$1);
                };
// 单目运算符 UnaryOp → '+' | '−' | '!' 注：'!'仅出现在条件表达式中
UnaryOp         :       ADD    
                {
                        struct ast_leaf_node_val temp_val;
                        temp_val.kind = ADD_;
                        $$ = new_ast_leaf_node(temp_val,"ADD",line_no);
                }
                |       SUB    
                {
                        struct ast_leaf_node_val temp_val;
                        temp_val.kind = SUB_;
                        $$ = new_ast_leaf_node(temp_val,"SUB",line_no);
                }
                |       NOT    
                {
                        struct ast_leaf_node_val temp_val;
                        temp_val.kind = NOT_;
                        $$ = new_ast_leaf_node(temp_val,"NOT",line_no);
                };
// 函数实参表 FuncRParams → Exp { ',' Exp }
FuncRParams     :       Exp
                {
                        $$=new_ast_node(FuncRParams_,"FuncRParams",$1);
                }
                |       FuncRParams COMMA Exp
                {
                        $1->sons.push_back($3);
                        $3->parent=$1;
                };
// 乘除模表达式 MulExp → UnaryExp | MulExp ('*' | '/' | '%') UnaryExp
MulExp          :       UnaryExp        {$$=$1;}
                |       MulExp MUL UnaryExp
                {
                        $$=new_ast_node(MulExp_Mul_,"MulExp_Mul",$1,$3);
                }
                |       MulExp DIV UnaryExp
                {
                        $$=new_ast_node(MulExp_Div_,"MulExp_Div",$1,$3);
                }
                |       MulExp MOD UnaryExp
                {
                        $$=new_ast_node(MulExp_Mod_,"MulExp_Mod",$1,$3);
                };
// 加减表达式 AddExp → MulExp | AddExp ('+' | '−') MulExp
AddExp          :       MulExp  {$$=$1;}
                |       AddExp ADD MulExp
                {
                        $$=new_ast_node(AddExp_Add_,"AddExp_Add",$1,$3);
                }
                |       AddExp SUB MulExp
                {
                        $$=new_ast_node(AddExp_Sub_,"AddExp_Sub",$1,$3);
                };
// 关系表达式 RelExp → AddExp | RelExp ('<' | '>' | '<=' | '>=') AddExp
RelExp          :       AddExp  {$$=$1;}
                |       RelExp LT AddExp
                {
                        $$=new_ast_node(RelExp_LT_,"RelExp_LT",$1,$3);
                }
                |       RelExp GT AddExp
                {
                        $$=new_ast_node(RelExp_GT_,"RelExp_GT",$1,$3);
                }
                |       RelExp LE AddExp
                {
                        $$=new_ast_node(RelExp_LE_,"RelExp_LE",$1,$3);
                }
                |       RelExp GE AddExp
                {
                        $$=new_ast_node(RelExp_GE_,"RelExp_GE",$1,$3);
                };
// 相等性表达式 EqExp → RelExp | EqExp ('==' | '!=') RelExp
EqExp           :       RelExp  {$$=$1;}
                |       EqExp EQ RelExp
                {
                        $$=new_ast_node(EqExp_EQ_,"EqExpp_EQ",$1,$3);
                }
                |       EqExp NE RelExp
                {
                        $$=new_ast_node(EqExp_NE_,"EqExp_NE",$1,$3);
                };
// 逻辑与表达式 LAndExp → EqExp | LAndExp '&&' EqExp
LAndExp         :       EqExp   {$$=$1;}
                |       LAndExp AND EqExp
                {
                        $$=new_ast_node(LAndExp_,"LAndExp_AND",$1,$3);
                }
                ;       
// 逻辑或表达式 LOrExp → LAndExp | LOrExp '||' LAndExp
LOrExp          :       LAndExp {$$=$1;}
                |       LOrExp OR LAndExp
                {
                        $$=new_ast_node(LOrExp_,"LOrExp_OR",$1,$3);
                };
// 常量表达式 ConstExp → AddExp
ConstExps       :       Exp
                {
                        $$= new struct ast_node(line_no,ConstExps_,"ConstExps");
                        $$->sons.push_back($1);
                }
                |       ConstExps RSQUARE LSQUARE Exp
                {
                        $1->sons.push_back($4);
                };
Ident_TEMP      :       Ident   {$$=$1;};

Ident           :       IDENT
                {
                        struct ast_leaf_node_val temp_val;
                        temp_val.kind = Ident_;
                        temp_val.ident=*$1;
                        $$ = new_ast_leaf_node(temp_val,"IDENT",line_no);
                }
                ;

Number          :       NUMBER
                {       
                        struct ast_leaf_node_val temp_val;
                        temp_val.kind = Number_;
                        temp_val.value=$1;
                        $$ = new_ast_leaf_node(temp_val,"NUMBER",line_no);
                }
                ;
                
%%
void yyerror(char * msg)
{
    printf("Line(%d): %s\n", line_no, msg);
}