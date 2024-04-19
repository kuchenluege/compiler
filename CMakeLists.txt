cmake_minimum_required(VERSION 3.8)

project(compiler VERSION 0.1
                 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(C_STANDARD_REQUIRED YES)
message(STATUS "Using C compiler ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")

add_library(hash_table STATIC src/hash_table.c)
target_include_directories(hash_table PUBLIC include)

add_library(token STATIC src/token.c)
target_include_directories(token PUBLIC include)

add_library(symbol_table_chain STATIC src/symbol_table_chain.c)
target_include_directories(symbol_table_chain PUBLIC include)
target_link_libraries(symbol_table_chain PUBLIC hash_table
                                                token)

add_library(error STATIC src/error.c)
target_include_directories(error PUBLIC include)

add_library(scanner STATIC src/scanner.c)
target_include_directories(scanner PUBLIC include)
target_link_libraries(scanner PUBLIC symbol_table_chain
								     error)

add_executable(${PROJECT_NAME} app/compiler.c)
target_link_libraries(${PROJECT_NAME} scanner)