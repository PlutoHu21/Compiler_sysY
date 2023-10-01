cmake_minimum_required(VERSION 3.13)

project(Compiler VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(PkgConfig REQUIRED)
IF(PKG_CONFIG_FOUND)
    INCLUDE(FindPkgConfig)
    PKG_CHECK_MODULES(LIBGVC libgvc)
ENDIF()

# flex+bison实现计算器例子
add_executable(compiler
    main.cpp
    sysy_lex.cpp
    sysy_lex.h
    sysy_yacc.cpp
    sysy_yacc.h
    ast.cpp
    ast.h
    symboltable.cpp
    symboltable.h
    dfs.cpp
    dfs.h
    block.cpp
    block.h
)

target_compile_options(compiler PRIVATE -Wno-write-strings)
target_include_directories(compiler PRIVATE ${LIBGVC_INCLUDE_DIRS})
target_compile_definitions(compiler PRIVATE ${LIBGVC_DEFINITIONS})
target_link_libraries(compiler  PRIVATE ${LIBGVC_LIBRARIES})
target_link_directories(compiler PRIVATE ${LIBGVC_LIBRARY_DIRS})

# 通过bison生成语法分析源代码
add_custom_command(OUTPUT
    sysy_yacc.cpp sysy_yacc.h
    PRE_BUILD
    COMMAND
    bison -d -o sysy_yacc.cpp --header=sysy_yacc.h ../sysy.y
    DEPENDS
    sysy.y
)

# 通过flex产生词法分析源代码
add_custom_command(OUTPUT
    sysy_lex.cpp sysy_lex.h
    PRE_BUILD
    COMMAND
    flex -o sysy_lex.cpp --header-file=sysy_lex.h ../sysy.l
    DEPENDS
    sysy.l
)