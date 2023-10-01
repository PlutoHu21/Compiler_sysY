
#ifndef _AST_H_
#define _AST_H_
#include <iostream>
#include <vector>
#include <stack>
#include <map>
#include <string>
#include <string.h>
#include "sysy_yacc.h"
using namespace std;

typedef enum{
    // 标识符(除保留字), 注释信息
    Ident_ , Comment_, Number_,Root_,Entry_,Exit_,Null_,Label_,Bc_,Br_,
    // 保留字
    INT_, VOID_, IF_, ELSE_, WHILE_, BREAK_, CONTINUE_, RETURN_, CONST_,MAIN_,
    // 非运算符号 (         )    [         ]     {      }     =
    SEMI_, COMM_, LPAREN_, RPAREN_, LSQUARE_, RSQUARE_, LBRACE_, BRAR_, RBRACE_,ASSIGN_,GASSIGN_,
    // 运算符号  -            ==   !=  ||   &&   !   <   <=   >   >=
    POS_,NEG_,ADD_, SUB_, DIV_, MUL_, MOD_, EQ_, NE_, OR_, AND_, NOT_, LT_, LE_, GT_, GE_,Call_,Call_Void,
    // 文件结束, 错误
    ENDF_, ERROR_,Load_,Store_,Alloc_
} TokenType;
typedef enum{
    // 常量声明语句
    ConstDefs_ = 400,CompUnitDecl_,CompUnitFuncDef_,UnaryExp_,Exps_,LVal_,ConstExps_,
    InitVals_,InitVal_,VarDefs_,VarDecl_,ConstDecl_,FuncRParams_,FuncFParams_,LOrExp_,LAndExp_,Cond_,
    Block_,Stmt_Assign_,Stmt_Exp_,Stmt_If_,Stmt_IfElse_,Stmt_While_,Stmt_Return_,Stmt_Return_Void_,Stmt_Conitue_,Stmt_Break_,
    MulExp_Mul_,MulExp_Div_,MulExp_Mod_,AddExp_Sub_,AddExp_Add_,RelExp_LT_,RelExp_GT_,RelExp_LE_,RelExp_GE_,
    EqExp_EQ_,EqExp_NE_,UnaryExp_func_,LVal_Array_,VarDef_array_init_,VarDef_array_,VarDef_single_,FuncFParam_,
    VarDef_single_init_,FuncFParam_array_,FuncFParam_single_,FuncFParam_singleArray_,FuncDef_int_,
    FuncDef_void_,FuncDef_int_para_,FuncDef_void_para_,InitVal_EXP,InitVal_NULL,ConstDef_array_,ConstDef_single_
} GrammaType;
extern int line_no;
// 用于存储变量的名字或者常量的值
typedef struct ast_leaf_node_val {
    TokenType kind;     // 类型，变量名称or常量数值
    int value=0;        // 常量数值
    string ident;       // 变量名称
} expr;
struct ast_node {
    int lineno;                                 // 结点所在行号
    int nodeno;                                 // 节点编号，画图时用于计数
    string name;                                // 结点名字
    int type;                                   // 节点类型
    vector<struct ast_node *> sons;             // 子节点
    struct ast_node *parent;                    // 父节点
    int kind_type;                              // 用来表示是字符，数组或者函数
    struct ast_leaf_node_val val;               // 用来存储变量的名字或者常量的值
    string true_label,false_label,next_label;   // 用于实现短路求值
    // 初始化
    ast_node(){sons.clear(); kind_type=0;}
    ast_node(int l, int x, string y){lineno = l; type=x; name = y; sons.clear();}
};
struct ast_node * new_ast_node(int type,string str,struct ast_node * first_son, 
    struct ast_node * second_son = nullptr,struct ast_node * third_son = nullptr,struct ast_node * forth_son=nullptr);
struct ast_node * new_ast_leaf_node(struct ast_leaf_node_val &val,string str,int no);
extern struct ast_node * ast_root;
void block_ast_tree(struct ast_node *root);
void stack_table_init();
void print_ast(string file);
#endif
