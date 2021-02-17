#!/bin/sh

set -e  # Exit on error


bison -d N15526247.trans.y
flex N15526247.trans.l
g++ lex.yy.c N15526247.trans.tab.c treenode.cpp


rm lex.yy.c N15526247.trans.tab.h N15526247.trans.tab.c