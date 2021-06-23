#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <regex>
#include <algorithm>

using namespace std;

fstream infile_instr;
fstream outfile_dyn;

int main(int argc, char** argv){
    if(argc < 2){
        cout << "Expecting filename as input" << endl;
        return 1;
    }

    string filename = argv[1];
    infile_instr.open(filename + ".instr", ios::in);
    outfile_dyn.open(filename + ".dyn", ios::out | ios::trunc);

    int num_rule = 0;
    string line;
    while(getline(infile_instr, line)){
        if(regex_match(line, regex("func_[0-9]*:"))){
            if(num_rule > 0){
                outfile_dyn << "}" << endl;
            }
            num_rule++;
            outfile_dyn << "void handler_" + to_string(num_rule) + "(JANUS_CONTEXT){" << endl;
            outfile_dyn << "    instr_t * trigger = get_trigger_instruction(bb,rule);" << endl;
            continue;
        }
        outfile_dyn << "    " << line << endl;
    }
    outfile_dyn << "}" << endl;

    outfile_dyn<<"void create_handler_table(){"<<endl;
    
    for(int i=0; i<num_rule; i++){
        outfile_dyn<<"    htable["<<i<<"] = (void*)&handler_"<<i+1<<";"<<endl; 
    }

    outfile_dyn<<"}"<<endl;

    infile_instr.close();
    outfile_dyn.close();

    return 0;
}