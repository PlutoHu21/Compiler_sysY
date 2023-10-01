#ifndef _BLOCK_
#define _BLOCK_
#include <string>
#include <vector>
#include "dfs.h"
using namespace std;

// 基本块
struct block
{
    struct ir_struct *Label;    // 表示标签名
    list block_ir;              // 表示语句体
    struct ir_struct *Jump;     // 表示跳转
};
// 为各个基本块之间建立有向图
struct graph_node{
    int index;                          // 索引
    block *block;                       // 语句块
    vector<graph_node*> next_nodes;     // 后继节点
    vector<graph_node*> front_nodes;    // 前驱节点
    int flag;                           // 0为开始，1为结束节点
};
//每个函数之间的流图
using block_graph=vector<graph_node*> ;
extern vector<block_graph> all_block_graph;//一个程序的所有流图
using fun_block_list=vector<block*>;//一个函数所有的基本块
extern vector<fun_block_list> block_ir_list;//一个程序各个函数的基本块集合
void basic_block();
void pass_block();
#endif