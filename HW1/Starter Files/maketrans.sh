#!/bin/sh

bison -d trans.y
flex trans.l
g++ lex.yy.c trans.tab.c treenode.cpp
