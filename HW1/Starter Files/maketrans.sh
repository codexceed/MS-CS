#!/bin/sh

set -e  # Exit on error


bison -d trans.starter.y
flex trans.starter.l
g++ lex.yy.c trans.starter.tab.c treenode.cpp
