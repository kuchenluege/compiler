cmake_minimum_required(VERSION 3.8)

project(compiler VERSION 0.1
                 LANGUAGES C)

add_library(hash_table STATIC src/hash_table.c)
target_include_directories(hash_table PUBLIC include)

add_library(token INTERFACE)
target_include_directories(token SYSTEM INTERFACE include)

add_library(error STATIC src/error.c)
target_include_directories(error PUBLIC include)

add_library(symbol_table_chain STATIC src/symbol_table_chain.c)
target_include_directories(symbol_table_chain PUBLIC include)
target_link_libraries(symbol_table_chain PUBLIC hash_table)

add_executable(compiler app/compiler.c)
target_link_libraries(compiler PUBLIC symbol_table_chain
									  token
									  error)