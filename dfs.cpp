#include <iostream>
#include <fstream>
#include "dfs.h"
#include "symboltable.h"
#include "block.h"

extern string outputPath;           // 输出文件名
extern bool printIrToFile;          // 打印IR
extern bool printSTToFile;          // 打印符号表
extern bool printQuadrupleToFile;   // 打印四元式
extern bool printASTToFile;         // 打印抽象语法树
extern bool printCFGToFile;         // 打印控制流图

//访问抽象语法树结点
string ast_visit_node(struct ast_node * node);
//表示作用域，0为全局变量，值越大代表嵌套层级越深
int scope=0;
//用来保存新的声明语句
vector<string>fun_declares;
vector<string>fun_names;
//用来存储四元式的结构体
map<int,ir_struct*>ir_map;
//用来存储一般IR
vector<list>ir_lists;
//用来存储声明IR
vector<list>ir_alloc_list;
//用来匹配最近的while语句
stack<while_label_struct*>while_list;
//用来存储所有的label
map<string,label_struct*>label_map;

int line_no_ir=1;   // IR行号
int temp_n;         // 临时变量编号
//创建新的临时变量，生成一个新的寄存器
string newtemp()
{
    string ir;
    ir="%t";
    ir+=to_string(temp_n);
    return ir;
}
int is_need_return_label;   //用来表示是否打return语句前的那个标签
int isglobal;   //是否为全局变量
int global_n=1; // 全局变量编号
//创建新的全局变量
string newglobal()
{
    string ir;
    ir="@g";
    ir+=to_string(global_n);
    return ir;
}
int local_n;    // 局部变量编号
//创建新的局部变量
string newlocal()
{
    string ir;
    ir="%l";
    ir+=to_string(local_n);
    return ir;
}
int label_n=2;  // 标签编号
string return_label;    //跳转到return语句的编号
//创建新的标签
string newlabel()
{
    string ir;
    ir=".L";
    ir+=to_string(label_n);
    struct label_struct *label_info=new struct label_struct();
    label_map.insert(make_pair(ir,label_info));
    return ir;
}
//用一个栈来表示if的嵌套层级，
//用来生成新的变量,fun_declares中最后一个元素肯定是当前的函数的声明语句，全局变量永远在第一个
void newdeclare(string type,string var)
{
    string ir="declare "+type+" "+var;
    fun_declares[fun_declares.size()-1]=fun_declares[fun_declares.size()-1]+ir+"\n";
    char c=var[1];
    if(c=='l'||c=='g') {
        struct ir_struct *ir1=new struct ir_struct("alloc",Alloc_,type,var,"NULL",line_no_ir);
        line_no_ir++;
        ir_alloc_list[ir_alloc_list.size()-1].push_back(ir1);
    }
}
string lval_ir(struct ast_node *src1_node);
string lval_array_ir(struct ast_node *src1_node,struct ast_node *src2_node);
// emit函数用来翻译四元式
// 根据type生成IR
string emit(int type,string ir_left,string ir_right,string result)
{
    string ir;
    switch(type) {
        case ADD_: {
            ir = result + "=" + "add " + ir_left + "," + ir_right;
            return ir;
        }
        case ASSIGN_: {
            ir = result + "=" + ir_left;
            return ir;
        }
        case Load_:
        {
            ir = result + "=" + ir_left;
            return ir;
        }
        case Store_:
        {
            ir = result + "=" + ir_left;
            return ir;
        }
        case MUL_: {
            ir = result + "=" + "mul " + ir_left + "," + ir_right;
            return ir;
        }
        case DIV_: {
            ir = result + "=" + "div " + ir_left + "," + ir_right;
            return ir;
        }
        case SUB_: {
            ir = result + "=" + "sub " + ir_left + "," + ir_right;
            return ir;
        }
        case MOD_: {
            ir = result + "=" + "mod " + ir_left + "," + ir_right;
            return ir;
        }
        case Entry_: {
            ir = "entry";
            return ir;
        }
        case Exit_: {
            ir = "exit " + ir_left + "\n}";
            return ir;
        }
        case NEG_: {
            ir = result + "=" + "neg " + ir_left;
            return ir;
        }
        case POS_: {
            ir = result + "=" + "pos " + ir_left;
            return ir;
        }
        case Call_: {
            ir = result + "=call i32 @" + ir_left + "(" + ir_right + ")";
            return ir;
        }
        case Call_Void: {
            ir = "call void @" + ir_left + "(" + ir_right + ")";
            return ir;
        }
        case INT_: {
            fun_names.push_back(ir_left);
            ir = "define i32 @" + ir_left + "(" + ir_right + "){\n";
            return ir;
        }
        case VOID_: {
            fun_names.push_back(ir_left);
            ir = "define void @" + ir_left + "(" + ir_right + "){\n";
            return ir;
        }
        case GT_: {
            ir = result + "=" + "cmp gt " + ir_left + ", " + ir_right;
            return ir;
        }
        case GE_: {
            ir = result + "=" + "cmp ge " + ir_left + ", " + ir_right;
            return ir;
        }
        case LT_: {
            ir = result + "=" + "cmp lt " + ir_left + ", " + ir_right;
            return ir;
        }
        case LE_: {
            ir = result + "=" + "cmp le " + ir_left + ", " + ir_right;
            return ir;
        }
        case NE_: {
            ir = result + "=" + "cmp ne " + ir_left + ", " + ir_right;
            return ir;
        }
        case EQ_: {
            ir = result + "=" + "cmp eq " + ir_left + ", " + ir_right;
            return ir;
        }
        case Label_: {
            ir=ir_left+":";
            return ir;
        }
        case Br_: {
            ir="br label "+ir_left;
            return ir;
        }
        case Bc_: {
            ir="bc "+result+", label "+ir_left+", label "+ir_right;
            return ir;
        }
    }
    return ir;
}
//对声明进行解析
string vardecl_ir(struct ast_node *src1_node,struct  ast_node *src2_node)
{
    string this_ir;
    string left_ir= ast_visit_node(src1_node);
    string right_ir= ast_visit_node(src2_node);
    this_ir+=left_ir;
    return this_ir;
}
//对常量进行解析
string constdecl_ir(struct ast_node *src1_node,struct  ast_node *src2_node)
{
    string this_ir;
    string left_ir= ast_visit_node(src1_node);
    string right_ir= ast_visit_node(src2_node);
    this_ir+=left_ir;
    return this_ir;
}
//对声明时就进行初始化的变量进行解析
string constdef_single_ir(struct ast_node *src1_node,struct ast_node*src2_node)
{
    src1_node->kind_type=single;
    struct information *info=new struct information;
    info->kind_type=src1_node->kind_type;
    info->value_name="NULL";
    symbol_table_insert(stack_table.top(),src1_node->val.ident,info,scope);
    string left_ir= ast_visit_node(src1_node);
    string right_ir= ast_visit_node(src2_node);
    //如果是全局变量的话就加到fun_declares[0]中 ，如果不是就加到对应的函数声明语句中
    if(isglobal==0) {
        newdeclare("i32", left_ir);
        struct ir_struct *ir=new struct ir_struct("load",Load_,right_ir,"NULL",left_ir,line_no_ir);
        ir_map.insert(make_pair(line_no_ir,ir));
        line_no_ir++;
        ir_lists[ir_lists.size()-1].push_back(ir);
    } else {
        //声明全局变量
        fun_declares[0] = fun_declares[0] + "declare i32 " + left_ir +"=" +right_ir+"\n";
        struct ir_struct *ir1=new struct ir_struct("alloc",Alloc_,"i32",left_ir,"NULL",line_no_ir);
        ir_map.insert(make_pair(line_no_ir,ir1));
        line_no_ir++;
        ir_alloc_list[0].push_back(ir1);
    }
    return "";
}
void vardef_single_init_ir(struct ast_node *src1_node,struct ast_node*src2_node)
{
    src1_node->kind_type=single;
    struct information *info=new struct information;
    info->kind_type=src1_node->kind_type;
    info->value_name="NULL";
    symbol_table_insert(stack_table.top(),src1_node->val.ident,info,scope);
    string left_ir= ast_visit_node(src1_node);
    string right_ir= ast_visit_node(src2_node);
    //如果是全局变量的话就加到fun_declares[0]中 ，如果不是就加到对应的函数声明语句中
    if(isglobal==0) {
        newdeclare("i32", left_ir);
        struct ir_struct *ir=new struct ir_struct("store",Store_,right_ir,"NULL",left_ir,line_no_ir);
        ir_map.insert(make_pair(line_no_ir,ir));
        line_no_ir++;
        ir_lists[ir_lists.size()-1].push_back(ir);
    } else {
        //声明全局变量
        fun_declares[0] = fun_declares[0] + "declare i32 " + left_ir +"=" +right_ir+"\n";
        struct ir_struct *ir1=new struct ir_struct("alloc",Alloc_,"i32",left_ir,"NULL",line_no_ir);
        ir_alloc_list[0].push_back(ir1);
        ir_map.insert(make_pair(line_no_ir,ir1));
        line_no_ir++;
    }
}
//对只声明没有初始化的变量进行解析
void vardef_single_ir(struct ast_node *src1_node)
{
    src1_node->kind_type=single;
    struct information *info=new struct information;
    info->kind_type=src1_node->kind_type;
    info->value_name="NULL";
    symbol_table_insert(stack_table.top(),src1_node->val.ident,info,scope);
    string left_ir= ast_visit_node(src1_node);
    if(isglobal==0) {
        newdeclare("i32", left_ir);
    } else {
        fun_declares[0] = fun_declares[0] + "declare i32" + left_ir + "\n";
        struct ir_struct *ir1=new struct ir_struct("alloc",Alloc_,"i32",left_ir,"NULL",line_no_ir);
        ir_alloc_list[0].push_back(ir1);
        ir_map.insert(make_pair(line_no_ir,ir1));
        line_no_ir++;
    }
}
void  vardef_array_ir(struct ast_node *src1_node,struct ast_node *src2_node)
{
    src1_node->kind_type=Gramma_type::array;
    struct information *info=new struct information;
    info->kind_type=src1_node->kind_type;
    info->value_name="NULL";
    symbol_table_insert(stack_table.top(),src1_node->val.ident,info,scope);
    string left_ir= ast_visit_node(src1_node);
    string right_ir= ast_visit_node(src2_node);
    info->val_array.push_back("4");
    left_ir=left_ir+right_ir;
    if(isglobal==0) {
        newdeclare("i32", left_ir);
    } else {
        fun_declares[0] = fun_declares[0] + "declare i32" + left_ir + "\n";
        struct ir_struct *ir1=new struct ir_struct("alloc",Alloc_,"i32",left_ir,"NULL",line_no_ir);
        ir_alloc_list[0].push_back(ir1);
        ir_map.insert(make_pair(line_no_ir,ir1));
        line_no_ir++;
    }
}
string  initvals_ir(struct ast_node *node)
{
    auto info=symbol_talbe_lookup(stack_table.top(),node->parent->sons[0]->val.ident);
    string base=info->value_name;
    if(node->sons.size()==0) {
        int num;
        if(info->val_array.size()==2) {
            num= atoi(info->val_array[0].c_str());
        } else if(info->val_array.size()==3) {
            num=atoi(info->val_array[0].c_str())* atoi(info->val_array[1].c_str());
        }
        for(int i=0;i<num;i++) {
            string offset= to_string(4*i);
            string addr;
            if(isglobal) {
                addr=newglobal();
                global_n++;
            } else {
                 addr=newtemp();
                temp_n++;
            }
            newdeclare("i32*",addr);
            struct ir_struct *ir1 = new struct ir_struct("add", ADD_, base, offset, addr,line_no_ir);
            ir_lists[ir_lists.size() - 1].push_back(ir1);
            ir_map.insert(make_pair(line_no_ir,ir1));
            line_no_ir++;
            struct ir_struct*ir2=new struct ir_struct("store",Store_,"0","NULL","*"+addr,line_no_ir);
            ir_lists[ir_lists.size() - 1].push_back(ir2);
            ir_map.insert(make_pair(line_no_ir,ir2));
            line_no_ir++;
        }
    } else {
        if(info->val_array.size()==2) {
            for (int i = 0; i < node->sons.size(); i++) {
                //a[3]={1,2,3}
                node->sons[i]->parent=node;
                string number = ast_visit_node(node->sons[i]);//返回的是初始化的数据
                string offset = to_string(4 * i);
                string addr;
                if(isglobal) {
                    addr=newglobal();
                    global_n++;
                } else {
                     addr=newtemp();
                    temp_n++;
                }
                newdeclare("i32*", addr);
                struct ir_struct *ir1 = new struct ir_struct("add", ADD_, base, offset, addr,line_no_ir);
                ir_lists[ir_lists.size() - 1].push_back(ir1);
                ir_map.insert(make_pair(line_no_ir,ir1));
                line_no_ir++;
                struct ir_struct *ir2 = new struct ir_struct("store", Store_, number, "NULL", "*" + addr,line_no_ir);
                ir_lists[ir_lists.size() - 1].push_back(ir2);
                ir_map.insert(make_pair(line_no_ir,ir2));
                line_no_ir++;
            }
        } else if(info->val_array.size()==3) {
            int start;
            int num=0;//表示到第几个元素
            for(int i=0;i<node->sons.size();i++) {
                if(node->sons[i]->type==InitVals_) {
                    int j;
                    //计算大括号中已有的数值
                    for(j=0;j<node->sons[i]->sons.size();j++)
                    {
                        node->sons[i]->sons[j]->parent=node->sons[i];
                        node->sons[i]->parent=node;
                        string number= ast_visit_node(node->sons[i]->sons[j]);
                        string offset= to_string(num*4);
                        string addr;
                        if(isglobal) {
                            addr=newglobal();
                            global_n++;
                        } else {
                             addr=newtemp();
                            temp_n++;
                        }
                        newdeclare("i32*", addr);
                        struct ir_struct *ir1 = new struct ir_struct("add", ADD_, base, offset, addr,line_no_ir);
                        ir_lists[ir_lists.size() - 1].push_back(ir1);
                        ir_map.insert(make_pair(line_no_ir,ir1));
                        line_no_ir++;
                        struct ir_struct *ir2 = new struct ir_struct("store", Store_, number, "NULL", "*" + addr,line_no_ir);
                        ir_lists[ir_lists.size() - 1].push_back(ir2);
                        ir_map.insert(make_pair(line_no_ir,ir2));
                        line_no_ir++;
                        num++;
                    }
                    //补全0
                    for(;j<atoi(info->val_array[1].c_str());j++) {
                        string offset= to_string(num*4);
                        string addr;
                        if(isglobal) {
                            addr=newglobal();
                            global_n++;
                        } else {
                            addr=newtemp();
                            temp_n++;
                        }
                        newdeclare("i32*",addr);
                        struct ir_struct *ir1 = new struct ir_struct("add", ADD_, base, offset, addr,line_no_ir);
                        ir_lists[ir_lists.size() - 1].push_back(ir1);
                        ir_map.insert(make_pair(line_no_ir,ir1));
                        line_no_ir++;
                        struct ir_struct*ir2=new struct ir_struct("store",Store_,"0","NULL","*"+addr,line_no_ir);
                        ir_lists[ir_lists.size() - 1].push_back(ir2);
                        ir_map.insert(make_pair(line_no_ir,ir2));
                        line_no_ir++;
                        num++;
                    }
                } else {
                    node->sons[i]->parent=node;
                    string number = ast_visit_node(node->sons[i]);//返回的是初始化的数据
                    string offset = to_string(4 * num);
                    string addr;
                    if(isglobal) {
                        addr=newglobal();
                        global_n++;
                    } else {
                         addr=newtemp();
                        temp_n++;
                    }
                    newdeclare("i32*", addr);
                    struct ir_struct *ir1 = new struct ir_struct("add", ADD_, base, offset, addr,line_no_ir);
                    ir_lists[ir_lists.size() - 1].push_back(ir1);
                    ir_map.insert(make_pair(line_no_ir,ir1));
                    line_no_ir++;
                    struct ir_struct *ir2 = new struct ir_struct("store", Store_, number, "NULL", "*" + addr,line_no_ir);
                    ir_lists[ir_lists.size() - 1].push_back(ir2);
                    ir_map.insert(make_pair(line_no_ir,ir2));
                    line_no_ir++;
                    num++;
                }
            }
        }
    }
    return "";
}
//处理数组初始化结果
void vardef_array_init(struct ast_node *src1_node,struct ast_node *src2_node,struct ast_node *src3_node )
{
    src1_node->kind_type=Gramma_type::array;
    struct information *info=new struct information;
    info->kind_type=src1_node->kind_type;
    info->value_name="NULL";
    symbol_table_insert(stack_table.top(),src1_node->val.ident,info,scope);
    string left_ir= ast_visit_node(src1_node);
    string right_ir= ast_visit_node(src2_node);
    info->val_array.push_back("4");
    left_ir=left_ir+right_ir;
    if(isglobal==0) {
        newdeclare("i32", left_ir);
    } else {
        fun_declares[0] = fun_declares[0] + "declare i32" + left_ir + "\n";
        struct ir_struct *ir1=new struct ir_struct("alloc",Alloc_,"i32",left_ir,"NULL",line_no_ir);
        ir_alloc_list[0].push_back(ir1);
        ir_map.insert(make_pair(line_no_ir,ir1));
        line_no_ir++;
    }
    string temp= ast_visit_node(src3_node);
}
//处理const数组初始化
string constdef_array_ir(struct ast_node *src1_node,struct ast_node *src2_node,struct ast_node *src3_node )
{
    src1_node->kind_type=Gramma_type::array;
    struct information *info=new struct information;
    info->kind_type=src1_node->kind_type;
    info->value_name="NULL";
    symbol_table_insert(stack_table.top(),src1_node->val.ident,info,scope);
    string left_ir= ast_visit_node(src1_node);
    string right_ir= ast_visit_node(src2_node);
    info->val_array.push_back("4");
    left_ir=left_ir+right_ir;
    if(isglobal==0) {
        newdeclare("i32", left_ir);
    } else {
        fun_declares[0] = fun_declares[0] + "declare i32" + left_ir + "\n";
        struct ir_struct *ir1=new struct ir_struct("alloc",Alloc_,"i32",left_ir,"NULL",line_no_ir);
        ir_alloc_list[0].push_back(ir1);
        ir_map.insert(make_pair(line_no_ir,ir1));
        line_no_ir++;
    }
    string temp= ast_visit_node(src3_node);
    return "";
}
//过渡节点，选择对应的解析函数
void vardefs_ir(struct ast_node*node)
{
    for(auto son:node->sons) {
        if(son->type==VarDef_single_init_) {
            vardef_single_init_ir(son->sons[0],son->sons[1]);
        } else if(son->type==VarDef_single_) {
            vardef_single_ir(son->sons[0]);
        } else if(son->type==VarDef_array_) {
            vardef_array_ir(son->sons[0],son->sons[1]);
        } else if(son->type==VarDef_array_init_) {
            vardef_array_init(son->sons[0],son->sons[1],son->sons[2]);
        }
    }
}
// 解析常量表达式
string constexps_ir(struct ast_node *node)
{
    string this_ir;
    auto info=symbol_talbe_lookup(stack_table.top(),node->parent->sons[0]->val.ident);
    for(int i=0;i<node->sons.size();i++) {
        string number= ast_visit_node(node->sons[i]);
        this_ir=this_ir+"["+number+"]";
        info->val_array.push_back(number);
    }
    return this_ir;
}
//解析具有函数返回值的函数
string funcdef_int_ir(struct ast_node *src1_node,struct ast_node* src2_node,struct ast_node* src3_node)
{
    string this_ir;
    src2_node->kind_type=func;
    struct information *info=new struct information();
    info->kind_type=src2_node->kind_type;
    info->value_name="NULL";
    symbol_table_insert(stack_table.top(),src2_node->val.ident,info,scope);
    //函数出口库所在的标签
    return_label=newlabel();
    label_n++;
    string middle= ast_visit_node(src2_node);
    struct ir_struct *ir1=new struct ir_struct("INT",INT_,middle,"","NULL",line_no_ir);
    string ir=emit(ir1->op,ir1->src1,ir1->src2,ir1->res);
    ir_map.insert(make_pair(line_no_ir,ir1));
    line_no_ir++;
    (fun_declares[fun_declares.size()-1]).insert(0,ir);
    //打印入口
    struct ir_struct*ir2=new struct ir_struct("entry",Entry_,"NULL","NULL","NULL",line_no_ir);
    ir_map.insert(make_pair(line_no_ir,ir2));
    line_no_ir++;
    src1_node->kind_type=func;
    this_ir+= ast_visit_node(src1_node);
    ir_lists[ir_lists.size()-1].push_back(ir2);
    this_ir= ast_visit_node(src3_node);
    //打印出口
    if(is_need_return_label) {
        struct ir_struct *ir6 = new ir_struct("br", Br_, return_label, "NULL", "NULL",line_no_ir);
        ir_lists[ir_lists.size() - 1].push_back(ir6);
        ir_map.insert(make_pair(line_no_ir,ir6));
        line_no_ir++;
        //打印.L1
        auto label_info=label_map.find(return_label)->second;
        label_info->lno=line_no_ir;
        struct ir_struct *ir3 = new ir_struct("Label", Label_, return_label, "NULL", "NULL",line_no_ir);
        ir_lists[ir_lists.size() - 1].push_back(ir3);
        ir_map.insert(make_pair(line_no_ir,ir3));
        line_no_ir++;
    }
    //打印%t=?
    string temp=newtemp();
    newdeclare("i32",temp);
    struct ir_struct *ir_1=new struct ir_struct("load",Load_,"%l0","NULL",temp,line_no_ir);//%tx=%l0
    ir_lists[ir_lists.size()-1].push_back(ir_1);
    ir_map.insert(make_pair(line_no_ir,ir_1));
    line_no_ir++;
    //打印exit
    struct ir_struct*ir_2=new struct ir_struct("exit",Exit_,temp,"NULL","NULL",line_no_ir);
    ir_lists[ir_lists.size()-1].push_back(ir_2);
    ir_map.insert(make_pair(line_no_ir,ir_2));
    line_no_ir++;
    return temp;
}
//解析int型，带有参数的函数
string funcdef_int_para_ir(struct ast_node *node)
{
    string this_ir;
    node->sons[1]->kind_type=func;
    node->sons[0]->kind_type=func;
    struct information *info=new struct information;
    info->kind_type=node->sons[1]->kind_type;
    info->value_name="NULL";
    //函数出口库所在的标签
    return_label=newlabel();
    label_n++;
    symbol_table_insert(stack_table.top(),node->sons[1]->val.ident,info,scope);
    string second_son= ast_visit_node(node->sons[1]);
    string first_son= ast_visit_node(node->sons[0]);
    //入口
    struct ir_struct*ir1=new struct ir_struct("entry",Entry_,"NULL","NULL","NULL",line_no_ir);
    ir_lists[ir_lists.size()-1].push_back(ir1);
    ir_map.insert(make_pair(line_no_ir,ir1));
    line_no_ir++;
    struct symbol_table *nowtable=new struct symbol_table();
    nowtable->pre_env=stack_table.top();
    stack_table.push(nowtable);
    scope++;
    string third_son= ast_visit_node(node->sons[2]);
    //声明，结束在return 语句中
    struct ir_struct *ir2=new struct ir_struct("INT",INT_,second_son,third_son,"NULL",line_no_ir);
    ir_map.insert(make_pair(line_no_ir,ir2));
    line_no_ir++;
    string ir=emit(ir2->op,ir2->src1,ir2->src2,ir2->res);
    (fun_declares[fun_declares.size()-1]).insert(0,ir);
    string forth_son= ast_visit_node(node->sons[3]);
    //是否需要打印标签
    if(is_need_return_label) {
        struct ir_struct *ir6 = new ir_struct("br", Br_, return_label, "NULL", "NULL",line_no_ir);
        ir_lists[ir_lists.size() - 1].push_back(ir6);
        ir_map.insert(make_pair(line_no_ir,ir6));
        line_no_ir++;
        auto label_info=label_map.find(return_label)->second;
        label_info->lno=line_no_ir;
        struct ir_struct *ir3 = new ir_struct("Label", Label_, return_label, "NULL", "NULL",line_no_ir);
        ir_lists[ir_lists.size() - 1].push_back(ir3);
        ir_map.insert(make_pair(line_no_ir,ir3));
        line_no_ir++;
    }
    string temp=newtemp();
    newdeclare("i32",temp);
    struct ir_struct *ir_1=new struct ir_struct("load",Load_,"%l0","NULL",temp,line_no_ir);//%tx=%l0
    ir_lists[ir_lists.size()-1].push_back(ir_1);
    ir_map.insert(make_pair(line_no_ir,ir_1));
    line_no_ir++;
    struct ir_struct*ir_2=new struct ir_struct("exit",Exit_,temp,"NULL","NULL",line_no_ir);
    ir_lists[ir_lists.size()-1].push_back(ir_2);
    ir_map.insert(make_pair(line_no_ir,ir_2));
    line_no_ir++;
    return temp;
}
// 解析一个函数的单变量形参
string funcfparam_single_ir(struct ast_node *node)
{
    node->sons[0]->kind_type=single;
    struct information *info=new struct information;
    info->kind_type=node->sons[0]->kind_type;
    info->value_name="NULL";
    symbol_table_insert(stack_table.top(),node->sons[0]->val.ident,info,scope);
    string ir_left= ast_visit_node(node->sons[0]);
    newdeclare("i32",ir_left);
    string temp=newtemp();
    // newdeclare("i32",temp);
    struct ir_struct *ir=new struct ir_struct("=",ASSIGN_,temp,"NULL",ir_left,line_no_ir);
    ir_lists[ir_lists.size()-1].push_back(ir);
    ir_map.insert(make_pair(line_no_ir,ir));
    line_no_ir++;
    temp_n++;
    string define_ir="i32 "+temp;
    return define_ir;   // 返回临时变量 %t
}
// 解析函数的数组形参
string funcfparam_array_ir(struct ast_node *node)
{
    //插入符号表中
    node->sons[0]->kind_type=Gramma_type::array;
    struct information *info=new struct information;
    info->kind_type=node->sons[0]->kind_type;
    info->value_name="NULL";
    symbol_table_insert(stack_table.top(),node->sons[0]->val.ident,info,scope);
    //获取名称,参数，实现声明
    string left_ir= ast_visit_node(node->sons[0]);
    info->val_array.push_back("0");
    info->val_array.push_back("4");
    newdeclare("i32",left_ir+"[0]");
    //%l1=%t1
    string temp=newtemp();
    temp_n++;
    //待修改
    struct ir_struct *ir=new struct ir_struct("=",ASSIGN_,temp,"NULL",left_ir,line_no_ir);
    ir_lists[ir_lists.size()-1].push_back(ir);
    ir_map.insert(make_pair(line_no_ir,ir));
    line_no_ir++;
    //i32 %t1[0]
    temp=temp+"[0]";
    string define_ir="i32 "+temp;
    return define_ir;//返回的是临时变量 %t
}
// 解析函数的参数节点
string funcfparam_ir(struct ast_node *node)
{
    string this_ir;
    for(int i=0;i<node->sons.size();i++)
    {   //解析
        if(node->sons[i]->type==FuncFParam_single_) {
            this_ir += ast_visit_node(node->sons[i]);//返回的是%t
            if (i!=node->sons.size()-1)
                this_ir += ",";
            else
                this_ir+="";
        } else if(node->sons[i]->type==FuncFParam_array_) {
            this_ir += ast_visit_node(node->sons[i]);//返回的是参数名称
            if (i!=node->sons.size()-1)
                this_ir += ",";
            else
                this_ir+="";
        }
    }
    return this_ir;//返回的是参数列表
}
//解析没有函数返回值的函数
string funcdef_void_ir(struct ast_node *src1_node,struct ast_node* src2_node,struct ast_node* src3_node)
{
    string this_ir;
    return_label=newlabel();
    label_n++;
    struct symbol_table *nowtable=new struct symbol_table();
    nowtable->pre_env=stack_table.top();
    stack_table.push(nowtable);
    scope++;
    src2_node->kind_type=func;
    src1_node->kind_type=func;
    struct information *info=new struct information;
    info->kind_type=src2_node->kind_type;
    info->value_name="NULL";
    symbol_table_insert(stack_table.top(),src2_node->val.ident,info,scope);
    string second_son= ast_visit_node(src2_node);
    string first_son= ast_visit_node(src1_node);
    //入口语句
    struct ir_struct*ir1=new struct ir_struct("entry",Entry_,"NULL","NULL","NULL",line_no_ir);
    ir_lists[ir_lists.size()-1].push_back(ir1);
    ir_map.insert(make_pair(line_no_ir,ir1));
    line_no_ir++;
    //声明语句
    struct ir_struct *ir2=new struct ir_struct("VOID",VOID_,second_son,"","NULL",line_no_ir);
    ir_map.insert(make_pair(line_no_ir,ir2));
    line_no_ir++;
    string ir=emit(ir2->op,ir2->src1,ir2->src2,ir2->res);
    (fun_declares[fun_declares.size()-1]).insert(0,ir);
    (ir_lists[ir_lists.size()-1]).insert(ir_lists[ir_lists.size()-1].begin(),ir2);
    //结束语句
    auto label_info=label_map.find(return_label)->second;
    label_info->lno=line_no_ir;
    struct ir_struct *ir3 = new ir_struct("Label", Label_, return_label, "NULL", "NULL",line_no_ir);
    ir_lists[ir_lists.size()-1].push_back(ir3);
    ir_map.insert(make_pair(line_no_ir,ir3));
    line_no_ir++;
    string ir4= ast_visit_node(src3_node);//进行块遍历
    struct ir_struct*ir5=new struct ir_struct("exit",Exit_,"","","",line_no_ir);
    ir_lists[ir_lists.size()-1].push_back(ir5);
    ir_map.insert(make_pair(line_no_ir,ir5));
    line_no_ir++;
    return this_ir;
}
//解析没有返回值，带有参数的函数
string funcdef_void_para_ir(struct ast_node *node)
{
    string this_ir;
    return_label=newlabel();
    label_n++;
    struct symbol_table *nowtable=new struct symbol_table();
    nowtable->pre_env=stack_table.top();
    stack_table.push(nowtable);
    scope++;
    node->sons[1]->kind_type=func;
    node->sons[0]->kind_type=func;
    struct information *info=new struct information;
    info->kind_type=node->sons[1]->kind_type;
    info->value_name="NULL";
    symbol_table_insert(stack_table.top(),node->sons[1]->val.ident,info,scope);
    string second_son= ast_visit_node(node->sons[1]);
    string first_son= ast_visit_node(node->sons[0]);
    //入口
    struct ir_struct*ir1=new struct ir_struct("entry",Entry_,"NULL","NULL","NULL",line_no_ir);
    ir_lists[ir_lists.size()-1].push_back(ir1);
    ir_map.insert(make_pair(line_no_ir,ir1));
    line_no_ir++;
    string third_son= ast_visit_node(node->sons[2]);
    //声明
    struct ir_struct *ir2=new struct ir_struct("VOID",VOID_,second_son,third_son,"NULL",line_no_ir);
    ir_map.insert(make_pair(line_no_ir,ir2));
    line_no_ir++;
    string ir=emit(ir2->op,ir2->src1,ir2->src2,ir2->res);
    (fun_declares[fun_declares.size()-1]).insert(0,ir);
    string forth_son= ast_visit_node(node->sons[3]);
    //结束
    auto label_info=label_map.find(return_label)->second;
    label_info->lno=line_no_ir;
    struct ir_struct *ir4 = new ir_struct("Label", Label_, return_label, "NULL", "NULL",line_no_ir);
    ir_lists[ir_lists.size()-1].push_back(ir4);
    ir_map.insert(make_pair(line_no_ir,ir4));
    line_no_ir++;
    struct ir_struct*ir3=new struct ir_struct("exit",Exit_,"","","",line_no_ir);
    ir_lists[ir_lists.size()-1].push_back(ir3);
    ir_map.insert(make_pair(line_no_ir,ir3));
    line_no_ir++;
    return this_ir;
}
//解析语句块的函数，同时如果遇到新的块，就压栈，结束后进行弹栈
string block_ir(struct ast_node*src1_node,struct ast_node *node)
{
    string this_ir;//如果函数带参数，就在最开始创建新的表，否则碰到block再创建新的表
    if(node->type==FuncDef_int_para_||node->type==FuncDef_void_para_) {

    } else {
        struct symbol_table *nowtable=new struct symbol_table();
        nowtable->pre_env=stack_table.top();
        stack_table.push(nowtable);
        scope++;
    }
    for(int i=0;i<src1_node->sons.size();i++) {

        string temp=ast_visit_node(src1_node->sons[i]);
        this_ir+=temp;
//        if(i==src1_node->sons.size()-1&&src1_node->sons[i]->type==Stmt_If_)
//        {
//            ir_lists[ir_lists.size()-1].pop_back();
//            ir_lists[ir_lists.size()-1].pop_back();
//            goto_list.push_back(atoi(temp.c_str()));
//            this_ir=temp;
//        }
        if(src1_node->sons[i]->type==Stmt_Return_) {
            this_ir="Return";
            break;
        }
    }
    stack_table.pop();
    scope--;
    return this_ir;
}
//stmt:expr:add 将结果存在的临时变量返回
string  expr_add_ir(struct ast_node *src1_node,struct ast_node* src2_node,struct ast_node* result_node)
{
    string this_ir;
    string ir_left= ast_visit_node(src1_node);
    string ir_right= ast_visit_node(src2_node);
    string temp=newtemp();
    newdeclare("i32",temp);
    struct ir_struct *ir=new struct ir_struct("add",ADD_,ir_left,ir_right,temp,line_no_ir);
    ir_lists[ir_lists.size()-1].push_back(ir);
    ir_map.insert(make_pair(line_no_ir,ir));
    line_no_ir++;
    temp_n++;
    return temp;
}
//stmt:expr:sub 将结果存在的临时变量返回
string  expr_sub_ir(struct ast_node *src1_node,struct ast_node* src2_node,struct ast_node* result_node)
{
    string this_ir;
    string ir_left= ast_visit_node(src1_node);
    string ir_right= ast_visit_node(src2_node);
    string temp=newtemp();
    newdeclare("i32",temp);
    struct ir_struct *ir=new struct ir_struct("sub",SUB_,ir_left,ir_right,temp,line_no_ir);
    ir_lists[ir_lists.size()-1].push_back(ir);
    ir_map.insert(make_pair(line_no_ir,ir));
    line_no_ir++;
    temp_n++;
    return temp;
}
//stmt:expr:mul将结果存在的临时变量返回
string expr_mul_ir(struct ast_node *src1_node,struct ast_node* src2_node,struct ast_node* result_node)
{
    string this_ir;
    string ir_left= ast_visit_node(src1_node);
    string ir_right= ast_visit_node(src2_node);
    string temp=newtemp();
    newdeclare("i32",temp);
    struct ir_struct *ir=new struct ir_struct("*",MUL_,ir_left,ir_right,temp,line_no_ir);
    ir_lists[ir_lists.size()-1].push_back(ir);
    ir_map.insert(make_pair(line_no_ir,ir));
    line_no_ir++;
    temp_n++;
    return temp;
}
//stmt:expr:div 将结果存在的临时变量返回
string expr_div_ir(struct ast_node *src1_node,struct ast_node* src2_node,struct ast_node* result_node)
{
    string ir_left= ast_visit_node(src1_node);
    string ir_right= ast_visit_node(src2_node);
    string temp=newtemp();
    newdeclare("i32",temp);
    struct ir_struct *ir=new struct ir_struct("\\",DIV_,ir_left,ir_right,temp,line_no_ir);
    ir_lists[ir_lists.size()-1].push_back(ir);
    ir_map.insert(make_pair(line_no_ir,ir));
    line_no_ir++;
    temp_n++;
    return temp;
}
//stmt:expr:mod 将结果存在的临时变量返回
string expr_mod_ir(struct ast_node *src1_node,struct ast_node* src2_node,struct ast_node* result_node)
{
    string ir_left= ast_visit_node(src1_node);
    string ir_right= ast_visit_node(src2_node);
    string temp=newtemp();
    newdeclare("i32",temp);
    struct ir_struct *ir=new struct ir_struct("%",MOD_,ir_left,ir_right,temp,line_no_ir);
    ir_lists[ir_lists.size()-1].push_back(ir);
    ir_map.insert(make_pair(line_no_ir,ir));
    line_no_ir++;
    temp_n++;
    return temp;
}
//stmt neg 取负
string stmt_neg_ir(struct  ast_node *src1_node)
{
    string ir_left= ast_visit_node(src1_node);//返回值为一个临时变量
    string temp=newtemp();
    newdeclare("i32",temp);
    if(src1_node->type==Number_) {
        struct ir_struct *ir=new struct ir_struct("load",Load_,"-"+ ir_left,"NULL",temp,line_no_ir);
        ir_lists[ir_lists.size()-1].push_back(ir);
        ir_map.insert(make_pair(line_no_ir,ir));
        line_no_ir++;
    } else {
        struct ir_struct *ir=new struct ir_struct("neg",NEG_,ir_left,ir_left,temp,line_no_ir);
        ir_lists[ir_lists.size()-1].push_back(ir);
        ir_map.insert(make_pair(line_no_ir,ir));
        line_no_ir++;
    }
    temp_n++;
    return temp;
}
//stmt pos
string stmt_pos_ir(struct  ast_node *src1_node)
{
    string ir_left= ast_visit_node(src1_node);//返回值为一个临时变量
    string temp=newtemp();
    newdeclare("i32",temp);
    struct ir_struct *ir=new struct ir_struct("pos",POS_,ir_left,ir_left,temp,line_no_ir);
    ir_lists[ir_lists.size()-1].push_back(ir);
    ir_map.insert(make_pair(line_no_ir,ir));
    line_no_ir++;
    temp_n++;
    return temp;
}
// stmt not 逻辑非
string stmt_not_ir(struct ast_node *src1_node)
{
    string ir_left= ast_visit_node(src1_node);//返回值为一个临时变量
    string temp=newtemp();
    temp_n++;
    newdeclare("i1",temp);//声明成i1类型
    struct ir_struct *ir=new ir_struct("==",EQ_,ir_left ,"0",temp,line_no_ir);
    ir_lists[ir_lists.size()-1].push_back(ir);
    ir_map.insert(make_pair(line_no_ir,ir));
    line_no_ir++;
    return temp;
}
//对函数的实参列表进行解析
string funcrparams_ir(struct ast_node *node)
{
    string this_ir;
    for(int i=0;i<node->sons.size();i++) {
        if (node->sons[i]->type == Number_) {
            string ir = ast_visit_node(node->sons[i]);
            if(i==node->sons.size()-1)
                this_ir=this_ir+"i32 "+ir;
            else
                this_ir = this_ir +"i32 "+ ir + ",";
        } else if(node->sons[i]->type==LVal_) {
            string ir;//处理一维数组和变量 一维数组指的是a，传的是数组的地址
            int kind= symbol_talbe_lookup(stack_table.top(),node->sons[i]->sons[0]->val.ident)->kind_type;
            if(i==node->sons.size()-1) {
                if(kind==Gramma_type::array) {
                    ir= ast_visit_node(node->sons[i]);
                    this_ir = this_ir + "i32* " + ir;
                } else {
                    ir= ast_visit_node(node->sons[i]);
                    this_ir = this_ir + "i32 " + ir;
                }
            } else {
                if(kind==Gramma_type::array) {
                    ir= ast_visit_node(node->sons[i]);
                    this_ir = this_ir + "i32* " + ir + ",";
                } else {
                    ir= ast_visit_node(node->sons[i]);
                    this_ir = this_ir + "i32 "+ir + ",";
                }
            }
        } else if(node->sons[i]->type==LVal_Array_) {
            int second= symbol_talbe_lookup(stack_table.top(),node->sons[i]->sons[0]->val.ident)->val_array.size()-2;//表示实际的数组维度
            int exp_second=node->sons[i]->sons[1]->sons.size();//表示实参的数组维度
            string ir_left= ast_visit_node(node->sons[i]);
            string ir;
            if (second>0&&exp_second==1) {  //处理putint(a[1]);a为二维数组
                ir=ir_left+"["+ symbol_talbe_lookup(stack_table.top(),node->sons[i]->sons[0]->val.ident)->val_array[1]+"]";
            } else if(second==-1&&exp_second==1) {  //处理putint(a[1]);a为一维数组
                newdeclare("i32*",ir_left);
                string temp=newtemp();
                temp_n++;
                struct ir_struct *ir1=new struct ir_struct("=",ASSIGN_,"*"+ir_left,"NULL",temp,line_no_ir);
                ir_lists[ir_lists.size()-1].push_back(ir1);
                ir_map.insert(make_pair(line_no_ir,ir1));
                line_no_ir++;
                ir=temp;
            } else {   //处理putint(a[1][1])
                newdeclare("i32*",ir_left);
                string temp=newtemp();
                temp_n++;
                struct ir_struct *ir1=new struct ir_struct("=",ASSIGN_,"*"+ir_left,"NULL",temp,line_no_ir);
                ir_lists[ir_lists.size()-1].push_back(ir1);
                ir_map.insert(make_pair(line_no_ir,ir1));
                line_no_ir++;
                ir=temp;
            }
            newdeclare("i32",ir);
            if(i==node->sons.size()-1)
                this_ir = this_ir + "i32 " + ir;
            else
                this_ir = this_ir + "i32 "+ir + ",";
        } else {
            string ir =ast_visit_node(node->sons[i]);
            if(i==node->sons.size()-1)
                this_ir = this_ir + "i32 " + ir;
            else
                this_ir = this_ir + "i32 "+ir + ",";
        }
    }
    return this_ir;
}
//stmt UnaryExp_ 分情况讨论
string unaryexp_ir(struct ast_node *node)
{
    string this_ir;
    if(node->sons[0]->type==SUB_) {
        this_ir=stmt_neg_ir(node->sons[1]);
        return this_ir;
    } else if(node->sons[0]->type==ADD_) {
        this_ir=stmt_pos_ir(node->sons[1]);
        return this_ir;
    } else if(node->sons[0]->type=NOT_) {
        this_ir=stmt_not_ir(node->sons[1]);
        return this_ir;
    }
        return this_ir;
}
//调用函数，传入实参
string unaryexp_func_ir(ast_node*src1_node,ast_node *src2_node)
{
    src1_node->kind_type=funr;
    string ir_right="";
    string ir_left= ast_visit_node(src1_node);//返回的是函数名称
    if(src1_node->parent->sons.size()>1)
        ir_right= ast_visit_node(src2_node);//返回的是参数列表
    if(src1_node->parent->parent->type==Stmt_Exp_)
    {   //处理没有返回值的函数
        string this_ir="";
        struct ir_struct *ir = new struct ir_struct("Call_Void", Call_Void, ir_left, ir_right, "NULL",line_no_ir);
        ir_lists[ir_lists.size() - 1].push_back(ir);
        ir_map.insert(make_pair(line_no_ir,ir));
        line_no_ir++;
        return this_ir;
    }
    else 
    {   //处理有返回值的函数
        string temp = newtemp();
        newdeclare("i32", temp);
        struct ir_struct *ir = new struct ir_struct("Call", Call_, ir_left, ir_right, temp,line_no_ir);
        ir_lists[ir_lists.size() - 1].push_back(ir);
        ir_map.insert(make_pair(line_no_ir,ir));
        line_no_ir++;
        temp_n++;
        return temp;//返回的是最终的结果储存的临时寄存器的编号
    }
}
//解析没有返回值的函数
void  stmt_exp_ir(struct ast_node *src1_node)
{
    string this_ir= ast_visit_node(src1_node);
}
//stmt return
string stmt_return_ir(struct ast_node *src1_node)
{

    string ir_left= ast_visit_node(src1_node);//返回的是返回的具体的值
    //需要在return前面加上label编号，即这个是函数最终的return语句情况为出现if中出现return语句,此时进入此判断语句
    if((src1_node->parent->parent->parent->type==FuncDef_int_||src1_node->parent->parent->parent->type==FuncDef_int_para_)) {
        /*语句格式为
         * .L1:
	            %t9 = %l0
	            exit %t9
        */
        struct ir_struct *ir_3=new struct ir_struct("store",Store_,ir_left,"NULL","%l0",line_no_ir);//%l0=0
        ir_lists[ir_lists.size()-1].push_back(ir_3);
        ir_map.insert(make_pair(line_no_ir,ir_3));
        line_no_ir++;
    }
    //这个是函数出中现不是最终的return语句，这个时候需要生成跳转语句
    else if((src1_node->parent->parent->type!=FuncDef_int_&&src1_node->parent->parent->type!=FuncDef_int_para_)) {
        /*语句格式为
         * 	%l0 = 1
	        br label .L2
         */
        struct ir_struct *ir_3=new struct ir_struct("store",Store_,ir_left,"NULL","%l0",line_no_ir);//%l0=0
        ir_lists[ir_lists.size()-1].push_back(ir_3);
        ir_map.insert(make_pair(line_no_ir,ir_3));
        line_no_ir++;
        auto label_info=label_map.find(return_label)->second;
        label_info->lno=line_no_ir;
        //    struct ir_struct *ir3 = new ir_struct("Label", Label_, return_label, "NULL", "NULL",line_no_ir);
        //    ir_lists[ir_lists.size()-1].push_back(ir3);
        //    ir_map.insert(make_pair(line_no_ir,ir3));
        //    line_no_ir++;
        is_need_return_label=1;
    }
    return "Return";
}
string stmt_return_void_ir() {return "Return";}
//等于号 不对这次的第一个节点，即左值进行处理
string stmt_assign_ir(struct ast_node *src1_node,struct ast_node* src2_node,struct ast_node* result_node)
{
    string this_ir;
    struct information *info= symbol_talbe_lookup(stack_table.top(),src1_node->sons[0]->val.ident);
    string ir_left;
    if(info->kind_type!=Gramma_type::array)
    ir_left=info->value_name;//ir_left的值为%l或者%g
    else
    ir_left= ast_visit_node(src1_node);
    string ir_right= ast_visit_node(src2_node);//ir_right的值为%t
    if(src1_node->type==LVal_Array_&&src2_node->type==LVal_Array_) {
        string right=newtemp();
        temp_n++;
        newdeclare("i32",right);
        struct ir_struct *ir=new struct ir_struct("load",Load_,ir_right,"NULL",right,line_no_ir);
        ir_lists[ir_lists.size()-1].push_back(ir);
        ir_map.insert(make_pair(line_no_ir,ir));
        line_no_ir++;
        struct ir_struct *ir1=new struct ir_struct("store",Store_,right,"NULL",ir_left,line_no_ir);
        ir_lists[ir_lists.size()-1].push_back(ir1);
        ir_map.insert(make_pair(line_no_ir,ir1));
        line_no_ir++;
    } else {
        struct ir_struct *ir = new struct ir_struct("store", Store_, ir_right, "NULL", ir_left,line_no_ir);
        ir_lists[ir_lists.size() - 1].push_back(ir);
        ir_map.insert(make_pair(line_no_ir,ir));
        line_no_ir++;
    }
    return this_ir;
}
//处理变量赋值
string lval_ir(struct ast_node *src1_node)
{
    string this_ir;
    this_ir= ast_visit_node(src1_node);//返回的应该是一个%l1或者%g1
    string temp=newtemp();
    int kind= symbol_talbe_lookup(stack_table.top(),src1_node->val.ident)->kind_type;
    if(kind==Gramma_type::array) {
        newdeclare("i32*",temp);
    } else {
        newdeclare("i32",temp);
    }
    struct ir_struct *ir=new ir_struct("load",Load_,this_ir,this_ir,temp,line_no_ir);
    ir_lists[ir_lists.size()-1].push_back(ir);
    ir_map.insert(make_pair(line_no_ir,ir));
    line_no_ir++;
    temp_n++;
    if(src1_node->parent->parent->type==Stmt_If_||src1_node->parent->parent->type==Stmt_IfElse_
    ||src1_node->parent->parent->type==Stmt_While_||src1_node->parent->parent->type==LAndExp_
    ||src1_node->parent->parent->type==LOrExp_) {
        string ir_left= ast_visit_node(src1_node);//返回的是与左值相等的一个临时变量或者数
        string temp=newtemp();
        newdeclare("i1",temp);//声明成i1类型
        struct ir_struct *ir=new ir_struct("==",NE_, ir_left,"0",temp,line_no_ir);
        ir_lists[ir_lists.size()-1].push_back(ir);
        ir_map.insert(make_pair(line_no_ir,ir));
        line_no_ir++;
        temp_n++;
        //语句格式为bc %t2, label .L3, label .L5
        struct ir_struct *ir1 = new ir_struct("bc", Bc_, src1_node->parent->true_label, src1_node->parent->false_label, temp,line_no_ir);
        ir_lists[ir_lists.size() - 1].push_back(ir1);
        ir_map.insert(make_pair(line_no_ir,ir1));
        line_no_ir++;
    }
    return temp;//返回一个临时变量
}
//用来处理数组偏移量
string exps_ir(struct ast_node *src1_node)
{
    auto info=symbol_talbe_lookup(stack_table.top(),src1_node->parent->sons[0]->val.ident);
    vector<string > order;
    int is_all_number=1;
    for(auto son:src1_node->sons) {
        son->parent=src1_node;
        string number= ast_visit_node(son);
        if(son->type!=Number_)
            is_all_number=0;
        order.push_back(number);
    }
    string first=order[0];
    int first_num=atoi(first.c_str());
    for(int i=0;i<order.size();i++)
    {
        if(is_all_number==1)
        {
            int offset_num;
            offset_num=first_num*atoi(info->val_array[i+1].c_str());
            if(i==src1_node->sons.size()-1) {
                string offset= to_string(offset_num);
                return offset;
            }
            offset_num=atoi(order[i+1].c_str())+offset_num;
            first_num=offset_num;
        }
        else {
            //计算实际个数
            string offset = newtemp();
            temp_n++;
            newdeclare("i32", offset);
            struct ir_struct *ir = new struct ir_struct("mul", MUL_, first, info->val_array[i + 1], offset,line_no_ir);
            ir_lists[ir_lists.size() - 1].push_back(ir);
            ir_map.insert(make_pair(line_no_ir,ir));
            line_no_ir++;
            if (i == src1_node->sons.size() - 1) {
                return offset;
            }
            string next = newtemp();
            temp_n++;
            newdeclare("i32", next);
            struct ir_struct *ir1 = new struct ir_struct("add", ADD_, offset, order[i + 1], next,line_no_ir);
            ir_lists[ir_lists.size() - 1].push_back(ir1);
            ir_map.insert(make_pair(line_no_ir,ir1));
            line_no_ir++;
            first = next;
        }
    }
    return "";
}
//处理数组赋值
string lval_array_ir(struct ast_node *src1_node,struct ast_node *src2_node)
{
    //返回的是数组的基地址，即存在符号表中的编号 %l1
    string ir_left= ast_visit_node(src1_node);
    //返回的是偏移量
    string ir_right= ast_visit_node(src2_node);
    string addr=newtemp();
    temp_n++;
    //a为2维数组，调用a[1]
    int kind= symbol_talbe_lookup(stack_table.top(),src1_node->val.ident)->val_array.size()-1;
    if(kind!=src2_node->sons.size()) {
        int temp= atoi(ir_right.c_str())*4;
        ir_right= to_string(temp);
    }
    struct ir_struct *ir1=new struct ir_struct("add",ADD_,ir_left,ir_right ,addr,line_no_ir);
    ir_lists[ir_lists.size()-1].push_back(ir1);
    ir_map.insert(make_pair(line_no_ir,ir1));
    line_no_ir++;
    //int kind= symbol_talbe_lookup(stack_table.top(),src1_node->sons[0]->val.ident)->second_width;
    if(src1_node->parent->parent->type==Stmt_Assign_) {
        newdeclare("i32*",addr);
        addr="*"+addr;
        return addr;
    } else if(src1_node->parent->parent->type==FuncRParams_) {
        return addr;
    } else if(src1_node->parent->parent->type==Stmt_If_||src1_node->parent->parent->type==Stmt_IfElse_
    ||src1_node->parent->parent->type==Stmt_While_||src1_node->parent->parent->type==LAndExp_
    ||src1_node->parent->parent->type==LOrExp_) {
        newdeclare("i32*",addr);
        string temp=newtemp();
        temp_n++;
        newdeclare("i32",temp);
        struct ir_struct *ir1=new struct ir_struct("load",Load_,"*"+addr,"NULL" ,temp,line_no_ir);
        ir_lists[ir_lists.size()-1].push_back(ir1);
        ir_map.insert(make_pair(line_no_ir,ir1));
        line_no_ir++;
        string temp1=newtemp();
        temp_n++;
        newdeclare("i1",temp1);//声明成i1类型
        struct ir_struct *ir=new ir_struct("==",NE_, temp,"0",temp1,line_no_ir);
        ir_lists[ir_lists.size()-1].push_back(ir);
        ir_map.insert(make_pair(line_no_ir,ir));
        line_no_ir++;
        //语句格式为bc %t2, label .L3, label .L5
        struct ir_struct *ir2 = new ir_struct("bc", Bc_, src1_node->parent->true_label, src1_node->parent->false_label, temp1,line_no_ir);
        ir_lists[ir_lists.size() - 1].push_back(ir2);
        ir_map.insert(make_pair(line_no_ir,ir2));
        line_no_ir++;
        return temp;
    } else {
        newdeclare("i32*",addr);
        string temp=newtemp();
        temp_n++;
        newdeclare("i32",temp);
        struct ir_struct *ir1=new struct ir_struct("load",Load_,"*"+addr,"NULL" ,temp,line_no_ir);
        ir_lists[ir_lists.size()-1].push_back(ir1);
        ir_map.insert(make_pair(line_no_ir,ir1));
        line_no_ir++;
        return temp;
    }
}
//处理大于号节点
string relexp_gt_ir(struct ast_node *src1_node,struct ast_node *src2_node)
{
    string this_ir;
    string if_left= ast_visit_node(src1_node);//返回的是与左值相等的一个临时变量或者数
    string if_right= ast_visit_node(src2_node);//返回的是一个临时变量或者数
    string temp=newtemp();
    newdeclare("i1",temp);//声明成i1类型
    struct ir_struct *ir=new ir_struct(">",GT_, if_left,if_right,temp,line_no_ir);
    ir_lists[ir_lists.size()-1].push_back(ir);
    ir_map.insert(make_pair(line_no_ir,ir));
    line_no_ir++;
    temp_n++;
    //语句格式为bc %t2, label .L3, label .L5
    struct ir_struct *ir1 = new ir_struct("bc", Bc_, src1_node->parent->true_label, src1_node->parent->false_label, temp,line_no_ir);
    ir_lists[ir_lists.size() - 1].push_back(ir1);
    ir_map.insert(make_pair(line_no_ir,ir1));
    line_no_ir++;
    return temp;//将结果装到临时变量，返回一个临时变量
}
//处理大于等于节点
string relexp_ge_ir(struct ast_node *src1_node,struct ast_node *src2_node)
{
    string this_ir;
    string if_left= ast_visit_node(src1_node);//返回的是与左值相等的一个临时变量或者数
    string if_right= ast_visit_node(src2_node);//返回的是一个临时变量或者数
    string temp=newtemp();
    newdeclare("i1",temp);//声明成i1类型
    struct ir_struct *ir=new ir_struct(">=",GE_, if_left,if_right,temp,line_no_ir);
    ir_lists[ir_lists.size()-1].push_back(ir);
    ir_map.insert(make_pair(line_no_ir,ir));
    line_no_ir++;
    temp_n++;
    //语句格式为bc %t2, label .L3, label .L5
    struct ir_struct *ir1 = new ir_struct("bc", Bc_, src1_node->parent->true_label, src1_node->parent->false_label, temp,line_no_ir);
    ir_lists[ir_lists.size() - 1].push_back(ir1);
    ir_map.insert(make_pair(line_no_ir,ir1));
    line_no_ir++;
    return temp;//将结果装到临时变量，返回一个临时变量
}
//处理小于号节点
string relexp_lt_ir(struct ast_node *src1_node,struct ast_node *src2_node)
{
    string this_ir;
    string if_left= ast_visit_node(src1_node);//返回的是与左值相等的一个临时变量或者数
    string if_right= ast_visit_node(src2_node);//返回的是一个临时变量或者数
    string temp=newtemp();
    newdeclare("i1",temp);//声明成i1类型
    struct ir_struct *ir=new ir_struct("<",LT_, if_left,if_right,temp,line_no_ir);
    ir_lists[ir_lists.size()-1].push_back(ir);
    ir_map.insert(make_pair(line_no_ir,ir));
    line_no_ir++;
    temp_n++;
    //语句格式为bc %t2, label .L3, label .L5
    struct ir_struct *ir1 = new ir_struct("bc", Bc_, src1_node->parent->true_label, src1_node->parent->false_label, temp,line_no_ir);
    ir_lists[ir_lists.size() - 1].push_back(ir1);
    ir_map.insert(make_pair(line_no_ir,ir1));
    line_no_ir++;
    return temp;//将结果装到临时变量，返回一个临时变量
}
//处理大于号节点
string relexp_le_ir(struct ast_node *src1_node,struct ast_node *src2_node)
{
    string this_ir;
    string if_left= ast_visit_node(src1_node);//返回的是与左值相等的一个临时变量或者数
    string if_right= ast_visit_node(src2_node);//返回的是一个临时变量或者数
    string temp=newtemp();
    newdeclare("i1",temp);//声明成i1类型
    struct ir_struct *ir=new ir_struct("<=",LE_, if_left,if_right,temp,line_no_ir);
    ir_lists[ir_lists.size()-1].push_back(ir);
    ir_map.insert(make_pair(line_no_ir,ir));
    line_no_ir++;
    temp_n++;
    //语句格式为bc %t2, label .L3, label .L5
    struct ir_struct *ir1 = new ir_struct("bc", Bc_, src1_node->parent->true_label, src1_node->parent->false_label, temp,line_no_ir);
    ir_lists[ir_lists.size() - 1].push_back(ir1);
    ir_map.insert(make_pair(line_no_ir,ir1));
    line_no_ir++;
    return temp;//将结果装到临时变量，返回一个临时变量
}
//处理大于号节点
string relexp_ne_ir(struct ast_node *src1_node,struct ast_node *src2_node)
{
    string this_ir;
    string if_left= ast_visit_node(src1_node);//返回的是与左值相等的一个临时变量或者数
    string if_right= ast_visit_node(src2_node);//返回的是一个临时变量或者数
    string temp=newtemp();
    newdeclare("i1",temp);//声明成i1类型
    struct ir_struct *ir=new ir_struct("!=",NE_, if_left,if_right,temp,line_no_ir);
    ir_lists[ir_lists.size()-1].push_back(ir);
    ir_map.insert(make_pair(line_no_ir,ir));
    line_no_ir++;
    temp_n++;
    //语句格式为bc %t2, label .L3, label .L5
    struct ir_struct *ir1 = new ir_struct("bc", Bc_, src1_node->parent->true_label, src1_node->parent->false_label, temp,line_no_ir);
    ir_lists[ir_lists.size() - 1].push_back(ir1);
    ir_map.insert(make_pair(line_no_ir,ir1));
    line_no_ir++;
    return temp;//将结果装到临时变量，返回一个临时变量
}
//处理大于号节点
string relexp_eq_ir(struct ast_node *src1_node,struct ast_node *src2_node)
{
    string this_ir;
    string if_left= ast_visit_node(src1_node);//返回的是与左值相等的一个临时变量或者数
    string if_right= ast_visit_node(src2_node);//返回的是一个临时变量或者数
    string temp=newtemp();
    newdeclare("i1",temp);//声明成i1类型
    struct ir_struct *ir=new ir_struct("==",EQ_, if_left,if_right,temp,line_no_ir);
    ir_lists[ir_lists.size()-1].push_back(ir);
    ir_map.insert(make_pair(line_no_ir,ir));
    line_no_ir++;
    temp_n++;
    //语句格式为bc %t2, label .L3, label .L5
    struct ir_struct *ir1 = new ir_struct("bc", Bc_, src1_node->parent->true_label, src1_node->parent->false_label, temp,line_no_ir);
    ir_lists[ir_lists.size() - 1].push_back(ir1);
    ir_map.insert(make_pair(line_no_ir,ir1));
    line_no_ir++;
    return temp;//将结果装到临时变量，返回一个临时变量
}
//处理&&
string landexp_and_ir(struct ast_node *src1_node,struct ast_node *src2_node) {
    string true_label=newlabel();
    label_n++;
    src1_node->true_label=true_label;
    src1_node->false_label=src1_node->parent->false_label;
    src2_node->true_label=src1_node->parent->true_label;
    src2_node->false_label=src1_node->parent->false_label;
    label_n++;
    string ir_left = ast_visit_node(src1_node);//返回的值得到一个%t
    if(src1_node->type==UnaryExp_||src1_node->type==UnaryExp_func_)
    {   //语句格式为bc %t2, label .L3, label .L5
        struct ir_struct *ir1 = new ir_struct("bc", Bc_, src1_node->true_label, src1_node->false_label, ir_left,line_no_ir);
        ir_lists[ir_lists.size() - 1].push_back(ir1);
        ir_map.insert(make_pair(line_no_ir,ir1));
        line_no_ir++;
    }
    auto label_info=label_map.find(true_label)->second;
    label_info->lno=line_no_ir;
    struct ir_struct *ir1 = new ir_struct("Label", Label_, true_label, "NULL", "NULL",line_no_ir);
    ir_lists[ir_lists.size() - 1].push_back(ir1);
    ir_map.insert(make_pair(line_no_ir,ir1));
    line_no_ir++;
    string ir_right = ast_visit_node(src2_node);
    if(src2_node->type==UnaryExp_||src2_node->type==UnaryExp_func_)
    {   //语句格式为bc %t2, label .L3, label .L5
        struct ir_struct *ir1 = new ir_struct("bc", Bc_, src2_node->true_label, src2_node->false_label, ir_right,line_no_ir);
        ir_lists[ir_lists.size() - 1].push_back(ir1);
        ir_map.insert(make_pair(line_no_ir,ir1));
        line_no_ir++;
    }
    return ir_right;
}
string landexp_or_ir(struct ast_node *src1_node,struct ast_node *src2_node) {
    string false_label = newlabel();
    label_n++;
    src1_node->true_label=src1_node->parent->true_label;
    src1_node->false_label=false_label;
    src2_node->true_label=src1_node->parent->true_label;
    src2_node->false_label=src1_node->parent->false_label;
    string ir_left = ast_visit_node(src1_node);//返回的值得到一个%t
    if(src1_node->type==UnaryExp_||src1_node->type==UnaryExp_func_)
    {
        //语句格式为bc %t2, label .L3, label .L5
        struct ir_struct *ir1 = new ir_struct("bc", Bc_, src1_node->true_label, src1_node->false_label, ir_left,line_no_ir);
        ir_lists[ir_lists.size() - 1].push_back(ir1);
        ir_map.insert(make_pair(line_no_ir,ir1));
        line_no_ir++;
    }
    auto label_info=label_map.find(false_label)->second;
    label_info->lno=line_no_ir;
    struct ir_struct *ir1 = new ir_struct("Label", Label_, false_label, "NULL", "NULL",line_no_ir);
    ir_lists[ir_lists.size() - 1].push_back(ir1);
    ir_map.insert(make_pair(line_no_ir,ir1));
    line_no_ir++;
    string ir_right = ast_visit_node(src2_node);
    if(src2_node->type==UnaryExp_||src2_node->type==UnaryExp_func_)
    {
        //语句格式为bc %t2, label .L3, label .L5
        struct ir_struct *ir1 = new ir_struct("bc", Bc_, src2_node->true_label, src2_node->false_label, ir_right,line_no_ir);
        ir_lists[ir_lists.size() - 1].push_back(ir1);
        ir_map.insert(make_pair(line_no_ir,ir1));
        line_no_ir++;
    }
    return ir_right;
}
//处理if节点
string stmt_if_ir(struct ast_node *src1_node,struct ast_node *src2_node)
{
    string label1 = newlabel();//条件为真的标签
    label_n++;
    string label2 = newlabel();//条件为假的标签
    label_n++;
    src1_node->true_label=label1;
    src1_node->false_label=label2;
    string ir_left= ast_visit_node(src1_node);
    if(src1_node->type==UnaryExp_||src1_node->type==UnaryExp_func_) {
        //语句格式为bc %t2, label .L3, label .L5
        struct ir_struct *ir1 = new ir_struct("bc", Bc_, src1_node->true_label, src1_node->false_label,ir_left,line_no_ir);
        ir_lists[ir_lists.size() - 1].push_back(ir1);
        ir_map.insert(make_pair(line_no_ir,ir1));
        line_no_ir++;
    }
    auto label_info=label_map.find(label1)->second;
    label_info->lno=line_no_ir;
    struct ir_struct *ir1 = new ir_struct("Label", Label_, label1, "NULL", "NULL",line_no_ir);
    ir_lists[ir_lists.size() - 1].push_back(ir1);
    ir_map.insert(make_pair(line_no_ir,ir1));
    line_no_ir++;
    string ir_right = ast_visit_node(src2_node);
    struct ir_struct *ir4;
    if(ir_right=="Return") {
        ir4 = new ir_struct("br", Br_, return_label, "NULL", "NULL",line_no_ir);
        ir_lists[ir_lists.size() - 1].push_back(ir4);
        ir_map.insert(make_pair(line_no_ir,ir4));
        line_no_ir++;
    }
    else if(ir_right=="Break"){}
    else if(ir_right=="Conitue"){}
    else {
            ir4 = new ir_struct("br", Br_, label2, "NULL", "NULL",line_no_ir);
            ir_lists[ir_lists.size() - 1].push_back(ir4);
        ir_map.insert(make_pair(line_no_ir,ir4));
        line_no_ir++;
    }
    //语句格式为.l3:********* br label .l2  条件为真
    auto label_info_1=label_map.find(label2)->second;
    label_info_1->lno=line_no_ir;
    struct ir_struct *ir3 = new ir_struct("Label", Label_, label2, "NULL", "NULL",line_no_ir);
    ir_lists[ir_lists.size() - 1].push_back(ir3);
    ir_map.insert(make_pair(line_no_ir,ir3));
    line_no_ir++;
    return ir_right;
}
//处理ifelse
string stmt_ifelse_ir(struct ast_node *src1_node,struct  ast_node *src2_node,struct ast_node*src3_node)
{
    //src1_node为if else 的条件
    string label1 = newlabel();//条件为真的标签
    label_n++;
    string label2 = newlabel();//条件为假的标签
    label_n++;
    string label3=newlabel();//为next的标签
    label_n++;
    //存储当前的if else的两个跳转标签
    src1_node->true_label=label1;
    src1_node->false_label=label2;
    string ir_left= ast_visit_node(src1_node);
    if(src1_node->type==UnaryExp_||src1_node->type==UnaryExp_func_)
    {
        //语句格式为bc %t2, label .L3, label .L5
        struct ir_struct *ir1 = new ir_struct("bc", Bc_, src1_node->true_label, src1_node->false_label,ir_left,line_no_ir);
        ir_lists[ir_lists.size() - 1].push_back(ir1);
        ir_map.insert(make_pair(line_no_ir,ir1));
        line_no_ir++;
    }
    auto label_info=label_map.find(label1)->second;
    label_info->lno=line_no_ir;
    struct ir_struct *ir1 = new ir_struct("Label", Label_, label1, "NULL", "NULL",line_no_ir);
    ir_lists[ir_lists.size() - 1].push_back(ir1);
    ir_map.insert(make_pair(line_no_ir,ir1));
    line_no_ir++;
    //src2_node为if 成立时的语句
    string ir_middle = ast_visit_node(src2_node);
    struct ir_struct *ir4;
    if(ir_middle=="Return")
    {
        ir4 = new ir_struct("br", Br_, return_label, "NULL", "NULL",line_no_ir);
        ir_lists[ir_lists.size() - 1].push_back(ir4);
        ir_map.insert(make_pair(line_no_ir,ir4));
        line_no_ir++;
    }
    else if(ir_middle=="Break"){}
    else if(ir_middle=="Conitue"){}
    else
    {

        ir4 = new ir_struct("br", Br_, label3, "NULL", "NULL",line_no_ir);
        ir_lists[ir_lists.size() - 1].push_back(ir4);
        ir_map.insert(make_pair(line_no_ir,ir4));
        line_no_ir++;
    }

    //src3_node为else 成立时的语句
    auto label_info_2=label_map.find(label2)->second;
    label_info_2->lno=line_no_ir;
    struct ir_struct *ir6 = new ir_struct("Label", Label_, label2, "NULL", "NULL",line_no_ir);
    ir_lists[ir_lists.size() - 1].push_back(ir6);
    ir_map.insert(make_pair(line_no_ir,ir6));
    line_no_ir++;
    string ir_right = ast_visit_node(src3_node);
    struct ir_struct *ir5;
    if(ir_right=="Return")
    {
        ir5 = new ir_struct("br", Br_, return_label, "NULL", "NULL",line_no_ir);
        ir_lists[ir_lists.size() - 1].push_back(ir5);
        ir_map.insert(make_pair(line_no_ir,ir5));
        line_no_ir++;
    }
    else if(ir_right=="Break"){}
    else if(ir_right=="Conitue"){}
    else
    {
        ir5 = new ir_struct("br", Br_, label3, "NULL", "NULL",line_no_ir);
        ir_lists[ir_lists.size() - 1].push_back(ir5);
        ir_map.insert(make_pair(line_no_ir,ir5));
        line_no_ir++;
    }
    auto label_info_3=label_map.find(label3)->second;
    label_info_3->lno=line_no_ir;
    struct ir_struct *ir7= new ir_struct("Label", Label_, label3, "NULL", "NULL",line_no_ir);
    ir_lists[ir_lists.size() - 1].push_back(ir7);
    ir_map.insert(make_pair(line_no_ir,ir7));
    line_no_ir++;
    return ir_right;
}
//解析while语句
string stmt_while_ir(struct ast_node *src1_node,struct ast_node* src2_node)
{
    string this_ir;
    string label1=newlabel();//用来进入while语句的标签
    label_n++;
    struct ir_struct *ir1 = new ir_struct("br", Br_, label1, "NULL", "NULL",line_no_ir);
    ir_lists[ir_lists.size()-1].push_back(ir1);
    ir_map.insert(make_pair(line_no_ir,ir1));
    line_no_ir++;
    auto label_info=label_map.find(label1)->second;
    label_info->lno=line_no_ir;
    struct ir_struct *ir2 = new ir_struct("Label", Label_, label1, "NULL", "NULL",line_no_ir);
    ir_lists[ir_lists.size() - 1].push_back(ir2);
    ir_map.insert(make_pair(line_no_ir,ir2));
    line_no_ir++;
    string label2=newlabel();//用来进入while循环体语句的标签
    label_n++;
    string label3=newlabel();//while后面的代码
    label_n++;
    //存储当前的条件的两个跳转标签
    src1_node->true_label=label2;
    src1_node->false_label=label3;
    struct while_label_struct *W=new struct while_label_struct();
    W->entry=label1;
    W->loop=label2;
    W->next=label3;
    while_list.push(W);
    string ir_left= ast_visit_node(src1_node);
    if(src1_node->type==UnaryExp_||src1_node->type==UnaryExp_func_)
    {
        //语句格式为bc %t2, label .L3, label .L5
        struct ir_struct *ir1 = new ir_struct("bc", Bc_, src1_node->true_label, src1_node->false_label,ir_left,line_no_ir);
        ir_lists[ir_lists.size() - 1].push_back(ir1);
        ir_map.insert(make_pair(line_no_ir,ir1));
        line_no_ir++;
    }
    auto label_info_1=label_map.find(label2)->second;
    label_info_1->lno=line_no_ir;
    struct ir_struct *ir4 = new ir_struct("Label", Label_, label2, "NULL", "NULL",line_no_ir);
    ir_lists[ir_lists.size() - 1].push_back(ir4);
    ir_map.insert(make_pair(line_no_ir,ir4));
    line_no_ir++;
    //进入循环体
    string ir_right= ast_visit_node(src2_node);
    struct ir_struct *ir9 = new ir_struct("br", Br_, label1, "NULL", "NULL",line_no_ir);
    ir_lists[ir_lists.size() - 1].push_back(ir9);//跳到判断条件
    ir_map.insert(make_pair(line_no_ir,ir9));
    line_no_ir++;
    auto label_info_2=label_map.find(label3)->second;
    label_info_2->lno=line_no_ir;
    struct ir_struct *ir6 = new ir_struct("Label", Label_, label3, "NULL", "NULL",line_no_ir);
    ir_lists[ir_lists.size() - 1].push_back(ir6);//跳出循环体
    ir_map.insert(make_pair(line_no_ir,ir6));
    line_no_ir++;
    while_list.pop();
    return this_ir;//不知道返回啥，先空着
}
//解析break语句
string stmt_break_ir(struct  ast_node *src1_node)
{
    string this_ir="Break";
    struct while_label_struct *temp=while_list.top();
    struct ir_struct *ir7 = new ir_struct("br", Br_, temp->next, "NULL", "NULL",line_no_ir);
    ir_lists[ir_lists.size() - 1].push_back(ir7);//跳到判断条件
    ir_map.insert(make_pair(line_no_ir,ir7));
    line_no_ir++;
    string label=newlabel();
    label_n++;
    auto label_info=label_map.find(label)->second;
    label_info->lno=line_no_ir;
    struct ir_struct *ir1 = new ir_struct("Label", Label_, label, "NULL", "NULL",line_no_ir);
    ir_lists[ir_lists.size() - 1].push_back(ir1);
    ir_map.insert(make_pair(line_no_ir,ir1));
    line_no_ir++;
    return this_ir;
}
//解析contiune语句
string stmt_conitue_ir(struct  ast_node *src1_node)
{
    string this_ir="Conitue";
    struct while_label_struct *temp=while_list.top();
    struct ir_struct *ir7 = new ir_struct("br", Br_, temp->entry, "NULL", "NULL",line_no_ir);
    ir_lists[ir_lists.size() - 1].push_back(ir7);//跳到判断条件
    ir_map.insert(make_pair(line_no_ir,ir7));
    line_no_ir++;
    string label=newlabel();
    label_n++;
    auto label_info=label_map.find(label)->second;
    label_info->lno=line_no_ir;
    struct ir_struct *ir1 = new ir_struct("Label", Label_, label, "NULL", "NULL",line_no_ir);
    ir_lists[ir_lists.size() - 1].push_back(ir1);
    ir_map.insert(make_pair(line_no_ir,ir1));
    line_no_ir++;
    return this_ir;
}
// ident
string ident_ir(struct ast_node* src1_node) {
    struct symbol_table *nowtable = stack_table.top();
    string this_ir;
    string name = src1_node->val.ident;//该标识符的名字
    struct information*result= symbol_talbe_lookup(nowtable,name);
    //处理库函数
    if(result== nullptr&&src1_node->kind_type==funr)
    {
        return name;
    }
    //处理正常情况
    if (result->value_name=="NULL") {
        if ((result->kind_type) == func) {
            this_ir = "@" + src1_node->val.ident + "(";
            //给当前语句生成新的声明块
            string new_fun_string;
            fun_declares.push_back(new_fun_string);
            list ir_list;
            ir_lists.push_back(ir_list);
            ir_alloc_list.push_back(ir_list);
            temp_n = 1;
            local_n = 1;
            result->value_name = name;
            is_need_return_label=0;
            return result->value_name;
        }
        else if(result->kind_type==funr)
        {
            result->value_name="@"+name;
            return result->value_name;
        }
        else {
            if (isglobal) {
                this_ir = newglobal();
                global_n++;

            } else {
                this_ir = newlocal();
                local_n++;
            }
        }
        result->value_name = this_ir;//存入临时变量的名字
        return this_ir;
    }
    else return result->value_name;
}
// int
string int_ir(struct ast_node *src1_node)
{
    string this_ir;
    if (src1_node->kind_type==func)
    {
        newdeclare("i32","%l0");
    }
    return this_ir;
}
// const
string const_ir(struct ast_node *src1_node)
{
    string this_ir;
    return this_ir;
}
// void
string void_ir(struct ast_node*node)
{
    string this_ir;
    return this_ir;
}
// number
string number_ir(struct ast_node*src1_node)
{
    string this_ir;
    this_ir=to_string(src1_node->val.value);    // 整型数转化为字符串
    return this_ir;
}
// 访问抽象语法树结点，根据结点类型生成IR
string ast_visit_node (struct ast_node *node)
{
    if(nullptr==node)
        return nullptr;
    string  ir;
    if(node->type==VarDecl_)
    {
        ir=vardecl_ir(node->sons[0],node->sons[1]);
        return ir;
    }
    else if(node->type==ConstDecl_)
    {
        ir=constdecl_ir(node->sons[0],node->sons[1]);
        return ir;
    }
    else if(node->type==ConstDef_single_)
    {
        ir=constdef_single_ir(node->sons[0],node->sons[1]);
        return ir;
    }
    else if(node->type==ConstDef_array_)
    {
        ir=constdef_array_ir(node->sons[0],node->sons[1],node->sons[2]);
        return ir;
    }
    else if(node->type==VarDefs_)
    {
        vardefs_ir(node);
        return ir;
    }
    else if(node->type==FuncDef_int_)
    {
        ir= funcdef_int_ir(node->sons[0],node->sons[1],node->sons[2]);
        return ir;
    }
    else if(node->type==FuncDef_int_para_)
    {
        ir= funcdef_int_para_ir(node);
        return ir;
    }
    else if(node->type==InitVals_)
    {
        ir= initvals_ir(node);
        return ir;
    }
    else if(node->type==FuncFParam_single_)
    {
        ir= funcfparam_single_ir(node);
        return ir;
    }
    else if(node->type==FuncFParam_array_)
    {
        ir= funcfparam_array_ir(node);
        return ir;
    }
    else if(node->type==FuncFParam_)
    {
        ir= funcfparam_ir(node);
        return ir;
    }
    else if(node->type==FuncDef_void_)
    {
        ir= funcdef_void_ir(node->sons[0],node->sons[1],node->sons[2]);
        return ir;
    }
    else if(node->type==FuncDef_void_para_)
    {
        ir= funcdef_void_para_ir(node);
        return ir;
    }
    else if(node->type==Block_)
    {
        ir=block_ir(node,node->parent);
        return ir;
    }
    else if(node->type==AddExp_Add_)
    {
        ir= expr_add_ir(node->sons[0],node->sons[1],node);
        return ir;
    }
    else if(node->type==AddExp_Sub_)
    {
        ir= expr_sub_ir(node->sons[0],node->sons[1],node);
        return ir;
    }
    else if(node->type==MulExp_Mul_)
    {
        ir=expr_mul_ir(node->sons[0],node->sons[1],node);
        return ir;
    }
    else if(node->type==MulExp_Div_)
    {
        ir=expr_div_ir(node->sons[0],node->sons[1],node);
        return ir;
    }
    else if(node->type==MulExp_Mod_)
    {
        ir=expr_mod_ir(node->sons[0],node->sons[1],node);
        return ir;
    }
    else if(node->type==Stmt_Assign_)
    {
        ir=stmt_assign_ir(node->sons[0],node->sons[1],node);
        return ir;
    }
    else if(node->type==Stmt_Exp_)
    {
        stmt_exp_ir(node->sons[0]);
        return ir;
    }
    else if(node->type==UnaryExp_func_)
    {
        ir= unaryexp_func_ir(node->sons[0],node->sons[1]);
        return ir;
    }
    else if(node->type==FuncRParams_)
    {
        ir= funcrparams_ir(node);
        return ir;
    }
    else if(node->type==RelExp_GT_)
    {
        ir=relexp_gt_ir(node->sons[0],node->sons[1]);
        return ir;
    }
    else if(node->type==RelExp_GE_)
    {
        ir=relexp_ge_ir(node->sons[0],node->sons[1]);
        return ir;
    }
    else if(node->type==RelExp_LT_)
    {
        ir=relexp_lt_ir(node->sons[0],node->sons[1]);
        return ir;
    }
    else if(node->type==RelExp_LE_)
    {
        ir=relexp_le_ir(node->sons[0],node->sons[1]);
        return ir;
    }
    else if(node->type==EqExp_EQ_)
    {
        ir=relexp_eq_ir(node->sons[0],node->sons[1]);
        return ir;
    }
    else if(node->type==EqExp_NE_)
    {
        ir= relexp_ne_ir(node->sons[0],node->sons[1]);
        return ir;
    }
    else if(node->type==LAndExp_)
    {
        ir=landexp_and_ir(node->sons[0],node->sons[1]);
        return ir;
    }
    else if(node->type==LOrExp_)
    {
        ir=landexp_or_ir(node->sons[0],node->sons[1]);
        return ir;
    }
    else if(node->type==Stmt_If_)
    {
        ir= stmt_if_ir(node->sons[0],node->sons[1]);
        return ir;
    }
    else if(node->type==Stmt_IfElse_)
    {
        ir= stmt_ifelse_ir(node->sons[0],node->sons[1],node->sons[2]);
        return ir;
    }
    else if(node->type==Stmt_While_)
    {
        ir= stmt_while_ir(node->sons[0],node->sons[1]);
        return ir;
    }
    else if(node->type==Stmt_Break_)
    {
        ir= stmt_break_ir(node);
        return ir;
    }
    else if(node->type==Stmt_Conitue_)
    {
        ir= stmt_conitue_ir(node);
        return ir;
    }
    else if(node->type==ConstExps_)
    {
        ir= constexps_ir(node);
        return ir;
    }
    else if(node->type==LVal_)
    {
        ir= lval_ir(node->sons[0]);
        return ir;
    }
    else if(node->type==LVal_Array_)
    {
        ir= lval_array_ir(node->sons[0],node->sons[1]);
        return ir;
    }
    else if(node->type==Exps_)
    {
        ir= exps_ir(node);
        return ir;
    }
    else if(node->type==INT_)
    {
        ir=int_ir(node);
        return ir;
    }
    else if(node->type==CONST_)
    {
        ir=const_ir(node);
        return ir;
    }
    else if(node->type==VOID_)
    {
        ir=void_ir(node);
        return ir;
    }
    else if(node->type==Ident_)
    {
        ir=ident_ir(node);
        return ir;
    }
    else if(node->type==Number_)
    {
        ir= number_ir(node);
        return ir;
    }
    else if(node->type==UnaryExp_)
    {
        ir= unaryexp_ir(node);
        return ir;
    }
    else if(node->type==Stmt_Return_)
    {
        ir=stmt_return_ir(node->sons[0]);
        return ir;
    }
    else if(node->type==Stmt_Return_Void_)
    {
        ir=stmt_return_void_ir();
        return ir;
    }
    else
        return nullptr;
}
/************************************/
/*        以下是打印相关函数         */
/************************************/
// 用于抽象语法树结点计数
int node_count = 0;
// 对于一些结点进行特殊处理
/*  以下ast_node进行消除处理
    ConstDef_single VarDef_single
    FuncFParam_single FuncFParam_singleArray
    LVal LValArray */
/*  以下ast_node进行赋值处理
    IDENT
    NUMBER
    ADD AddExp_Add
    SUB AddExp_Sub
    MulExp_Mul
    MulExp_Div
    MulExp_Mod
    RelExp_LT
    RelExp_GT
    RelExp_LE
    RelExp_GE
    NOT
    EqExpp_EQ
    EqExpp_NE */
// 打印抽象语法树的结点
void print_ast_node(FILE* fp, ast_node* node)
{
    if(strcmp(node->name.c_str(), "ConstDef_single") && strcmp(node->name.c_str(), "VarDef_single") 
    && strcmp(node->name.c_str(), "FuncFParam_single") && strcmp(node->name.c_str(), "FuncFParam_singleArray")
    && strcmp(node->name.c_str(), "LVal") && strcmp(node->name.c_str(), "LVal_Array") && strcmp(node->name.c_str(), "Block_"))
    {
        node->nodeno = node_count++;
        if(!strcmp(node->name.c_str(),"IDENT")) {
            fprintf_s(fp, "node%d [label=\"%s\"]\n", node->nodeno, node->val.ident.c_str());        // 变量名
        } else if(!strcmp(node->name.c_str(),"NUMBER")) {
            fprintf_s(fp, "node%d [label=\"%d\"]\n", node->nodeno, node->val.value);                // 常量值
        }else if (!strcmp(node->name.c_str(),"ADD") ||!strcmp(node->name.c_str(),"AddExp_Add")) {
            fprintf_s(fp, "node%d [label=\"+\"]\n", node->nodeno);                                  // +
        } else if (!strcmp(node->name.c_str(),"SUB") ||!strcmp(node->name.c_str(),"AddExp_Sub")) {
            fprintf_s(fp, "node%d [label=\"-\"]\n", node->nodeno);                                  // -
        } else if (!strcmp(node->name.c_str(),"MulExp_Mul")) {
            fprintf_s(fp, "node%d [label=\"*\"]\n", node->nodeno);                                  // *
        } else if (!strcmp(node->name.c_str(),"MulExp_Div")) {
            fprintf_s(fp, "node%d [label=\"/\"]\n", node->nodeno);                                  // /
        } else if (!strcmp(node->name.c_str(),"MulExp_Mod")) {
            fprintf_s(fp, "node%d [label=\"%\"]\n", node->nodeno);                                  // %
        } else if (!strcmp(node->name.c_str(),"RelExp_LT")) {
            fprintf_s(fp, "node%d [label=\"<\"]\n", node->nodeno);                                  // <
        } else if (!strcmp(node->name.c_str(),"RelExp_GT")) {
            fprintf_s(fp, "node%d [label=\">\"]\n", node->nodeno);                                  // >
        } else if (!strcmp(node->name.c_str(),"RelExp_LE")) {      
            fprintf_s(fp, "node%d [label=\"<=\"]\n", node->nodeno);                                 // <=
        } else if (!strcmp(node->name.c_str(),"RelExp_GE")) {      
            fprintf_s(fp, "node%d [label=\">=\"]\n", node->nodeno);                                 // >=
        } else if (!strcmp(node->name.c_str(),"NOT")) {             
            fprintf_s(fp, "node%d [label=\"!\"]\n", node->nodeno);                                  // !
        } else if (!strcmp(node->name.c_str(),"EqExpp_EQ")) {      
            fprintf_s(fp, "node%d [label=\"==\"]\n", node->nodeno);                                 // ==
        } else if (!strcmp(node->name.c_str(),"EqExpp_NE")) {      
            fprintf_s(fp, "node%d [label=\"!=\"]\n", node->nodeno);                                 // !=
        } else {
            fprintf_s(fp, "node%d [label=\"%s\"]\n", node->nodeno, node->name.c_str());             // 其他结点
        }
    }
    for(int i=0;i<node->sons.size();i++) {
        print_ast_node(fp,node->sons[i]);   // 打印子节点
    }
}
/*  以下ast_node进行消除处理
    其父结点与其子节点相连
    ConstDef_single
    VarDef_single
    FuncFParam_single
    FuncFParam_singleArray
    LVal
    LValArray */
// 打印抽象语法树的有向边
void print_ast_edge(FILE* fp, ast_node* node)
{
    for(int i=0;i<node->sons.size();i++)
    {
        if(strcmp(node->sons[i]->name.c_str(), "ConstDef_single") && strcmp(node->sons[i]->name.c_str(), "VarDef_single") 
        && strcmp(node->sons[i]->name.c_str(), "FuncFParam_single") && strcmp(node->sons[i]->name.c_str(), "FuncFParam_singleArray")
        && strcmp(node->sons[i]->name.c_str(), "LVal") && strcmp(node->name.c_str(), "LVal_Array") && strcmp(node->name.c_str(), "Block_")) {
            fprintf_s(fp, "node%d->node%d;\n", node->nodeno, node->sons[i]->nodeno);
            print_ast_edge(fp,node->sons[i]);
        } else {
            fprintf_s(fp, "node%d->node%d;\n", node->nodeno, node->sons[i]->sons[0]->nodeno);   // 其父结点与其子节点相连
        }
    }
}
// 打印抽象语法树
void print_ast_to_file(string file)
{
    FILE* fp;
	fopen_s(&fp, "ast.gv", "w+");   // 新建并打开一个用于graphviz画图的中间文件
	fprintf_s(fp, "digraph graphname {\n");
    print_ast_node(fp, ast_root);
    print_ast_edge(fp, ast_root);
    fprintf_s(fp, "}\n");
    fclose(fp);
	string command = "dot ast.gv -Tpng -o " + file; // 用graphviz命令画图
    system(command.c_str());    // 执行画图命令
	system(file.c_str());       // 展示抽象语法树
}
// 打印四元式
void print_quadruple_to_file(string file)
{
    ofstream out(file);
    if(out.is_open())
    {
        auto global_block_ir = block_ir_list[0][0]->block_ir;
        for(int i=0;i<ir_alloc_list[0].size();i++)
        {   // 全局变量声明
            out << "(" << ir_alloc_list[0][i]->opstring << "," <<ir_alloc_list[0][i]->src1 << "," 
                << ir_alloc_list[0][i]->src2 << "," << ir_alloc_list[0][i]->res << ")" << endl;
        }
        for(int i=0;i<global_block_ir.size();i++) 
        {   // 全局变量相关语句
            string ir = emit(global_block_ir[i]->op, global_block_ir[i]->src1, global_block_ir[i]->src2, global_block_ir[i]->res);
            out << ir << endl;
        }
        for(int j=1;j<fun_declares.size();j++) 
        {   // 函数
            // out << "——基本块划分——" << endl;
            out << fun_names[j-1] << ":" << endl;
            for(int i=0;i<ir_alloc_list[j].size();i++)
            {   // 函数入口
                // 局部变量声明
                out << "(" << ir_alloc_list[j][i]->opstring << "," <<ir_alloc_list[j][i]->src1 << "," 
                    << ir_alloc_list[j][i]->src2 << "," << ir_alloc_list[j][i]->res << ")" << endl;
            }
            auto nowlist = block_ir_list[j];
            for (int i = 0; i < nowlist.size(); i++) 
            {
                // out << "——基本块划分——" << endl;
                // 标签
                out << "(" << nowlist[i]->Label->opstring << "," << nowlist[i]->Label->src1 << ","
                    << nowlist[i]->Label->src2 << "," << nowlist[i]->Label->res << ")" << endl;
                for (int m = 0; m < nowlist[i]->block_ir.size(); m++) 
                {   // 普通语句
                    out << "(" << nowlist[i]->block_ir[m]->opstring << "," << nowlist[i]->block_ir[m]->src1 
                        << "," << nowlist[i]->block_ir[m]->src2 << "," << nowlist[i]->block_ir[m]->res << ")" << endl;
                }
                // 跳转
                out << "(" << nowlist[i]->Jump->opstring << "," << nowlist[i]->Jump->src1 << ","
                    << nowlist[i]->Jump->src2 << "," << nowlist[i]->Jump->res << ")" << endl;
            }
        }
    }
    out.close();
    system(file.c_str());
}
// 打印中间IR
void print_ir_to_file(string file)
{
    ofstream out(file);
    if (out.is_open())
    {
        out<<fun_declares[0];
        auto global_block_ir =block_ir_list[0][0]->block_ir;
        for(int i=0;i<global_block_ir.size();i++) 
        {   // 全局变量
            string ir = emit(global_block_ir[i]->op, global_block_ir[i]->src1, global_block_ir[i]->src2, global_block_ir[i]->res);
            out << ir << endl;
        }
        for(int j=1;j<fun_declares.size();j++)
        {   // 函数
            out << fun_declares[j]; // 函数声明
            auto nowlist=block_ir_list[j];
            for(int i=0;i<nowlist.size();i++) { 
                string label = emit(nowlist[i]->Label->op, nowlist[i]->Label->src1, nowlist[i]->Label->src2, nowlist[i]->Label->res);
                out << label << endl;   // 标签
                for(int m=0;m<nowlist[i]->block_ir.size();m++)
                {
                    string ir=emit(nowlist[i]->block_ir[m]->op,nowlist[i]->block_ir[m]->src1,nowlist[i]->block_ir[m]->src2,nowlist[i]->block_ir[m]->res);
                    out << ir << endl;  // 普通语句
                }
                string jump=emit(nowlist[i]->Jump->op,nowlist[i]->Jump->src1,nowlist[i]->Jump->src2,nowlist[i]->Jump->res);
                out << jump << endl;    // 跳转
            }
        }
        out.close();
    }
    system(file.c_str());
}
// 用于基本块计数
int block_count;
// 打印控制流图的基本块
void print_cfg_node(FILE* fp)
{
    auto global_block_ir = block_ir_list[0][0]->block_ir;
    for(int i=0;i<ir_alloc_list[0].size();i++)
    {   // 全局变量声明
        fprintf_s(fp,"(%s,%s,%s,%s)\n", ir_alloc_list[0][i]->opstring.c_str(), 
        ir_alloc_list[0][i]->src1.c_str(), ir_alloc_list[0][i]->src2.c_str(), ir_alloc_list[0][i]->res.c_str());
    }
    for(int i=0;i<global_block_ir.size();i++) 
    {   // 全局变量相关语句
        string ir = emit(global_block_ir[i]->op, global_block_ir[i]->src1, global_block_ir[i]->src2, global_block_ir[i]->res);
        fprintf_s(fp, "%s\n", ir.c_str());
    }
    fprintf_s(fp, "\"]\n}\n");
    block_count = 1;
    for(int j=1;j<fun_declares.size();j++) 
    {   // 函数入口
        // 每个函数独立一个子图
        fprintf_s(fp, "subgraph cluster%d{\nlabel=\"function %s\"\n", j, fun_names[j-1].c_str());
        fprintf_s(fp, "node%d [label=\"", block_count++);
        for(int i=0;i<ir_alloc_list[j].size();i++)
        {   // 函数入口
            // 局部变量声明
            fprintf_s(fp,"(%s,%s,%s,%s)\n", ir_alloc_list[j][i]->opstring.c_str(), 
            ir_alloc_list[j][i]->src1.c_str(), ir_alloc_list[j][i]->src2.c_str(), ir_alloc_list[j][i]->res.c_str());
        }
        fprintf_s(fp, "\"]\n");
        auto nowlist = block_ir_list[j];
        for (int i = 0; i < nowlist.size(); i++) 
        {
            fprintf_s(fp, "node%d [label=\"", block_count++);
            // 打印标签语句
            fprintf_s(fp,"(%s,%s,%s,%s)\n", nowlist[i]->Label->opstring.c_str()
            , nowlist[i]->Label->src1.c_str(), nowlist[i]->Label->src2.c_str(), nowlist[i]->Label->res.c_str());
            for (int m = 0; m < nowlist[i]->block_ir.size(); m++) 
            {   // 打印普通语句
                fprintf_s(fp,"(%s,%s,%s,%s)\n", nowlist[i]->block_ir[m]->opstring.c_str(), 
                nowlist[i]->block_ir[m]->src1.c_str(), nowlist[i]->block_ir[m]->src2.c_str(), nowlist[i]->block_ir[m]->res.c_str());
            }
            // 打印跳转语句
            fprintf_s(fp,"(%s,%s,%s,%s)\n", nowlist[i]->Jump->opstring.c_str(), nowlist[i]->Jump->src1.c_str(),
            nowlist[i]->Jump->src2.c_str(), nowlist[i]->Jump->res.c_str());
            fprintf_s(fp, "\"]\n");
        }
        fprintf_s(fp, "}\n");
        // 函数出口
    }
}
// 打印控制流图的基本块之间的有向边
void print_cfg_edge(FILE* fp)
{
    // fprintf_s(fp,"node0->node1;\n");
    // 直接从函数开始
    block_count = 1;
    for(int j=1;j<fun_declares.size();j++) 
    {   // 函数内部
        fprintf_s(fp, "node%d->node%d\n", block_count, block_count+1);
        block_count++;
        auto nowlist = block_ir_list[j];
        for (int i = 0; i < nowlist.size(); i++) {
            const char* jump = nowlist[i]->Jump->opstring.c_str();
            if(!strcmp(jump, "bc")) {                                       // 有条件跳转
                const char* jump_label1 = nowlist[i]->Jump->src1.c_str();   // 跳转地址1
                const char* jump_label2 = nowlist[i]->Jump->src2.c_str();   // 跳转地址2
                for (int k = 1; k < nowlist.size(); k++) {                  // 遍历函数内部的标签
                    const char* label = nowlist[k]->Label->src1.c_str();    // 标签
                    if(!strcmp(jump_label1, label) || !strcmp(jump_label2, label))
                    {   // 跳转地址和标签匹配则打印有向边
                        fprintf_s(fp, "node%d->node%d\n", block_count+i, block_count+k);
                    }
                }
            } else if(!strcmp(jump, "br")) {                                // 无条件跳转
                const char* jump_label = nowlist[i]->Jump->src1.c_str();    // 跳转地址
                for (int k = 1; k < nowlist.size(); k++) {                  // 遍历函数内部的标签
                    const char* label = nowlist[k]->Label->src1.c_str();    // 标签
                    if(!strcmp(jump_label, label))
                    {   // 跳转地址和标签匹配则打印有向边
                        fprintf_s(fp, "node%d->node%d\n", block_count+i, block_count+k);
                    }
                }
            }
        }
        block_count += nowlist.size();  // 记录函数基本块数量
    }
}
// 打印控制流图
void print_cfg_to_file(string file)
{
    FILE* fp;
	fopen_s(&fp, "cfg.gv", "w+");   // 新建并打开一个用于graphviz画图的中间文件
    fprintf_s(fp, "digraph graphname {\nnode[shape=box]\n");    // 结点形状为方形
    fprintf_s(fp, "subgraph cluster0{\nlabel=\"globle var\"\nnode0 [label=\""); // 全局变量子图
    print_cfg_node(fp);
    print_cfg_edge(fp);
    fprintf_s(fp, "}\n");
    fclose(fp);
	string command = "dot cfg.gv -Tpng -o " + file; // 用graphviz命令画图
    system(command.c_str());    // 执行画图命令
    system(file.c_str());       // 展示控制流图
}
// 遍历抽象语法树
void dfs(struct ast_node *root)
{
    string result,first_declare;
    stack_table_init();                     // 符号表初始化
    fun_declares.push_back(first_declare);  // 函数
    list newlist;
    ir_alloc_list.push_back(newlist);       // 声明语句
    ir_lists.push_back(newlist);            // 运算语句
    for(int i=0;i<root->sons.size();i++)
    {
        if((root->sons[i])->type==VarDecl_||root->sons[i]->type==ConstDecl_)
            isglobal=1; // 全局变量
        else
            isglobal=0; // 局部变量
        result=ast_visit_node(root->sons[i]);   // 访问抽象语法树子结点
    }
    pass_block();
}
// 根据命令行参数选择生成抽象语法树,符号表,四元式,中间IR,控制流图
void print_to_file(struct ast_node *root)
{
    if(printASTToFile)
    {
        print_ast_to_file(outputPath);
        return;
    }
    dfs(root);  //遍历抽象语法树
    if(printIrToFile) {
        print_ir_to_file(outputPath);               // 中间IR
    } else if(printSTToFile) {
        print_symbol_talbe_to_file(outputPath);     // 符号表
    } else if(printQuadrupleToFile) {
        print_quadruple_to_file(outputPath);        // 四元式
    } else if(printCFGToFile) {
        print_cfg_to_file(outputPath);              // 控制流图
    }
}
