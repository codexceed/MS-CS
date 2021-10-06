#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <vector>
#include <tuple>
#include <map>
#include <iomanip>
#include <exception>

using namespace std;

// Declare global entities
struct token{
    string value;
    int line;
    int pos;
};
enum Section { Definition, Use, Program };
typedef vector< tuple <string, string> > symb_table_type;
typedef tuple <int, int, vector<string>> arg_type;
symb_table_type symbol_table;
map <string, bool> ulist_symbol_use;
map <string, bool> dlist_symbol_use;

// Exception handler and validators
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
    cout<<"Parse Error line "<<tok.line<<" offset "<<tok.pos<<": "<<errstr[errcode]<<"\n";
    exit(0);
}

bool symbolExists(string symb){
    for(auto it = symbol_table.begin(); it != symbol_table.end(); it++){
        if(symb == get<0>(*it)){
            string new_addr = get<1>(*it) + " Error: This variable is multiple times defined; first value used";
            (*it) = make_tuple(symb, new_addr);
            return true;
            break;
        }
    }

    return false;
}


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
string modInstrR(arg_type args){
    return to_string(get<1>(args) + get<0>(args));
}

string modInstrE(arg_type args){
    int operand = get<0>(args);
    vector<string> use_list = get<2>(args);

    if(operand >= use_list.size()) return to_string(operand) + " Error: External address exceeds length of uselist; treated as immediate";

    // Identify symbol
    string symb = use_list[operand];
    
    // Search symbol table for new address corresponding to the symbol
    for(auto it = symbol_table.begin(); it != symbol_table.end(); it++){
        if (symb == get<0>(*it)){
            ulist_symbol_use[symb] = true;
            return to_string(stoi(get<1>(*it)));
        }
    }

    return "0 Error: " + symb + " is not defined; zero used";
}

string modInstrI(arg_type args){
    // Written for conventional consistency
    return to_string(get<0>(args));
}

string modInstrA(arg_type args){
    int operand = get<0>(args);

    if (operand >= 512) return "0 Error: Absolute address exceeds machine size; zero used";

    return to_string(operand);
}


// Parse functions
symb_table_type getSymbolTable(vector< struct token > tokens){
    // Define token navigation trackers
    int curr_module = 0;
    Section curr_section = Definition;
    vector<struct token>::iterator it = tokens.begin();


    // Parse the tokens
    while ( it!=tokens.end() ){
        try{
            // Get count of current list
            int count = stoi((*it).value);

            // Check def list length
            if(count > 16) __parseerror(4, *it);

            if (curr_section == Definition){
                for(int i=0; i<count; i++){
                    string symb = (*++it).value;
                    
                    // Check symbol length
                    if(symb.length() > 16) __parseerror(3, *it);

                    string addr = to_string(curr_module + stoi((*++it).value));

                    // Only push if the symbol is new
                    if(!symbolExists(symb)) symbol_table.push_back(make_tuple(symb, addr));
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
        catch(invalid_argument &in_arg){
            __parseerror(0, *it);
        }
        
    }

    // Print the symbol table
    cout << "Symbol Table" << endl;
    for(auto it = symbol_table.begin(); it != symbol_table.end(); it++){
            cout << get<0>(*it) << "=" << get<1>(*it) << endl;
    }

    return symbol_table;

}

void getInstruction(vector< struct token > tokens){
    // Define token navigation trackers
    int curr_module = 0, instr_index = 0;
    Section curr_section = Definition;
    vector <struct token>::iterator it = tokens.begin();
    vector <string> dlist_warnings;


    // Instruction entities
    map <string, string (*)(arg_type)> instr_mod_map = { {"R", &modInstrR}, {"E", &modInstrE}, {"I", &modInstrI}, {"A", &modInstrA} };
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
            // Check use list length
            if(count > 16) __parseerror(5, *it);

            for(int i = 0; i < count; i++){
                string usymb = (*++it).value;
                use_list.push_back(usymb);
                dlist_symbol_use[usymb] = true;
            }

            curr_section = Program;
        }

        else if (curr_section == Program){
            for(int i = 0; i < count; i++){
                // Get instruction
                string type = (*++it).value;
                int instr = stoi((*++it).value);
                int opcode = instr/1000, operand = instr%1000;

                if(opcode >= 10){
                    cout << setw(3) << setfill('0') << instr_index << ": " << "9999 Error: Illegal opcode; treated as 9999\n";
                }

                else if(operand >= 10000 && type == "I"){
                    cout << setw(3) << setfill('0') << instr_index << ": " << "9999 Error: Illegal immediate value; treated as 9999\n";
                }

                else{
                    // Modify instruction by calling the corresponding mod function
                    instr_args = make_tuple(operand, curr_module, use_list);
                    string modded_instr = instr_mod_map[type](instr_args);

                    // Sanitize the value pulled from symbol table to format the actual address and error message
                    int new_operand = stoi(modded_instr);
                    int new_op_len = to_string(new_operand).length();
                    string extra_text = modded_instr.substr(modded_instr.find(to_string(new_operand)) + new_op_len, modded_instr.length()-new_op_len);

                    cout << setw(3) << setfill('0') << instr_index << ": " << opcode << setw(3) << setfill('0') << new_operand << extra_text << endl;
                }
                    
                instr_index++;
            }
            // Print use list warnings
            for(auto use_it = use_list.begin(); use_it != use_list.end(); use_it++){
                string symb = *use_it;
                if(!ulist_symbol_use[symb]){
                    cout << "Warning: Module " + to_string(curr_module+1) + ": " + symb + " appeared in the uselist but was not actually used\n";
                }
            }

            // Move on to the next module
            curr_section = Definition;
            curr_module+=count;
            use_list.clear();
        }

        // Keep it moving
        it++;
    }

    cout << endl;

    // Print unused symbols from deflist
    for(auto it = symbol_table.begin(); it != symbol_table.end(); it++){
        string symb = get<0>(*it);
        if(!dlist_symbol_use[symb]) cout << "Warning: " + symb + " was defined but never used\n";
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
    if (pass == 1){
        symbol_table.clear();
        symbol_table = getSymbolTable(tokens);
    } 

    else if (pass == 2) getInstruction(tokens);
    
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

