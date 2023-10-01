#ifndef _DFS_H_
#define _DFS_H_

#include <string>
#include <vector>
#include "ast.h"

using namespace std;

// 中间IR语句
struct ir_struct
{
    TokenType op;           // 运算符种类
    string opstring;        // 运算符
    string src1;            // 参数2
    string src2;            // 参数2
    string res;             // 结果
    int line_no;            // 表示行号
    // 初始化
    ir_struct() {op=Null_;}
    ir_struct(TokenType t) {op=t;}
    ir_struct(string o,TokenType t,string l,string r,string c,int no) {opstring=o,op=t;src1=l;src2=r;res=c;line_no=no;}
};
// if循环的标签
struct if_label_struct
{
    string if_true;     // 条件为真
    string if_false;    // 条件为假
};
// while循环的标签
struct while_label_struct
{
    string entry;       // 循环的入口
    string next;        // 循环的出口
    string loop;        // 循环体的入口
};
// 其他标签
struct label_struct
{
    int lno;                // 表示行号
    vector<int>use_list;    // 表示使用标签的语句
};

using list=vector<ir_struct*>;      //IR语句数组
extern vector<list>ir_lists;        //所有生成的四元式
extern map<int,ir_struct*>ir_map;   //为每个ir语句建立哈希表，存储的是该ir语句的行号和对应的四元式
extern map<string,label_struct*>label_map;  //为每个label建立一个哈希表，存储的是每个.L的信息
void print_to_file(struct ast_node *root);  // 根据命令行参数选择生成抽象语法树,符号表,四元式,中间IR,控制流图

#endif