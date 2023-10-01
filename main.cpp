#include <stdio.h>
#include <string>

#include "sysy_lex.h"
#include "sysy_yacc.h"
#include "dfs.h"

using namespace std;

string outputPath;                  // C语言源文件名
string sourcePath;                  // 输出文件名
bool printIrToFile = false;         // 生成中间IR
bool printSTToFile = false;         // 生成符号表
bool printASTToFile = false;        // 生成抽象语法树
bool printQuadrupleToFile = false;  // 生成四元式
bool printCFGToFile = false;        // 生成控制流图
void read_arg(char* arg);           // 分析命令行参数
extern int yyparse(void);           // 建抽象语法树

int main(int argc, char *argv[]) 
{
    if(argc >= 4) {
        for(int i = 1; i< argc; i++) {
            read_arg(argv[i]);
        }
    } else {
        printf("wrong command!");   // 输入命令格式错误
        return -1;
    }
    // 若指定有参数，则作为词法分析的输入文件
    if ((yyin = fopen(sourcePath.c_str(), "r")) == NULL) {
        printf("Can't open file %s\n", sourcePath.c_str()); //C语言源文件打开失败
        return -1;
    }
    // 词法、语法分析生成抽象语法树AST
    int result = yyparse();
    // result为0说明词法、语法分析顺利完成
    // 根据命令行参数选择生成抽象语法树,符号表,四元式,中间IR,控制流图
    if(0 == result) {
        print_to_file(ast_root);
    }
    return 0;
}

// 分析命令行参数
void read_arg(char* cur_arg) {
    static char* last_arg="";
    if(strcmp(cur_arg, "-i") == 0) {
        printIrToFile = true;               // 生成中间IR
    } else if(strcmp(cur_arg, "-s") == 0) {
        printSTToFile = true;               // 生成符号表
    } else if(strcmp(cur_arg, "-a") == 0) {
        printASTToFile = true;              // 生成抽象语法树
    } else if(strcmp(cur_arg, "-q") == 0) {
        printQuadrupleToFile = true;        // 生成四元式
    } else if(strcmp(cur_arg, "-c") == 0) {
        printCFGToFile = true;              // 生成控制流图
    } else if(strcmp(cur_arg, "-o") == 0) {
        if(strcmp(last_arg, "-i") && strcmp(last_arg, "-s") && strcmp(last_arg, "-a") && strcmp(last_arg, "-q") && strcmp(last_arg, "-c")) {
            printf("wrong command!");       // 输入命令错误
            return;
        }
    } else {
        if(strcmp(last_arg, "-o") == 0) {
            outputPath = string(cur_arg);       // 输出文件名
        } else {
            sourcePath = string(cur_arg);       // C语言源文件名
        }
    }
    last_arg = cur_arg;     //记录上一个参数
}
