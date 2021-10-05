#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <vector>
#include <tuple>
#include <map>
#include <iomanip>

using namespace std;

// Declare global entities
struct token{
    string value;
    int line;
    int pos;
};

enum Section { Definition, Use, Program };

typedef tuple <int, int, vector<string>, vector< tuple <string, int> > > arg_type;


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


// Instruction operand modification
int modInstrR(arg_type args){
    return get<1>(args) + get<0>(args);
}

int modInstrE(arg_type args){
    int operand = get<0>(args);
    vector<string> use_list = get<2>(args);
    vector< tuple <string, int> > symbol_table = get<3>(args);

    // Identify symbol
    string symb = use_list[operand];

    
    // Search symbol table for new address corresponding to the symbol
    for(auto it = symbol_table.begin(); it != symbol_table.end(); it++){
        if (symb == get<0>(*it)) return get<1>(*it);
    }

    return operand;
}

int modInstrI(arg_type args){
    // Written for conventional consistency
    return get<0>(args);
}

int modInstrA(arg_type args){
    int operand = get<0>(args);

    if (operand >= 512) return -1;

    return operand;
}

vector< tuple <string, int> > getSymbolTable(vector< struct token > tokens){
    // Define token navigation trackers
    int curr_module = 0;
    Section curr_section = Definition;
    vector<struct token>::iterator it = tokens.begin();

    vector< tuple <string, int> > symbol_table;

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
            curr_module+=count;
        }

        // Keep it moving
        it++;
    }

    // Print the symbol table
    cout << "Symbol Table" << endl;
    for(auto it = symbol_table.begin(); it != symbol_table.end(); it++){
            cout << get<0>(*it) << "=" << get<1>(*it) << endl;
    }

    return symbol_table;

}

void getInstruction(vector< struct token > tokens, vector< tuple <string, int> > symbol_table){
    // Define token navigation trackers
    int curr_module = 0, instr_index = 0;
    Section curr_section = Definition;
    vector <struct token>::iterator it = tokens.begin();

    // Instruction entities
    map <string, int (*)(arg_type)> instr_mod_map = { {"R", &modInstrR}, {"E", &modInstrE}, {"I", &modInstrI}, {"A", &modInstrA} };
    arg_type instr_args;
    vector<string> use_list;

    cout << "Memory Map" << endl;

    // Parse the tokens
    while ( it!=tokens.end() ){
        // Get count of current list
        int count = stoi((*it).value);

        if (curr_section == Definition){
            for(int i=0; i<count; i++, it+=2);
            curr_section = Use;
        }

        else if (curr_section == Use){
            for(int i = 0; i < count; i++, it++){
                use_list.push_back((*it).value);
            }
            curr_section = Program;
        }

        else if (curr_section == Program){
            for(int i = 0; i < count; i++){
                // Get instruction
                string type = (*++it).value;
                int instr = stoi((*++it).value);
                int opcode = instr/1000, operand = instr%1000;

                // Modify instruction
                instr_args = make_tuple(operand, curr_module, use_list, symbol_table);
                int modded_instr = instr_mod_map[type](instr_args);

                cout << setw(3) << setfill('0') << instr_index << ": " << opcode << setw(3) << setfill('0') << modded_instr << endl;
                
                instr_index++;
            }

            // Move on to the next module
            curr_section = Definition;
            curr_module+=count;
            use_list.clear();
        }

        // Keep it moving
        it++;
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
    vector < tuple <string, int> > symbol_table;
    if (pass == 1) symbol_table = getSymbolTable(tokens);

    else if (pass == 2) getInstruction(tokens, symbol_table);
    
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
    
    cout << endl;

    // Execute Pass 2
    parse(file_path, 2);
    

    return 0;
}

