add_library(utils utils/debug_assert.cpp ${CMAKE_SOURCE_DIR}/include/debug_assert.h)
target_include_directories(utils PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_compile_options(utils PUBLIC -Og -fsanitize=address,undefined -fsanitize-address-use-after-scope)
target_link_options(utils PUBLIC -fsanitize=address,undefined)

add_library(cool-support)
target_sources(cool-support
PRIVATE
    cool-support/ast_lex.cpp
    cool-support/ast_parse.cpp
    cool-support/cool_tree.cpp
    cool-support/dumptype.cpp
    cool-support/stringtab.cpp
    cool-support/tree.cpp
    cool-support/utils.cpp
PUBLIC
    "${CMAKE_SOURCE_DIR}/include/cool-support/ast_parse.h"
    "${CMAKE_SOURCE_DIR}/include/cool-support/cool_tree.h"
    "${CMAKE_SOURCE_DIR}/include/cool-support/copyright.h"
    "${CMAKE_SOURCE_DIR}/include/cool-support/stringtab.h"
    "${CMAKE_SOURCE_DIR}/include/cool-support/symtab.h"
    "${CMAKE_SOURCE_DIR}/include/cool-support/tree.h"
    "${CMAKE_SOURCE_DIR}/include/cool-support/utils.h"
    "${CMAKE_SOURCE_DIR}/include/cool-support/symbol.h"
    "${CMAKE_SOURCE_DIR}/include/cool_tree.handcode.h"
    "${CMAKE_SOURCE_DIR}/include/stringtab.handcode.h"
)
target_include_directories(cool-support PUBLIC "${CMAKE_SOURCE_DIR}/include/cool-support" "${CMAKE_SOURCE_DIR}/include")
target_compile_options(cool-support PRIVATE -O3 -UNDEBUG -Wno-write-strings -Wno-register -fsanitize=address,undefined -fsanitize-address-use-after-scope)
target_link_libraries(cool-support PUBLIC utils)
target_link_options(cool-support PRIVATE -fsanitize=address,undefined)

add_library(lab3-support)
target_sources(lab3-support
PRIVATE
        CgenClassTable.cpp
        CgenEnvironment.cpp
        CgenNode.cpp
PUBLIC
        "${CMAKE_SOURCE_DIR}/include/CgenClassTable.h"
        "${CMAKE_SOURCE_DIR}/include/CgenEnvironment.h"
        "${CMAKE_SOURCE_DIR}/include/CgenNode.h"
)
target_include_directories(lab3-support PUBLIC "${CMAKE_SOURCE_DIR}/include")
target_link_libraries(lab3-support PUBLIC cool-support LLVM)
target_compile_options(lab3-support PRIVATE -Og -fsanitize=address,undefined -fsanitize-address-use-after-scope)
target_link_options(lab3-support PRIVATE -fsanitize=address,undefined)

add_library(lab3)
target_sources(lab3
PRIVATE
    cgen.cpp
)
target_include_directories(lab3 PUBLIC "${CMAKE_SOURCE_DIR}/include")
target_link_libraries(lab3 PUBLIC lab3-support)
target_compile_options(lab3 PRIVATE -fsanitize=address,undefined -fsanitize-address-use-after-scope)
target_link_options(lab3 PRIVATE -fsanitize=address,undefined)
target_precompile_headers(lab3 PRIVATE
    <llvm/Support/FileSystem.h>
    <llvm/ADT/STLExtras.h>
    <llvm/IR/IRBuilder.h>
    <llvm/IR/NoFolder.h>
    <llvm/Support/raw_ostream.h>
    <sstream>
    <iostream>
    <string>
    <memory>
)


