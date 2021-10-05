#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <vector>
#include <tuple>

using namespace std;

// Declare global entities
struct token{
    string value;
    int line;
    int pos;
};

enum Section { Definition, Use, Program };


// Define functions
vector<struct token> getToken(ifstream &file){
    string line;
    int line_no = 0;
    vector<struct token> tokens;

    while( getline( file, line ) ){
        // Parse new line
        line_no++;
        char* token_line = &line[0];
        char* token_ptr = strtok(token_line, " \t\n");

        // Read tokens
        while (token_ptr != NULL){

            // Create new token object
            struct token new_token;
            new_token.value = token_ptr;
            new_token.line = line_no;
            new_token.pos = token_ptr-token_line+1;

            // Push new token obj to vector list
            tokens.push_back(new_token);

            // Move to next token
            token_ptr  = strtok(NULL, " \t\n");
        }
    }

    return tokens;
    
}

void printSymbolTable(vector< struct token > tokens){

    int curr_module = 0;
    vector< tuple <string, int> > symbol_table;
    Section curr_section = Definition;
    vector<struct token>::iterator it = tokens.begin();

    // Parse the tokens
    while ( it!=tokens.end() ){
        // Get count of current list
        int count = stoi((*it).value);

        if (curr_section == Definition){
            for(int i=0; i<count; i++){
                string symb = (*++it).value;
                int addr = curr_module + stoi((*++it).value);
                symbol_table.push_back(make_tuple(symb, addr));
            }
            curr_section = Use;
        }

        else if (curr_section == Use){
            for(int i = 0; i < count; i++, it++);
            curr_section = Program;
        }

        else if (curr_section == Program){
            for(int i = 0; i < count; i++, it++, it++);
            curr_section = Definition;
        }

        // Keep it moving
        it++;
    }

    // Print the symbol table
    cout << "Symbol Table" << endl << endl;
    for(auto it = symbol_table.begin(); it != symbol_table.end(); it++){
            cout << get<0>(*it) << "=" << get<1>(*it) << endl;
    }

}

void parse(string input_file_path, int pass){
    ifstream input_file(input_file_path);

    if (!input_file){
        cout << "File at " << input_file_path <<" not found." << endl;
        exit(1);
    }
    
    vector<struct token> tokens = getToken(input_file);
    
    // No longer need the file once tokenized
    input_file.close();

    // Pass 1 here
    if (pass == 1) printSymbolTable(tokens);

    else if (pass == 1){}
    
}    

int main(int argc, char const *argv[])
{
    if (argc==1){
        cout<<"Missing Argument: Please pass the path to the linker input file.\n";
        exit(1);
    }

    string file_path =  argv[1];

    // Execute Pass 1
    parse(file_path, 1);
    

    return 0;
}

