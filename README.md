## Repo Info

This repo has two branches, `master` and `dev`. `master` currently has the scanner, parser, and type checking fully implemented. Code generation is currently in progress on the `dev` repo and is not fully functional yet.

## Build Requirements
A C/C++ compiler, LLVM (for code generation on `dev`), should build on any standard Linux box, developed on Debian WSL

## Build Instructions
1. `mkdir build`
2. `cd build`
3. `cmake ..`
4. `make`
5. `./compiler <path/to/source_file>`