#include "ast.h"

/* 整个AST的根节点 */
struct ast_node * ast_root = new struct ast_node(-1,Root_,"Root");
// 新结点
struct ast_node * new_ast_node(int type,string str,struct ast_node * first_son, 
    struct ast_node * second_son,struct ast_node * third_son,struct ast_node * forth_son)
{   // 最多4个子树
    struct ast_node * node = new struct ast_node();
    node->type = type;
    node->name = str;
    node->parent = nullptr;
    node->lineno = line_no;
    node->sons.push_back(first_son);
    first_son->parent = node;
    if(second_son != nullptr) {
        node->sons.push_back(second_son);
        second_son->parent = node;
    }
    if(third_son != nullptr) {
        node->sons.push_back(third_son);
        third_son->parent = node;
    }
    if(forth_son != nullptr) {
        node->sons.push_back(forth_son);
        forth_son->parent = node;
    }
    return node;
}
// 新叶子结点
struct ast_node * new_ast_leaf_node(struct ast_leaf_node_val &val,string str,int no)
{
    struct ast_node * node = new struct ast_node();
    node->type = val.kind;
    node->parent = nullptr;
    node->val = val;
    node->name = str;
    node->lineno = no;
    return node;
}