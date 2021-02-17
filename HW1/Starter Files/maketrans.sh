#!/bin/sh

set -e  # Exit on error


bison -d trans.y
flex --debug trans.l
g++ lex.yy.c trans.tab.c treenode.cpp


rm lex.yy.c trans.tab.h trans.tab.c