#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <regex>
#include <algorithm>
#include <list>
#include <vector>

using namespace std;

void to_upper(string& str) {transform(str.begin(), str.end(), str.begin(), ::toupper);}

string assemblyToDynamorio(string instr);
void use_reg(string reg);

fstream infile_a;
fstream outfile_c;

vector<string> used_reg;
list<string> instr_buffer;

enum OP_TYPE{
    OP_REG = 0,
    OP_VALUE,
    OP_VAR
};

int main(int argc, char** argv){
    if(argc < 2){
        cout << "Expecting assembly filename as input" << endl;
    }
    string filename = argv[1];
    infile_a.open(filename + ".s", ios::in);
    outfile_c.open(filename + ".instr", ios::out);

    string line;
    bool in_function = false;
    while(getline(infile_a, line)){
        size_t pos = 0;
        string token;
        bool contin = false;
        
        if(in_function){
            if(regex_match(line, regex(".*ret.*"))){
                in_function = false;

                for(int i = 0; i < used_reg.size(); i++){
                    string reg = used_reg.at(i);
                    outfile_c << "dr_save_reg(drcontext,bb,trigger," + reg + ",SPILL_SLOT_" + to_string(i+1) + ");" << endl;
                }
                
                for(string instr: instr_buffer){
                    outfile_c << instr << endl;
                }
                instr_buffer.clear();

                for(int i = 0; i < used_reg.size(); i++){
                    string reg = used_reg.at(i);
                    outfile_c << "dr_restore_reg(drcontext,bb,trigger," + reg + ",SPILL_SLOT_" + to_string(i+1) + ");" << endl;
                }
                
                used_reg.clear();
                break;
            }
            if(regex_match(line, regex("#.*"))){
                continue;
            }
            instr_buffer.push_back(assemblyToDynamorio(line));
            continue;
        }

        while ((pos = line.find(" ")) != string::npos) {
            token = line.substr(0, pos);
            line.erase(0, pos + 1);

            if(token == "#"){
                contin = true;
                break;
            }

            if(regex_match(token, regex(".*func_([0-9]*).*:"))){
                in_function = true;
                contin = true;
                size_t tpos = token.find("func_");
                token = token.substr(tpos, 6);
                token = token.append(":");
                outfile_c << token << endl;
                break;
            }
        }

        if(contin){
            continue;
        }
    }
    if(in_function){
        cout << "Error: expecting end of function in assembly file" << endl;
    }

    infile_a.close();
    outfile_c.close();

    return 0;
}

string assemblyToDynamorio(string instr){
    size_t pos = 0;
    string opcode, opnd1, opnd2;
    string output = "instrlist_meta_preinsert(bb, trigger,";
    OP_TYPE op[2];

    /* Find opcode */
    instr.erase(0, 1);
    pos = instr.find("\t");
    opcode = instr.substr(0, pos);
    instr.erase(0, pos + 1);

    /* Find first operand */
    pos = instr.find(" ");
    opnd1 = instr.substr(0, pos);
    if(opnd1[opnd1.length()-1]==','){
        opnd1.erase(opnd1.length()-1);
    }
    instr.erase(0, pos + 1); 

    switch(opnd1[0]){
        case '$':
            op[0] = OP_VALUE;
            opnd1 = "OPND_CREATE_INT8(" + opnd1.substr(1, opnd1.length()-1) + ")";
            break;
        case '%':
            op[0] = OP_REG;
            to_upper(opnd1); opnd1 = "opnd_create_reg(DR_REG_" + opnd1.substr(1, opnd1.length()-1) + ")";
            break;
        default:
            if(opnd1.length() > 1){
                op[0] = OP_VAR;
                opnd1 = "OPND_CREATE_ABSMEM((byte *)&" + opnd1 + ", OPSZ_8)";
            }
    }
        
    /* Find second operand */
    pos = instr.find("\n");
    opnd2 = instr.substr(0, pos);

    switch(opnd2[0]){
        case '$':
            op[1] = OP_VALUE;
            opnd2 = "OPND_CREATE_INT8(" + opnd2.substr(1, opnd2.length()-1) + ")";
            break;
        case '%':
            op[1] = OP_REG;
            to_upper(opnd2); opnd2 = "opnd_create_reg(DR_REG_" + opnd2.substr(1, opnd2.length()-1) + ")";
            break;
        default:
            if(opnd2.length() > 1){
                op[1] = OP_VAR;
                opnd2 = "OPND_CREATE_ABSMEM((byte *)&" + opnd2 + ", OPSZ_8)";
            }
    }


    /* Insert add instruction */
    if(regex_match(opcode, regex("add[a-z]"))){
        output += "XINST_CREATE_add(drcontext, " + opnd2 + ", " + opnd1 + "));";
        if(op[1] == OP_REG){
            string reg = opnd2.substr(16);
            reg.erase(reg.length()-1);
            use_reg(reg);
        }
    }

    /* Insert mov instruction */
    if(regex_match(opcode, regex("mov[a-z]"))){
        if(op[1] == OP_REG){
            string reg = opnd2.substr(16);
            reg.erase(reg.length()-1);
            use_reg(reg);
        }

        if(op[0] == OP_VAR && op[1] == OP_REG){
            output += "XINST_CREATE_load(drcontext, " + opnd2 + ", " + opnd1 + "));";
        }

        if(op[0] == OP_REG && op[1] == OP_VAR){
            output += "XINST_CREATE_store(drcontext, " + opnd2 + ", " + opnd1 + "));";
        }

        if(op[0] == OP_REG && op[1] == OP_REG){
            output += "XINST_CREATE_move(drcontext, " + opnd2 + ", " + opnd1 + "));";
        }
    }
    
    return output;
}

void use_reg(string reg){
    if(!used_reg.empty()){
        for(string o_reg: used_reg){
            if(o_reg == reg)
                return;
        }
    }
    used_reg.push_back(reg);
}