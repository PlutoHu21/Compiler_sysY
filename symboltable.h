#ifndef _SYMBOL_H
#define _SYMBOL_H

#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <stack>
#include <vector>
#include <iomanip>

using namespace std;

//表示该标识符是数组还是单变量 func表示声明的函数,funr表示调用函数
typedef  enum
{
    single,array,func,funr,other
}Gramma_type;

// 用来表示符号表中的每一项的具体信息
struct information
{
    int kind_type;              //表示类型,表示是数组还是单变量
    string value_name;          //储存其值的局部或全局变量
    vector<string>val_array;    //用来存储变量为数组的特殊情况
    int address;                //地址
    int varscope;               //层数，用于实现变量分层管理
};

// 用来表示每一张符号表
struct symbol_table
{
    map<string,information*> table;     // 符号表
    symbol_table *pre_env=nullptr;      // 指向上一张符号表的指针
};

//用栈来表示嵌套结构
extern stack<symbol_table*> stack_table;
//有一张全局符号表
extern multimap<string,information*> all_table;
//向符号表中插入一项
void symbol_table_insert(symbol_table *nowtable,string name,struct information *information,int scope);
//在符号表中进行查询，返回标识符的信息
struct information* symbol_talbe_lookup(symbol_table *nowtable,string name);
//打印符号表
void print_symbol_talbe_to_file(string file);
//用来初始化符号表栈
void stack_table_init();

#endif