cmake_minimum_required(VERSION 3.8)

project(compiler VERSION 0.1
                 LANGUAGES C)

add_library(hash_table STATIC src/hash_table.c include/compiler/hash_table.h)
target_include_directories(hash_table PUBLIC include)

add_executable(compiler app/compiler.c)
target_link_libraries(compiler PUBLIC hash_table)