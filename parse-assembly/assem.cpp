#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <regex>
#include <algorithm>
#include <list>
#include <vector>
#include <set>

using namespace std;

void to_upper(string& str) {transform(str.begin(), str.end(), str.begin(), ::toupper);}

string assemblyToDynamorio(string instr);
void use_reg(string reg);
void use_volatile_reg();
void save_arith_flags(int i);
void restore_arith_flags(int i);
int get_opnd_size(string opcode);

fstream infile_a;
fstream outfile_c;
fstream outfile_funch;
fstream outfile_dynh;

vector<string> used_reg;
list<string> instr_buffer;
map<int, int> args;
int arg_index = 0;
bool save_arith_flag = false;
set<string> setAppFunc = {"mapSet", "mapGet", "print_u64", "print_u32", "print_i", "print_str", 
                          "setCount", "setInsert", "setSize", "vectorPushback", "vectorAt", "fileOpen", "fileWrite", "fileClose"};
set<string> volatile_reg = {"DR_REG_RAX", "DR_REG_RCX", "DR_REG_RDX", "DR_REG_RSI", "DR_REG_RDI",
                            "DR_REG_R8", "DR_REG_R9", "DR_REG_R10", "DR_REG_R11", "DR_REG_R15"};
set<string> labels;

vector<string> lits_name;
vector<string> lits_type;
vector<string> lits_varname;

int RAX_slot;
int rsp_pos;
bool has_arg = false;

/* operand types */
enum OP_TYPE{
    OP_REG = 0,
    OP_VALUE,
    OP_VAR,
    OP_ARG,
    OP_MEMPTR,
    OP_FUNC,
    OP_PC,
    OP_TGRADDR
};

/* Janus regName and corresponding uint32_t values */
map<string, uint32_t> regName = {{"DR_REG_NULL", 0},
    {"DR_REG_RAX", 1},    {"DR_REG_RCX", 2},    {"DR_REG_RDX", 3},    {"DR_REG_RBX", 4},
    {"DR_REG_RSP", 5},    {"DR_REG_RBP", 6},    {"DR_REG_RSI", 7},    {"DR_REG_RDI", 8},
    {"DR_REG_R8", 9},    {"DR_REG_R9", 10},    {"DR_REG_R10", 11},    {"DR_REG_R11", 12},
    {"DR_REG_R12", 13},    {"DR_REG_R13", 14},    {"DR_REG_R14", 15},    {"DR_REG_R15", 16},
    {"DR_REG_EAX", 17},    {"DR_REG_ECX", 18},    {"DR_REG_EDX", 19},    {"DR_REG_EBX", 20},
    {"DR_REG_ESP", 21},    {"DR_REG_EBP", 22},    {"DR_REG_ESI", 23},    {"DR_REG_EDI", 24},
    {"DR_REG_R8D", 25},    {"DR_REG_R9D", 26},    {"DR_REG_R10D", 27},    {"DR_REG_R11D", 28},
    {"DR_REG_R12D", 29},    {"DR_REG_R13D", 30},    {"DR_REG_R14D", 31},    {"DR_REG_R15D", 32},
    {"DR_REG_AX", 33},    {"DR_REG_CX", 34},    {"DR_REG_DX", 35},    {"DR_REG_BX", 36},
    {"DR_REG_SP", 37},    {"DR_REG_BP", 38},    {"DR_REG_SI", 39},    {"DR_REG_DI", 40},
    {"DR_REG_R8W", 41},    {"DR_REG_R9W", 42},    {"DR_REG_R10W", 43},    {"DR_REG_R11W", 44},
    {"DR_REG_R12W", 45},    {"DR_REG_R13W", 46},    {"DR_REG_R14W", 47},    {"DR_REG_R15W", 48},
};

int main(int argc, char** argv){
    if(argc < 2){
        cout << "Expecting assembly filename as input" << endl;
    }
    string filename = argv[1];
    infile_a.open(filename + ".s", ios::in);
    outfile_c.open(filename + ".instr", ios::out);
    outfile_funch.open(filename + ".funch", ios::app);
    outfile_dynh.open(filename + ".dynh", ios::app);

    string line;
    bool in_function = false;
    while(getline(infile_a, line)){
        size_t pos = 0;
        string token;
        bool contin = false;
        
        if(in_function){
            if(regex_match(line, regex(".*ret.*"))){
                in_function = false;

                int i = 0;
                for(string label : labels){
                    outfile_c << "instr_t *" + label + " = INSTR_CREATE_label(drcontext);" << endl;
                }
                if(save_arith_flag)
                    use_reg("DR_REG_RAX");
                for(i = 0; i < used_reg.size(); i++){
                    string reg = used_reg.at(i);
                    if(reg == "DR_REG_RSP" || reg == "DR_REG_RAX"){
                        outfile_c << "dr_save_reg(drcontext,bb,trigger," + reg + ",SPILL_SLOT_" + to_string(i+1) + ");" << endl;
                        if(reg == "DR_REG_RAX")
                            RAX_slot = i+1;
                    } else{
                        outfile_c << "if(inRegSet(bitmask," + to_string(regName[reg]) + ")) dr_save_reg(drcontext,bb,trigger," + reg + ",SPILL_SLOT_" + to_string(i+1) + ");" << endl;
                    }
                }
                if(save_arith_flag){
                    save_arith_flags(i+1);
                }
                
                for(string instr: instr_buffer){
                    outfile_c << instr << endl;
                }
                instr_buffer.clear();

                for(i = 0; i < used_reg.size(); i++){
                    string reg = used_reg.at(i);
                    if(reg == "DR_REG_RSP" || reg == "DR_REG_RAX")
                        outfile_c << "dr_restore_reg(drcontext,bb,trigger," + reg + ",SPILL_SLOT_" + to_string(i+1) + ");" << endl;
                    else
                        outfile_c << "if(inRegSet(bitmask," + to_string(regName[reg]) + ")) dr_restore_reg(drcontext,bb,trigger," + reg + ",SPILL_SLOT_" + to_string(i+1) + ");" << endl;
                }
                if(save_arith_flag){
                    restore_arith_flags(i+1);
                }
                
                save_arith_flag = false;
                used_reg.clear();
            }
            if(regex_match(line, regex("#.*"))){
                continue;
            }
            string assem_line = assemblyToDynamorio(line);
            if(assem_line != "None"){
                instr_buffer.push_back(assem_line);
            }
            continue;
        }

        /* Define global variables to be used as a literal value */
        

        while (1) {
            bool br = false;
            pos = line.find(" ");
            if(pos == string::npos){
                pos = line.find("\t");
                if(pos == string::npos) br = true;
            }
            token = line.substr(0, pos);
            line.erase(0, pos + 1);
            if(regex_match(token, regex("#.*"))){
                contin = true;
                break;
            }

            if(regex_match(token, regex(".*func_([0-9]+).*:"))){
                in_function = true;
                char x = token[token.find(":") - 1];
                if(x != 'v')
                    has_arg = true;
                else
                    has_arg = false;
                rsp_pos = 0;
                contin = true;
                size_t tpos = token.find("func_");
                token = token.substr(tpos, 6);
                token = token.append(":");
                outfile_c << token << endl;
                break;
            }

            if(regex_match(token, regex("\\..*:"))){
                for(int i = 0; i < lits_name.size(); i++){
                    string name = lits_name[i];
                    if(regex_match(token, regex("\\." + name + ":"))){
                        string var_name = lits_varname[i];
                        string type_name = lits_type[i];
                        if(type_name == "str"){
                            getline(infile_a, line);
                            line.erase(0, 1);
                            line.erase(0, line.find("\t") + 1);
                            outfile_funch << "extern char " + var_name + "[];";
                            outfile_dynh << "char " + var_name + "[] = " + line + ";";
                        }
                    }
                }
            }

            if(br) break;
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
    outfile_dynh.close();
    outfile_funch.close();

    return 0;
}

/* Parse one line of assembly code, turns it into dynamoRio macros and return them */
string assemblyToDynamorio(string instr){
    size_t pos = 0;
    string opcode, opnd1, opnd2;
    string opnd1_val, opnd2_val;
    string output = "instrlist_meta_preinsert(bb, trigger, ";
    OP_TYPE op[2];

    /* Find opcode */
    instr.erase(0, 1);
    pos = instr.find("\t");
    opcode = instr.substr(0, pos);
    int opsize = get_opnd_size(opcode);
    instr.erase(0, pos + 1);

    /* Find first operand */
    pos = instr.find(" ");
    opnd1 = instr.substr(0, pos);
    if(opnd1[opnd1.length()-1]==','){
        opnd1.erase(opnd1.length()-1);
    }
    instr.erase(0, pos + 1); 

    switch(opnd1[0]){
        case '$':{
            op[0] = OP_VALUE;
            if(opnd1[1] == '.'){
                string name = opnd1.substr(2);
                lits_name.push_back(name);
                string base_name = name.substr(0, name.find("."));
                name.erase(0, name.find(".") + 1);
                size_t pos = name.find(".");
                string disp_name;
                string type_name;
                if(pos == string::npos){
                    disp_name = "_0";
                    type_name = name;
                } else {
                    disp_name = "_" + name.substr(pos+1);
                    type_name = name.substr(0, pos);
                }
                string var_name = base_name + disp_name;
                opnd1 = "OPND_CREATE_INT64((uint64_t)" + var_name + ")";
                lits_type.push_back(type_name);
                lits_varname.push_back(var_name);
            } else if(regex_match(opnd1, regex("\\$[0-9]*"))) {
                opnd1_val = opnd1.substr(1);
                opnd1 = "OPND_CREATE_INT32(" + opnd1.substr(1) + ")";
            } else {
                opnd1 = "OPND_CREATE_INT64((uint64_t)&" + opnd1.substr(1) + ")";
            }
            break;
        }
        case '%':{
            op[0] = OP_REG;
            to_upper(opnd1);
            opnd1 = "opnd_create_reg(DR_REG_" + opnd1.substr(1) + ")";
            break;
        }
        case '.':{
            op[0] = OP_PC;
            opnd1 = opnd1.substr(1);
            break;
        }
        default:{
            int p = 0;
            if(opnd1.find("(")!=string::npos && opnd1.find("(")!=0)
                p = stoi(opnd1.substr(0, opnd1.find("(")));
            if(regex_match(opnd1, regex("-?[0-9]*\\(%rsp\\)")) && (rsp_pos - p == opsize) && has_arg){
                op[0] = OP_ARG;
                int index = 0;
                if(opnd1.substr(0, opnd1.find("(")) != ""){
                    index = stoi(opnd1.substr(0, opnd1.find("(")));
                }
                args[index] = arg_index++;
                opnd1 = "OPND_CREATE_INT" + to_string(opsize*8) + "(rule->reg0)";
            } else if(regex_match(opnd1, regex(".*[0-9]*\\(%[a-z][a-z][a-z]\\).*"))){
                op[0] = OP_MEMPTR;
                string offset;
                if(opnd1.find("(") != string::npos){
                    offset = opnd1.substr(0, opnd1.find("("));
                }
                if(opnd1.find("(") == 0)
                    offset = "0";
                string regis = opnd1.substr(opnd1.find("(") + 2);
                to_upper(regis);
                regis = "DR_REG_" + regis;
                regis.erase(regis.length()-1);
                opnd1 = "OPND_CREATE_MEM" + to_string(opsize*8) + "(" + regis + ", " + offset + ")";
            } else if(regex_match(opcode, regex("call[a-z]*"))){
                op[0] = OP_FUNC;
                use_volatile_reg();
                save_arith_flag = true;
                for(string appfunc: setAppFunc){
                    if(regex_match(opnd1, regex(".*" + appfunc + ".*"))){
                        string func_name = appfunc;
                        size_t npos = opnd1.find(appfunc);
                        npos += appfunc.size();
                        while('0' <= opnd1[npos] && '9' >= opnd1[npos]){
                            func_name += opnd1[npos];
                            npos++;
                        }
                        opnd1 = "opnd_create_pc((byte *)&" + func_name + ")";
                        break;
                    }
                }
            } else {
                op[0] = OP_VAR;
                string x;
                //TODO: change size of arg registers based on opsize
                if(opsize == 8)
                    x = "R";
                else if (opsize == 4)
                    x = "E";
                else
                    x = "";
                if(opnd1 == "I_nextaddr"){
                    op[0] = OP_VALUE;
                    opnd1 = "OPND_CREATE_INTPTR(get_nextaddr_full(drcontext, trigger))";
                } else if(opnd1 == "I_memaddr"){
                    op[0] = OP_VALUE;
                    output = "get_mem_rw_addr_full(drcontext, bb, trigger);"; return output;
                } else if (opnd1 == "I_trgaddr") {
                    op[0] = OP_TGRADDR;
                } else if (opnd1 == "I_rtnval") {
                    op[0] = OP_REG;
                    opnd1 = "opnd_create_reg(DR_REG_" + x + "AX)";
                } else if (opnd1 == "I_arg1") {
                    op[0] = OP_REG;
                    opnd1 = "opnd_create_reg(DR_REG_" + x + "DI)";
                } else if (opnd1 == "I_arg2") {
                    op[0] = OP_REG;
                    opnd1 = "opnd_create_reg(DR_REG_" + x + "SI)";
                } else if (opnd1 == "I_arg3") {
                    op[0] = OP_REG;
                    opnd1 = "opnd_create_reg(DR_REG_" + x + "DX)";
                } else if (opnd1 == "I_arg4") {
                    op[0] = OP_REG;
                    opnd1 = "opnd_create_reg(DR_REG_" + x + "CX)";
                } else if (opnd1 == "I_arg5") {
                    op[0] = OP_REG;
                    opnd1 = "opnd_create_reg(DR_REG_R8)";
                } else if (opnd1 == "I_arg6") {
                    op[0] = OP_REG;
                    opnd1 = "opnd_create_reg(DR_REG_R9)";
                } else {
                    opnd1 = "OPND_CREATE_ABSMEM((byte *)&" + opnd1 + ", OPSZ_" + to_string(opsize) + ")";
                }
            }
        }
    }
        
    /* Find second operand */
    pos = instr.find("\n");
    opnd2 = instr.substr(0, pos);
    pos = opnd2.find(" ");
    if(pos != string::npos){
        opnd2.erase(pos);
    }

    switch(opnd2[0]){
        case '$':{
            op[1] = OP_VALUE;
            if(regex_match(opnd2, regex("\\$[0-9]*"))){
                opnd2 = "OPND_CREATE_INT32(" + opnd2.substr(1, opnd2.length()-1) + ")";
            } else {
                opnd2 = "OPND_CREATE_INT64((uint64_t)&" + opnd2.substr(1, opnd2.length()-1) + ")";
            }
            break;
        }
        case '%':{
            op[1] = OP_REG;
            to_upper(opnd2);
            opnd2 = "opnd_create_reg(DR_REG_" + opnd2.substr(1, opnd2.length()-1) + ")";
            break;
        }
        default:{
            if(regex_match(opnd2, regex(".*-[0-9]*\\(%rsp\\).*"))){
                op[1] = OP_ARG;
            } else if(regex_match(opnd2, regex(".*[0-9]*\\(%[a-z][a-z][a-z]\\).*"))){
                op[1] = OP_MEMPTR;
                string offset;
                if(opnd2.find("(") != string::npos){
                    offset = opnd2.substr(0, opnd2.find("("));
                }
                if(opnd2.find("(") == 0)
                    offset = "0";
                string regis = opnd2.substr(opnd2.find("(") + 2);
                to_upper(regis);
                regis = "DR_REG_" + regis;
                regis.erase(regis.length()-1);
                opnd2 = "OPND_CREATE_MEM" + to_string(opsize*8) + "(" + regis + ", " + offset + ")";
            } else if(opnd2.length() > 1){
                op[1] = OP_VAR;
                opnd2 = "OPND_CREATE_ABSMEM((byte *)&" + opnd2 + ", OPSZ_" + to_string(opsize) + ")";
            }
        }
    }


    /* Insert add instruction */
    if(regex_match(opcode, regex("add[a-z]"))){
        if(opnd2 == "opnd_create_reg(DR_REG_RSP)")
            rsp_pos -= stoi(opnd1_val);
        output += "XINST_CREATE_add(drcontext, " + opnd2 + ", " + opnd1 + "));";
        if(op[1] == OP_REG){
            string reg = opnd2.substr(16);
            reg.erase(reg.length()-1);
            use_reg(reg);
        }
        return output;
    }

    /* Insert mov instruction */
    if(regex_match(opcode, regex("mov[a-z]*"))){
        if(op[1] == OP_REG){
            string reg = opnd2.substr(16);
            reg.erase(reg.length()-1);
            use_reg(reg);
        }

        if((op[0] == OP_VAR || op[0] == OP_MEMPTR) && op[1] == OP_REG){
            output += "XINST_CREATE_load(drcontext, " + opnd2 + ", " + opnd1 + "));";
        }

        if(op[0] == OP_REG && (op[1] == OP_VAR || op[1] == OP_MEMPTR)){
            output += "XINST_CREATE_store(drcontext, " + opnd2 + ", " + opnd1 + "));";
        }

        if(op[0] == OP_REG && op[1] == OP_REG){
            output += "XINST_CREATE_move(drcontext, " + opnd2 + ", " + opnd1 + "));";
        }

        if((op[0] == OP_VALUE || op[0] == OP_ARG) && op[1] == OP_REG){
            output += "XINST_CREATE_load_int(drcontext, " + opnd2 + ", " + opnd1 + "));";
        }

        if(op[0] == OP_VALUE && op[1] == OP_MEMPTR){
            string temp_reg;
            if(opsize == 8)
                temp_reg = "DR_REG_RAX";
            if(opsize == 4)
                temp_reg = "DR_REG_EAX";
            if(opsize == 2)
                temp_reg = "DR_REG_AX";
            if(opsize == 1)
                temp_reg = "DR_REG_AL";    
            output = "dr_save_reg(drcontext,bb,trigger,DR_REG_RAX,SPILL_SLOT_16);\n";
            output += "instrlist_meta_preinsert(bb, trigger, XINST_CREATE_load_int(drcontext, opnd_create_reg(" + temp_reg + "), " + opnd1 + "));\n";
            output += "instrlist_meta_preinsert(bb, trigger, XINST_CREATE_store(drcontext, " + opnd2 + ", opnd_create_reg(" + temp_reg + ")));\n";
            output += "dr_save_reg(drcontext,bb,trigger,DR_REG_RAX,SPILL_SLOT_16);";
            return output;
        }

        if(op[0] == OP_TGRADDR){
            if(op[1] == OP_REG){
                use_reg("DR_REG_RSP");
                output = R"(dr_save_reg(drcontext,bb,trigger,DR_REG_RSP,SPILL_SLOT_17);
dr_restore_reg(drcontext,bb,trigger,DR_REG_RSP,SPILL_SLOT_)" + to_string(used_reg.size()) + R"();
if(instr_is_return(trigger)){
    instrlist_meta_preinsert(bb, trigger, XINST_CREATE_load(drcontext, )" + opnd2 + R"(, OPND_CREATE_MEMPTR(DR_REG_RSP,0)));
} else if (instr_is_call_direct(trigger)){
    instrlist_meta_preinsert(bb, trigger, XINST_CREATE_load_int(drcontext, )" + opnd2 + R"(, OPND_CREATE_INTPTR(get_target_addr_call(trigger))));
} else {
    opnd_t tgraddr = get_target_addr_indcall(trigger);
    if(opnd_is_reg(tgraddr)){
        instrlist_meta_preinsert(bb, trigger, XINST_CREATE_move(drcontext, )" + opnd2 + R"(, tgraddr));
    } else {
        instrlist_meta_preinsert(bb, trigger, XINST_CREATE_load(drcontext, )" + opnd2 + R"(, tgraddr));
    }
}
dr_restore_reg(drcontext,bb,trigger,DR_REG_RSP,SPILL_SLOT_17);)";
            } else {
                output = "None";
            }
        }

        if(op[1] == OP_ARG){
            output = "None";
        }
        return output;
    }

    /* Insert call instruction */
    if(regex_match(opcode, regex("call[a-z]"))){
        if(op[0] == OP_FUNC){
            output += "XINST_CREATE_call(drcontext, " + opnd1 + "));";
        } else {
            output = "None";
        }
        return output;
    }

    /* Insert sub instruction */
    if(regex_match(opcode, regex("sub[a-z]"))){
        if(opnd2 == "opnd_create_reg(DR_REG_RSP)")
            rsp_pos += stoi(opnd1_val);
        output += "XINST_CREATE_sub(drcontext, " + opnd2 + ", " + opnd1 + "));";
        if(op[1] == OP_REG){
            string reg = opnd2.substr(16);
            reg.erase(reg.length()-1);
            use_reg(reg);
        }
        return output;
    }

    /* Insert cmp instruction */
    if(regex_match(opcode, regex("cmp[a-z]"))){
        output += "XINST_CREATE_cmp(drcontext, " + opnd2 + "," + opnd1 + "));";
        return output;
    }


    /* Insert push instructions in terms of a sub and a store instruction */
    if(regex_match(opcode, regex("push[a-z]"))){
        rsp_pos += opsize;
        if(op[0] == OP_REG){
            output = "instrlist_meta_preinsert(bb, trigger,INSTR_CREATE_push(drcontext, " + opnd1 + "));";
        } else {
            output = "PUSH ERROR";
        }
        return output;
    }


    /* Insert pop instructions in terms of a load and an add instruction */
    if(regex_match(opcode, regex("pop[a-z]"))){
        rsp_pos -= opsize;
        if(op[0] == OP_REG){
            output = "instrlist_meta_preinsert(bb, trigger, INSTR_CREATE_pop(drcontext, " + opnd1 + "));";
        } else {
            output = "PUSH ERROR";
        }
        return output;
    }

    
    /* Insert labels */
    if(regex_match(opcode, regex(".*:.*"))){
        if(opcode.find(" ") != string::npos)
            opcode.erase(opcode.find(" "));
        opcode = opcode.substr(0, opcode.size()-1);
        labels.insert(opcode);
        output = "instrlist_meta_preinsert(bb, trigger, " + opcode + ");";
        return output;
    }

    /* Insert xor instructions */
    if(regex_match(opcode, regex("xor.*"))){
        output += "INSTR_CREATE_xor(drcontext, " + opnd2 + "," + opnd1 + "));";
        return output;
    }


    /* Insert conditional jump instructions */
    if(regex_match(opcode, regex("jle"))){
        output += "XINST_CREATE_jump_cond(drcontext, DR_PRED_LE, opnd_create_instr(" + opnd1 + ")));";
        return output;
    }

    if(regex_match(opcode, regex("jg"))){
        output += "XINST_CREATE_jump_cond(drcontext, DR_PRED_NLE, opnd_create_instr(" + opnd1 + ")));";
        return output;
    }

    if(regex_match(opcode, regex("jge"))){
        output += "XINST_CREATE_jump_cond(drcontext, DR_PRED_NL, opnd_create_instr(" + opnd1 + ")));";
        return output;
    }

    if(regex_match(opcode, regex("ja"))){
        output += "XINST_CREATE_jump_cond(drcontext, DR_PRED_NBE, opnd_create_instr(" + opnd1 + ")));";
        return output;
    }

    if(regex_match(opcode, regex("jae"))){
        output += "XINST_CREATE_jump_cond(drcontext, DR_PRED_NB, opnd_create_instr(" + opnd1 + ")));";
        return output;
    }
    
    if(regex_match(opcode, regex("jl"))){
        output += "XINST_CREATE_jump_cond(drcontext, DR_PRED_L, opnd_create_instr(" + opnd1 + ")));";
        return output;
    }

    if(regex_match(opcode, regex("jb"))){
        output += "XINST_CREATE_jump_cond(drcontext, DR_PRED_B, opnd_create_instr(" + opnd1 + ")));";
        return output;
    }

    if(regex_match(opcode, regex("jo"))){
        output += "XINST_CREATE_jump_cond(drcontext, DR_PRED_O, opnd_create_instr(" + opnd1 + ")));";
        return output;
    }

    if(regex_match(opcode, regex("jno"))){
        output += "XINST_CREATE_jump_cond(drcontext, DR_PRED_NO, opnd_create_instr(" + opnd1 + ")));";
        return output;
    }
    
    if(regex_match(opcode, regex("jmp"))){
        output += "XINST_CREATE_jump(drcontext, opnd_create_instr(" + opnd1 + ")));";
        return output;
    }

    if(regex_match(opcode, regex("jz"))){
        output += "XINST_CREATE_jump_cond(drcontext, DR_PRED_Z, opnd_create_instr(" + opnd1 + ")));";
        return output;
    }

    if(regex_match(opcode, regex("je"))){
        output += "XINST_CREATE_jump_cond(drcontext, DR_PRED_Z, opnd_create_instr(" + opnd1 + ")));";
        return output;
    }

    if(regex_match(opcode, regex("jnz"))){
        output += "XINST_CREATE_jump_cond(drcontext, DR_PRED_NZ, opnd_create_instr(" + opnd1 + ")));";
        return output;
    }

    if(regex_match(opcode, regex("jne"))){
        output += "XINST_CREATE_jump_cond(drcontext, DR_PRED_NZ, opnd_create_instr(" + opnd1 + ")));";
        return output;
    }

    if(regex_match(opcode, regex("js"))){
        output += "XINST_CREATE_jump_cond(drcontext, DR_PRED_S, opnd_create_instr(" + opnd1 + ")));";
        return output;
    }

    if(regex_match(opcode, regex("jns"))){
        output += "XINST_CREATE_jump_cond(drcontext, DR_PRED_NS, opnd_create_instr(" + opnd1 + ")));";
        return output;
    }

    /* Omit other code */
    if(regex_match(opcode, regex("\\..*")) || regex_match(opcode, regex("ret[a-z]*")) || regex_match(opcode, regex(".*#.*"))){
        output = "None";
        return output;
    }
    
    return opcode;
}

/* save and restore some registers */
void use_reg(string reg){
    if(reg[7] == 'E'){
        reg[7] = 'R';
    }
    if(reg.size() < 10 && reg[7] != 'R'){
        reg = reg.substr(0, 7) + "R" + reg.substr(7);
    }
    if(!used_reg.empty()){
        for(string o_reg: used_reg){
            if(o_reg == reg)
                return;
        }
    }
    used_reg.push_back(reg);
}

/* save and restore all caller saving registers */
void use_volatile_reg(){
    for(string reg: volatile_reg){
        use_reg(reg);
    }
}

/* save arith flags safely */
void save_arith_flags(int i){
    outfile_c << "dr_save_arith_flags(drcontext,bb,trigger,SPILL_SLOT_" + to_string(i) + ");" << endl;
    outfile_c << "dr_save_reg(drcontext,bb,trigger,DR_REG_RAX,SPILL_SLOT_" + to_string(i) + ");" << endl;
    outfile_c << "dr_restore_reg(drcontext,bb,trigger,DR_REG_RAX,SPILL_SLOT_" + to_string(RAX_slot) + ");" << endl;
}

/* restore arith flags safely */
void restore_arith_flags(int i){
    outfile_c << "dr_restore_reg(drcontext,bb,trigger,DR_REG_RAX,SPILL_SLOT_" + to_string(i) + ");" << endl;
    outfile_c << "dr_restore_arith_flags(drcontext,bb,trigger,SPILL_SLOT_" + to_string(i) + ");" << endl;
    outfile_c << "dr_restore_reg(drcontext,bb,trigger,DR_REG_RAX,SPILL_SLOT_" + to_string(RAX_slot) + ");" << endl;
}

int get_opnd_size(string opcode){
    if(regex_match(opcode, regex("(mov|xor|add|sub|push|pop|cmp|call)[a-z]*"))){
        char c = opcode[opcode.size() - 1];
        if(c == 'l')
            return 4;
        if(c == 'w')
            return 2;
        if(c == 'b')
            return 1;
        if(c == 'q' || regex_match(opcode, regex("mov[a-z]*")))
            return 8;
        return -1;
    } else {
        return -1;
    }
}