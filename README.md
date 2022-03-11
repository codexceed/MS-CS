# Programming Languages Homework 1 Problem 5 Solution | (E)BNF Parser
**Submitted By**: Sarthak Joshi
**ID**: N15526247
## What this package does?
The compiled output of the **HW1/Translator/N15526247.trans.l** and **HW1/Translator/N15526247.trans.y** files using **Flex** and **Bison** respectively will generate a parser capable to ingesting and validate the BNF grammar on the input bison file and output the validated grammar.

## How to run this?
### Compile
Navigate to **HW1/Translator** and run the following.
```bash
./N15526247.trans.sh
```
### Run the executable
```bash
./a.out < N15526247.tinybasic.in > N15526247.tinybasic.y
```
### Check the output file
The output file **N15526247.tinybasic.y** should contain the non-grammar blocks of the input file, with the grammar lines printed as follows
```
%token LPAREN RPAREN
%token AND OR NOT
%token <val> BOOL


%type <val> expr

%left OR
%left AND
%left NOT

%%

                             
     

                          
%%=================This is the BNF Grammar output=================
%%
 prog : expr 


   ; expr : expr OR expr 




  | expr AND expr 



  | NOT expr 


  | BOOL 

  | LPAREN expr RPAREN 



   ; 
%%
 ==========================End of output=========================

int main()
{
    yyparse(); // A parsing function that will be generated by Bison.
    return 0;
}

std::string printer (bool x) {
    return x ? "true" : "false";
}
```
>**Note to the grader:**
    As of this moment:
    - This package does **not**
    - Contain a translator script
    - Contain a flex file for EBNF
    - Convert EBNF to BNF
    - The only progress so far has been w.r.t making the starter code work with normal BNF grammar from a valid bison file and interpret it correctly.
## Work in Progress
- EBNF to BNF conversion
- Valid bison file output

## Troubleshooting
- **I'm unable to execute the `N15526247.trans.sh` script**
Try modifying permissions for the script. You need the script to have execution permissions to run. You can do it via `chmod 755 N15526247.trans.sh`.