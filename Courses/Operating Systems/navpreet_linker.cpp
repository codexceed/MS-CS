/*
Submitted By:
Navpreet Singh
ns4767
*/

#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <stdlib.h>
#include <string>
#include <iomanip>


using namespace std;

struct sym{
    string name;
    int mod;
    int relval;
    int absval;
    bool mul_def = false;
    bool used = false;
};

struct token{
    string name;
    int linenum;
    int pos;
};

vector<struct sym> syms;
int modul;

void __parseerror(int errcode, struct token tok) {
    string errstr[] = {
    "NUM_EXPECTED", // Number expect, anything >= 2^30 is not a number either
    "SYM_EXPECTED", // Symbol Expected
    "ADDR_EXPECTED", // Addressing Expected which is A/E/I/R
    "SYM_TOO_LONG", // Symbol Name is too long
    "TOO_MANY_DEF_IN_MODULE", // > 16
    "TOO_MANY_USE_IN_MODULE", // > 16
    "TOO_MANY_INSTR", // total num_instr exceeds memory size (512)
    };
    cout<<"Parse Error line "<<tok.linenum<<" offset "<<tok.pos<<": "<<errstr[errcode]<<"\n";
    exit(0);
}

vector<struct token> tokens;

void getToken(vector<string> lines){

    vector<string>:: iterator iter = lines.begin();
    vector<string>:: iterator end = lines.end();
    int lineno=0;
    int pos;
    char *ltok;
    int lline;
    while(iter!=end){
        lineno++;
        char *line = &*(*iter).begin();
        char *token = strtok(line," \t\n");
        while(token!=NULL){
            struct token tok;
            pos = token-line+1;
            tok.name = token;
            tok.linenum = lineno;
            tok.pos = pos;
            tokens.push_back(tok);
            ltok = token;
            lline = lineno;
            token = strtok (NULL," \t\n");
        }
        iter++;
    }
    if (lline==lineno){
        pos = pos + strlen(ltok);
    }
    else{
        pos = 1;
    }
    
    struct token tok;
    tok.name = "##eof##";
    tok.linenum = lineno;
    tok.pos = pos;
    tokens.push_back(tok);
}

int symvalue(string symbol){

    for(int i=0;i<syms.size();i++){
        if (syms[i].name == symbol){
            return syms[i].absval;
        }
    }
    //Rule3
    return 0;
}

int readInt(struct token tok){
    string s = tok.name;
    
    for (char c: s){
        if (isdigit(c)==false){
            __parseerror(0, tok);
        }
    }
    
    long n = stol(tok.name);

    if (n>=1073741824){
        __parseerror(0, tok);
    }
    n = (int) n;
    return n;
}

string readSymbol(struct token tok){
    string s = tok.name;
    char c1 = s[0];
    if (isdigit(c1)){
        __parseerror(1, tok);
    }
    for (char c: s){
        if (isalnum(c)==0){
            __parseerror(1, tok);
        }
    }
    if (s.length()>16){
        __parseerror(3, tok);
    }

    return s;
}

bool repeatSym(string symbol){

    for(int i=0;i<syms.size();i++){
        //Rule 2
        if (syms[i].name == symbol){
            syms[i].mul_def = true;
            return true;
        }
    }
    return false;
}

string readIAER(struct token tok){
    string s = tok.name;
    if (s=="I" || s=="A" || s=="E" || s=="R"){
        return s;
    }
    else{
        __parseerror(2, tok);
    }
}

bool checkOp(int instr, int cnt){
    //Rule 11
    if (instr/1000 >= 10){
        cout<<setfill('0') << std::setw(3) <<cnt<<": 9999 Error: Illegal opcode; treated as 9999\n";
        return true;
    }
    return false;
}

bool checkImm(int instr, int cnt){
    //Rule 10
    if (instr >= 10000){
        cout<<setfill('0') << std::setw(3) <<cnt<<": 9999 Error: Illegal immediate value; treated as 9999\n";
        return true;
    }
    return false;
}

void instr_I(int instr, int cnt){
    cout<<setfill('0') << std::setw(3) <<cnt<<": "<< setfill('0') << std::setw(4) <<instr<<"\n";
}

void instr_A(int instr, int cnt, string errormsg){
    if (errormsg=="-1"){
        cout<<setfill('0') << std::setw(3) <<cnt<<": "<< setfill('0') << std::setw(4) <<instr<<"\n";
    }
    else{
        cout<<setfill('0') << std::setw(3) <<cnt<<": "<< setfill('0') << std::setw(4) <<instr<<" "<<errormsg<<"\n";
    }
    
}

bool checkE(string symbol){
    for(int i=0;i<syms.size();i++){
        if (syms[i].name == symbol){
            syms[i].used = true; //Rule 4
            return false;
        }
    }
    //Rule3
    return true;
}

void instr_E(int instr, int cnt, string errormsg){
    if (errormsg == "-1"){
        cout<< setfill('0') << std::setw(3) <<cnt<<": "<< setfill('0') << std::setw(4) <<instr<<"\n";
    }
    else{
        cout<< setfill('0') << std::setw(3) <<cnt<<": "<< setfill('0') << std::setw(4) <<instr<<" "<<errormsg<<"\n";
    }
}

void instr_R(int instr, int cnt, string errormsg){
    if (errormsg == "-1"){
        cout<< setfill('0') << std::setw(3) <<cnt<<": "<< setfill('0') << std::setw(4) <<instr<<"\n";
    }
    else{
        cout<< setfill('0') << std::setw(3) <<cnt<<": "<< setfill('0') << std::setw(4) <<instr<<" "<<errormsg<<"\n";
    }

}

void parse1(){

    int defcount,usecount,codecount;
    struct token tok;
    int mod = 0;
    int it = 0;
    int tot_instr = 0;

    for(it;it<tokens.size();it++){

        tok = tokens[it];
        if (tok.name=="##eof##"){
            break;
        }
        mod += 1;
        defcount = readInt(tok);
        if (defcount>16){
            __parseerror(4, tok);
        }
        
        for(int i=0;i<defcount;i++){
            it++;
            tok = tokens[it];
            bool def = repeatSym(readSymbol(tok));
            if (def==true){
                it++;
                tok = tokens[it];
            }
            else{
                struct sym symb;
                symb.name = readSymbol(tok);
                symb.mod = mod;
                it++;
                tok = tokens[it];
                symb.relval = readInt(tok); 
                symb.absval = modul+ symb.relval;
                syms.push_back(symb);
            }
            
        }
        it++;
        tok = tokens[it];
        usecount = readInt(tok);

        if (usecount>16){
            __parseerror(5, tok);
        }

        for(int i=0;i<usecount;i++){
            it++;
            tok = tokens[it];
            string tmp = readSymbol(tok);
        }
        it++;
        tok = tokens[it];
        codecount = readInt(tok);
        tot_instr += codecount;
        if (tot_instr>512){
            __parseerror(6, tok);
        }

        //Rule 5
        for(int i=0;i<syms.size();i++){
            if (syms[i].mod==mod){
                if (syms[i].relval>codecount-1){
                    cout<<"Warning: Module "<<syms[i].mod<<": "<<syms[i].name<<" too big "<<syms[i].relval<<" (max="<<codecount-1<<") assume zero relative\n";
                    syms[i].relval = 0;
                    syms[i].absval = modul+ syms[i].relval;
                }
            }
        }
        for(int i=0;i<codecount;i++){
            it++;
            tok = tokens[it];
            string tmp = readIAER(tok);
            it++;
            tok = tokens[it];
            int tmp1 = readInt(tok);
        }
        modul += codecount;
    }

}

void parse2(){

    int defcount,usecount,codecount;
    struct token tok;
    int cnt=0;
    string instr_type;
    int instr;
    int it = 0;
    int mod = 0;
    for(it;it<tokens.size();it++){
        
        mod += 1;
        tok = tokens[it];
        if (tok.name=="##eof##"){
            break;
        }
        defcount = readInt(tok);
        
        for(int i=0;i<defcount;i++){
            it++;
            tok = tokens[it];
            it++;
            tok = tokens[it];
        }
        it++;
        tok = tokens[it];
        usecount = readInt(tok);
        vector<string> usecount_syms;
        for(int i=0;i<usecount;i++){
            it++;
            tok = tokens[it];
            usecount_syms.push_back(readSymbol(tok));
        }
        it++;
        tok = tokens[it];
        codecount = readInt(tok);
        vector<string> used_syms;
        for(int i=0;i<codecount;i++){

            it++;
            tok = tokens[it];
            instr_type = readIAER(tok);
            it++;
            tok = tokens[it];
            instr = readInt(tok);
            if (instr_type=="I"){
                if (checkImm(instr,cnt) == false){
                    instr_I(instr, cnt);
                }
            }
            else if (instr_type=="A"){
                if (checkOp(instr,cnt) == false){
                    string errormsg = "-1";
                    //Rule 8
                    if (instr%1000 >= 512){
                        instr = instr - instr%1000;
                        errormsg = "Error: Absolute address exceeds machine size; zero used";
                        instr_A(instr, cnt, errormsg);
                    }
                    else{
                        instr_A(instr, cnt, errormsg);
                    }
                    
                }
                
            }
            else if (instr_type=="E"){
                if (checkOp(instr,cnt) == false){
                    int operand = instr%1000;
                    string errormsg = "-1";
                    //Rule 6
                    if (operand>=usecount_syms.size()){
                        errormsg = "Error: External address exceeds length of uselist; treated as immediate";
                        instr_E(instr,cnt,errormsg);
                    }
                    else{
                        string symbol = usecount_syms[operand];
                        used_syms.push_back(symbol);
                        //Rule3
                        if (checkE(symbol)==true){
                            errormsg = "Error: "+symbol+" is not defined; zero used";
                            instr = instr - operand + symvalue(symbol);
                            instr_E(instr, cnt, errormsg);
                        }
                        else{
                            instr = instr - operand + symvalue(symbol);
                            instr_E(instr, cnt, errormsg);
                        }
                        
                    }
                }
            }
            else if (instr_type=="R"){
                if (checkOp(instr,cnt) == false){
                    int module_size = modul+codecount;
                    string errormsg = "-1";
                    //Rule 9
                    if (instr%1000>=module_size){
                        instr = modul + instr - instr%1000;
                        errormsg = "Error: Relative address exceeds module size; zero used";
                        instr_R(instr, cnt, errormsg);
                    }
                    else{
                        instr = modul + instr;
                        instr_R(instr, cnt, errormsg);
                    }
                    
                }
            }
            cnt++;

        }
        //Rule 7
        for(int i=0;i<usecount_syms.size();i++){
            bool found = false;
            for (int j=0;j<used_syms.size();j++){
                if (usecount_syms[i]==used_syms[j]){
                    found = true;
                    break;
                }
            }
            if (found==false){
                cout<<"Warning: Module "<<mod<<": "<<usecount_syms[i]<<" appeared in the uselist but was not actually used\n";
            }
        }
        modul += codecount;
    }
    //Rule 4
    for(int i=0;i<syms.size();i++){

        if (syms[i].used==false){
            cout<<"Warning: Module "<<syms[i].mod<<": "<<syms[i].name<<" was defined but never used\n";
        }
    }

}

int main(int argc,char** argv){

    fstream file;
    vector<string> lines;

    if (argc==1){
        cout<<"Expected argument after options\n";
        exit(0);
    }

    //begin parse 1
    file.open(argv[1],ios::in);
    if (!file){
        cout<<"Not a valid inputfile <"<<argv[1]<<">\n";
        exit(0);
    }
    if (file.is_open()){  
        string line;
        while(getline(file, line)){     
            lines.push_back(line);
        }
        file.close(); 
        getToken(lines);
    }

    
    modul=0;
    parse1();

    //parse 1 end. Print symbol table
    cout<<"Symbol Table\n";

    for(int i=0;i<syms.size();i++){
        if (syms[i].mul_def==true){
            cout<<syms[i].name<<"="<<syms[i].absval<<" Error: This variable is multiple times defined; first value used\n";
        }
        else{
            cout<<syms[i].name<<"="<<syms[i].absval<<"\n";
        }
        
    }
    

    // begin parse 2 and print memory map while parsing.
    file.open(argv[1],ios::in);
    if (file.is_open()){  
        string line;
        while(getline(file, line)){     
            lines.push_back(line);
        }
        file.close(); 
    }
    modul=0;
    cout<<"Memory Map\n";
    parse2();

}