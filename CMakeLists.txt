cmake_minimum_required(VERSION 3.8)

project(compiler VERSION 0.1
                 LANGUAGES C)

add_library(hash_table STATIC src/hash_table.c)
target_include_directories(hash_table PUBLIC include)

add_library(abstract_syntax_tree STATIC src/abstract_syntax_tree.c)
target_include_directories(abstract_syntax_tree PUBLIC include)

add_library(token INTERFACE)
target_include_directories(token SYSTEM INTERFACE include)

add_library(error STATIC src/error.c)
target_include_directories(error PUBLIC include)

add_executable(compiler app/compiler.c)
target_link_libraries(compiler PUBLIC hash_table
                                      abstract_syntax_tree
									  token
									  error)