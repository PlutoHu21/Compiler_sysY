#include "ast.h"
#include "symboltable.h"

// 用栈来表示嵌套结构
stack<symbol_table*> stack_table;
// 所有符号表
multimap<string,information*> all_table;

// 对栈进行初始化
void stack_table_init()
{
    struct symbol_table *global=new struct symbol_table();
    stack_table.push(global);
}
// 对符号表进行插入操作
void symbol_table_insert(struct symbol_table *nowtable,string name,struct information *information,int scope)
{
    information->varscope=scope;
    (nowtable->table).insert(make_pair(name,information));
    all_table.insert(make_pair(name,information));
}
// 对符号表进行查询操作
 struct information *symbol_talbe_lookup(symbol_table *nowtable,string name)
 {
     if ( nowtable==nullptr)
         return nullptr;
     else {
        auto pr=nowtable->table.find(name);
        if(pr!=nowtable->table.end())
            return pr->second;
        else
            return symbol_talbe_lookup(nowtable->pre_env,name);
     }
 }
// 打印符号表
// 格式为 name type value scope
void print_symbol_talbe_to_file(string file)
{
    ofstream out(file);
    if (out.is_open())
    {
        string type;    // 符号类型
        out << setw(10) << "name" << setw(10) << "type" << setw(10) << "value" << setw(10) << "scope" << endl;
        for(auto iter = all_table.rbegin(); iter != all_table.rend(); iter++){
            if(iter->second->kind_type == 0)            {type = "var";}         // 单变量
                else if(iter->second->kind_type == 1)   {type = "array";}       // 数组变量
            else if(iter->second->kind_type == 2)       {type = "func";}        // 函数
            out << setw(10) << iter->first << setw(10) << type << setw(10) 
            << iter->second->value_name << setw(10) << iter->second->varscope << endl;
        }
    }
    out.close();
    system(file.c_str());   // 展示符号表
}