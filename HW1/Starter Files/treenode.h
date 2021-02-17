#ifndef TREENODE_H
#define TREENODE_H

#include <iostream>
#include <string>

using namespace std;

#define MAX_RULENAME 25
#define MAX_CHILD 25
#define NULL_STRING_VAL "NULL"
#define EMPTY_VAL " /* empty */"

namespace asttree {

class treenode;
class expr;
class statement;
class statements;
class rest;
class terminal;
class quant;
class prog;
class empty;
class BNFConverter;
class treenode;



  /*************************************************************
    class  BNFConverter

    Contains the rules for converting an EBNF grammar to BNF.

   *************************************************************/

 class BNFConverter
{
public: 
   static treenode* statementNode;

   static void doGrouping(expr* target, treenode* rhs);

   static void doZeroOrOne(expr* target, treenode* rhs);
  
   static void doZeroOrMore(expr* target, treenode* rhs);
 
   static void doOneOrMore(expr* target, treenode* rhs);
};



  /*************************************************************
    class  treenode

    Represents a node in the abstract syntax tree. The constructor
    overloads correspond to the number of items on the right-hand-side
    of rules in your Bison grammar, regardless of whether they
    are terminals or non-terminals.  We don't construct instances
    of this class directly---use the subclasses instead.

   *************************************************************/

class treenode 
{
   int childNumber;
   treenode* children[MAX_CHILD];
   treenode* parent;

protected:
   int numChildren=0;

public:
   treenode() : numChildren(0), parent(NULL)
   {
     children[0] = NULL;
   }

   treenode(treenode* t1) : numChildren(1), parent(NULL)
   { 
     children[0] = t1;
   }


   treenode(treenode* t1, treenode* t2) : numChildren(2), parent(NULL)
   { 
     children[0] = t1;
     children[1] = t2;
   }


   treenode(treenode* t1, treenode* t2, treenode* t3) : numChildren(3), parent(NULL)
   { 
     children[0] = t1;
     children[1] = t2;
     children[2] = t3;
   }

   treenode(treenode* t1, treenode* t2, treenode* t3, treenode* t4) : numChildren(4), parent(NULL)
   { 
     children[0] = t1;
     children[1] = t2;
     children[2] = t3;
     children[3] = t4;
   }

   void setParent(treenode* p)
   {
      parent = p;
   }

   treenode* getParent() const  { return parent; }


   void replace(treenode* t, treenode* with)
   {
          for (int i=0; i < numChildren; i++)
         {
           if (children[i] != NULL)
              if (children[i] == t)
                {
                 children[i] = with;
                 return;
               }
         }
   }

   virtual void addChild(treenode* t) 
   {
     children[numChildren++] = t;
   }

   treenode* getChild(int c) 
    {
      if (c >= MAX_CHILD || c >= numChildren) 
        return NULL;

      return children[c]; 
    }

   void setChild(int c, treenode* t)
    {
      children[c]=t;
    }


   virtual string value() const { return NULL; }

   virtual string setValue(string s) {}

   virtual void print(ostream& o) const 
    {  
       for (int i=0; i < numChildren; i++)
         {
           if (children[i] != NULL)
             children[i]->print(o);
         }
    }

   virtual void doConversion()
   {
      for (int i=0; i < numChildren; i++)
         {
           if (children[i] != NULL)
            {
             children[i]->setParent(this);
             children[i]->doConversion();
            }
         }
   }

};

/* Subclasses should be named after the left-hand-side of each
   rule in your Bison grammar.  Some limited classes are supplied 
   to you.
*/


class prog : public treenode
{
   public:

   prog(treenode* t1, treenode* s, treenode* t2) : treenode(t1,s,t2) {}

};

class statement : public treenode
{
  public:
  
  statement(treenode* t1, treenode* t2) : treenode(t1, t2) {}
  statement(treenode* t3, treenode* t4, treenode* t5) : treenode(t3, t4, t5) {}

  
     virtual void print(ostream& o) const 
     { 
        treenode::print(o); 
        o << endl << endl;
     }
 
};

class statements: public treenode
{
  public:
  
   statements(statement* s, terminal* t1) : treenode(s, t1) {}
   statements(statement* s1, statements* s2) : treenode(s1,s2) {}
   statements(statement* s3, terminal* t2, statements* s4) : treenode(s3, t2, s4) {}
};



class terminal : public treenode
{
   string s;

public: 
   terminal(string v) : s(v) {}
  
   virtual string setValue(string s1) { s = s1; }

   virtual string value() const { return s; }

    virtual void print(ostream& o) const  { o << s << " "; }
};


/* Fill in the rest of the class implementations here */



} 

#endif

