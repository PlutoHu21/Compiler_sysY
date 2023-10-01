#include"block.h"
#include<vector>
#include<map>
#include<utility>

vector<fun_block_list> block_ir_list;//一个程序各个函数的基本块集合
vector<block_graph> all_block_graph;//一个程序的所有流图
vector<int>visited;//表示是否来过该节点
//划分基本块和删除无用的跳转指令,同时生成有向图
void basic_block()
{
    //全局变量的基本块
    fun_block_list zero_block;
    struct block *block=new struct block();
    for(int i=0;i<ir_lists[0].size();i++) {
        block->block_ir.push_back(ir_lists[0][i]);
    }
    zero_block.push_back(block);
    block_ir_list.push_back(zero_block);
    for(int i=0;i<ir_lists[0].size();i++) {
        block->block_ir.push_back(ir_lists[0][i]);
    }
    for(int i=1;i<ir_lists.size();i++) {
        fun_block_list newblock;//一个函数的所有block块的集合
        block_graph this_block_graph;//为当前函数建立一个流图
        //对每个函数块进行遍历
        auto nowlist=ir_lists[i];
        block=new struct block();
        for(int j=0;j<nowlist.size();j++) {
            //如果是碰到新的标签就把这个标签当成这个块语句的标签
            if(nowlist[j]->op==Label_||nowlist[j]->op==Entry_) {
                block->Label=nowlist[j];
            } else if(nowlist[j]->op==Br_||nowlist[j]->op==Bc_||nowlist[j]->op==Exit_) {
                if(nowlist[j]->op==Bc_) {
                    //记录待删除的标签都被哪些行语句使用，以后做替换,真假标签都存进去
                    auto label_info=label_map.find(nowlist[j]->src1);
                    label_info->second->use_list.push_back(nowlist[j]->line_no);
                 auto label_info1=label_map.find(nowlist[j]->src2);
                 label_info1->second->use_list.push_back(nowlist[j]->line_no);

                 block->Jump=nowlist[j];
                 newblock.push_back(block);
                 block=new struct block();
                } else if(nowlist[j]->op==Br_) {
                    //判断是否为无用的跳转标签
                    if(block->block_ir.size()==0) {
                        string temp=block->Label->src1;
                        auto delete_list=label_map.find(block->Label->src1)->second->use_list;
                        for(int m=0;m<delete_list.size();m++) {
                            auto item=ir_map.find(delete_list[m])->second;//要被替换的那一项
                            if(item->op==Br_) {
                                if(item->src1==temp) {
                                    item->src1 = nowlist[j]->src1;
                                } else {
                                    continue;
                                }
                            } else {
                                if(item->src1==temp) {
                                    item->src1 = nowlist[j]->src1;
                                } else {
                                    item->src2 = nowlist[j]->src1;
                                }
                            }
                        }
                        auto label_info=label_map.find(nowlist[j]->src1);
                        label_info->second->use_list.insert(label_info->second->use_list.end(),delete_list.begin(),delete_list.end());
                    } else {
                        auto label_info=label_map.find(nowlist[j]->src1);
                        label_info->second->use_list.push_back(nowlist[j]->line_no);
                        block->Jump=nowlist[j];
                        newblock.push_back(block);
                        block=new struct block();
                    }

                } else {
                    block->Jump=nowlist[j];
                    newblock.push_back(block);
                    block=new struct block();
                }
            } else {   
                block->block_ir.push_back(nowlist[j]);  //语句直接pushback
            }
        }
        block_ir_list.push_back(newblock);
    }
}
//初始化流图
void init_graph()
{
    //为第一个节点建立流图
    block_graph  zero_block_graph;
    struct graph_node *zero_block_graph_node=new struct graph_node();
    zero_block_graph_node->index=0;
    zero_block_graph.push_back(zero_block_graph_node);
    all_block_graph.push_back(zero_block_graph);
    for(int i=1;i<block_ir_list.size();i++) {
        map<int,int>index_rel_label;
        fun_block_list this_fun_block=block_ir_list[i];//每个函数的所有基本块
        block_graph this_fun_graph;//当前函数的流图
        for(int j=0;j<this_fun_block.size();j++) {
            //建立节点
            struct graph_node *node=new struct graph_node();
            auto label = this_fun_block[j]->Label;
            if (label->op == Entry_) {
                node->index = 0;
                node->flag=1;
            } else {
                string temp = label->src1;
                node->index = atoi(temp.substr(2).c_str());
            }
            index_rel_label.insert(make_pair(node->index,j));
            node->block=this_fun_block[j];
            this_fun_graph.push_back(node);
        }
        //添加后继节点
        for(int j=0;j<this_fun_graph.size();j++) {
            auto jump=this_fun_block[j]->Jump;
            if(jump->op==Exit_) {
                this_fun_graph[j]->flag=1;
            } else if(jump->op==Br_) {
                string temp=jump->src1;
                int next= atoi(temp.substr(2).c_str());
                int index=index_rel_label.find(next)->second;
                this_fun_graph[j]->next_nodes.push_back(this_fun_graph[index]);
                this_fun_graph[index]->front_nodes.push_back(this_fun_graph[j]);
            } else {
                string temp;
                temp=jump->src1;
                int next= atoi(temp.substr(2).c_str());
                int index=index_rel_label.find(next)->second;
                this_fun_graph[j]->next_nodes.push_back(this_fun_graph[index]);
                this_fun_graph[index]->front_nodes.push_back(this_fun_graph[j]);
                temp=jump->src2;
                next= atoi(temp.substr(2).c_str());
                index=index_rel_label.find(next)->second;
                this_fun_graph[j]->next_nodes.push_back(this_fun_graph[index]);
                this_fun_graph[index]->front_nodes.push_back(this_fun_graph[j]);
            }
        }
        all_block_graph.push_back(this_fun_graph);
    }
}
//删除死基本块
void delete_dead_block()
{
    for(int i=1;i<all_block_graph.size();i++) {
        auto block_graph =all_block_graph[i];
        for(int j=0;j<block_graph.size();j++) {
            if(block_graph[j]->front_nodes.size()==0) {
                if (block_graph[j]->flag != 1) {
                    if (block_graph[j]->flag != 1) {
                        auto it = block_ir_list[i].begin() + j;
                        auto it1 = block_graph.begin() + j;
                        block_ir_list[i].erase(it);
                        block_graph.erase(it1);
                    }
                }
            }
        }
        int c=1;
    }
}
// 调用以上函数
void pass_block()
{
    basic_block();
    init_graph();
    delete_dead_block();
}