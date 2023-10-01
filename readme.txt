# 编译器代码构成说明

sysy.l              借助flex工具实现的词法分析器脚本源代码
sysy.cpp            借助flex工具实现的词法分析器自动生成C语言源文件
sysy.h              借助flex工具实现的词法分析器自动生成C语言头文件

sysy.y              借助yacc/bison工具实现的语法分析器脚本源代码
sysy.cpp            借助yacc/bison工具实现的语法分析器自动生成C语言源文件
sysy.h              借助yacc/bison工具实现的语法分析器自动生成C语言头文件

ast.cpp             抽象语法树创建所需要的函数
ast.h               对应的头文件

symboltable.cpp     符号表创建所需要的函数
symboltable.h       对应的头文件

block.cpp           基本块创建所需要的函数
block.h             对应的头文件

dfs.cpp             遍历抽象语法树生成抽象语法树,符号表,四元式,控制流图,中间IR
dfs.h               对应的头文件

main.cpp            编译器程序的主函数
test.c              测试用例代码

# 编译器使用方法
运行平台：Windows
编译方法：cmake（具体编译过程见CMakeList.txt）
## 生成抽象语法树
./cmake-build-debug/compiler -a -o 输出文件 C语言源文件
## 生成符号表
./cmake-build-debug/compiler -s -o 输出文件 C语言源文件
## 生成四元式
./cmake-build-debug/compiler -q -o 输出文件 C语言源文件
## 生成中间IR
./cmake-build-debug/compiler -i -o 输出文件 C语言源文件
## 生成控制流图
./cmake-build-debug/compiler -c -o 输出文件 C语言源文件