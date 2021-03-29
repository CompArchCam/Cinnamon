#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <set>
#include <deque>
#include <stack>
#include <utility>
#include <algorithm>
#include "CodeGen.h"
#include "util.h"

string comp_type;
string curr_key;
string type_key;


bool global=false;
bool lhs_assn=false;
int actionID=1;

int mode=0;
int error_count=0;

typedef std::pair<int, std::string> comp_decl;
std::map<int, comp_decl> comp_map;


typedef std::set<std::string> VarDefs;
VarDefs activeCompVars;                         //active CFE elements
std::set<std::string> globalVars;               //set of global Vars 
std::set<std::string> actionVars;               //set of variables defined within actions
std::deque<std::string> activeVars;             //overall set of active variables (includes global and currently active)
//std::map<string, string> activeVarsType;        //type of activeVars (currently tracing only primitive)
std::map <string, int> globalDeclList;          //all global decl and their location in globalh file
std::set<std::string> stdLib;                   //standard libraries to be included
std::set<std::string> static_global_defs;


int var_count = 0;                              //counter for variables being passed to instrumented actions
int cmdlevel = -1;                              //cmdlevel = 0 is outermost command, cmdlevel>0, inner commands
int deref_level=0;                              //dereferencing level e.g. I.src has one indirectyion, I.src.type has two

bool within_action=false;               //to determine if the current statement or access within an action
bool fcall = false;                     //to specify whether this is a function call expression
bool type_expr = false;
bool exit_block = false;                //to specifiy if we are within an exit block
bool init_block = false;                //to specify if we are within an init block
bool pass_args= false;                  //to specify if we need to pass arguments from static part to dynamic part in Janus
bool regexpr = false;

std::string lhs_reg_expr;
std::map<string, int> activeVarsType;            //type of activeVars (currently tracing only primitive)
std::stack<int> offset;
std::deque<string> lhs_tree;
std::map<int, std::string> pin_loc;
std::set<int> instrumentSet;

map<string, map_info> dyn_sym_map;
std::set<string> dyn_sym_set;
std::set<int> CommandSet;
map<int, int> cfgid;

enum{
    INS = 0, 
    BBL,
    RTN,
    IMG,
    LP
};

map<int, std::string> str;

template<class Type1, class Type2>
bool isInstanceOf(Type1* Instance) {
        return (dynamic_cast<Type2*>(Instance) != NULL);
}

void popActiveVars(){
    int num = offset.top();
    offset.pop();
    int count = 0;
    for (auto rit = activeVars.rbegin(); rit!= activeVars.rend(); ++rit){
       if(++count > num)
       break;
       activeVarsType.erase(*rit);
    }
    for(int i=0; i< num ; i++){
        activeVars.pop_back();
    }
}
        
CodeGen::CodeGen(std::string filename){  
    outfile_ins.open(filename+".ins", ios::out);
    outfile_bbl.open(filename+".bbl", ios::out);
    outfile_rtn.open(filename+".rtn", ios::out);
    outfile_img.open(filename+".img", ios::out);
    outfile_f.open(filename+".func", ios::out);
    outfile_fh.open(filename+".funch", ios::out);
    outfile_e.open(filename+".exit", ios::out);
    outfile_i.open(filename+".init", ios::out);
    outfile_p.open(filename+".ptype", ios::out);
    
    outfile[INS] = &outfile_ins;
    outfile[BBL] = &outfile_bbl;
    outfile[RTN] = &outfile_rtn;
    outfile[IMG] = &outfile_img;
    outfile[FUNC_C]= &outfile_f;
    outfile[FUNC_H] = &outfile_fh;
    outfile[EXIT_C] = &outfile_e;
    outfile[INIT_C] = &outfile_i;
    outfile[TEMP_C] = &outfile_t;
    outfile[INST_PTYPE] = &outfile_p;

    pin_loc[T_BEFORE] = "IPOINT_BEFORE";
    pin_loc[T_AFTER] = "IPOINT_AFTER";
    pin_loc[T_ENTRY] = "IPOINT_BEFORE";
    pin_loc[T_EXIT] = "IPOINT_AFTER";

    cfgid[FUNC] = RTN; 
    cfgid[MODULE] = IMG; 
    cfgid[BASICBLOCK] = BBL; 
    cfgid[INST] = INS;
    cfgid[LOOP] = LP;

    str[INS] = "INS";
    str[IMG] = "IMG";
    str[BBL] = "BBL";
    str[RTN] = "RTN";
    
    get_func[STATIC]= acc_func_stat;
    get_func[DYNAMIC]= acc_func_dyn;
   
    get_reg[16] = regtable_16;
    get_reg[32] = regtable_32;
    get_reg[64] = regtable_64;
    mode = STATIC;
}
CodeGen::~CodeGen(){
    outfile_ins.close();
    outfile_bbl.close();
    outfile_rtn.close();
    outfile_img.close();
    outfile_f.close();
    outfile_fh.close();
    outfile_e.close();

}

void CodeGen::printInstrFunc(int type, std::string scopevar){

        *(outfile[curr])<<"VOID ";
        switch(type){
            case INS:
               *(outfile[curr])<<"Instruction(INS "<<scopevar; 
            break;
            case BBL:
               *(outfile[curr])<<"Trace(TRACE trace"; 
            break;
            case IMG:
               *(outfile[curr])<<"ImageLoad(IMG "<<scopevar; 
            break;
            case RTN:
               *(outfile[curr])<<"Routine(RTN "<<scopevar; 
            break;
        }
        *(outfile[curr])<<", VOID *v){"<<endl;
        indentLevel[curr]++;
        if(type == BBL){ //BasicBlock accessed through Trace
            indent();*(outfile[curr])<<"for (BBL "<<scopevar<<" = TRACE_BblHead(trace); BBL_Valid("<<scopevar<<"); " <<scopevar<<" = BBL_Next("<<scopevar<<")){"<<endl;
             indentLevel[curr]++;
        }
}
void CodeGen::indent(){
    for(int i=0; i<indentLevel[curr]; i++)
        *(outfile[curr])<<"    ";
}

void CodeGen::visit(StatementList* stmtlst){
    
    if(stmtlst->statements.size() != 0)
    {
        for(auto stmt: stmtlst->statements){
            stmt->accept(*this);
        }
    }
}
void CodeGen::visit(SelectStatementList* stmtlst){
    
    if(stmtlst->stmts.size() != 0)
    {
        for(auto stmt: stmtlst->stmts){
            ((elseIfStmt*)stmt)->accept(*this);
        }
    }
}

void CodeGen::visit(NodeList* nl) {
    for(auto node: nl->nodes){
        node->accept(*this);
    }
}

void CodeGen::visit(ExpressionList* el) {
    int size,count=0;
    size = el->expressions.size();
    for(auto exp: el->expressions){
        exp->accept(*this);
        count++;
        if(count<size)
            *(outfile[curr])<<",";   
    }        
}
void CodeGen::visit(Identifier* id) {
    string ident = id->name;
    if(std::find(activeVars.begin(), activeVars.end(), ident) != activeVars.end()){
        if(within_action){
            //case 1: if it's a global variable, emit as it is
            if(globalVars.count(ident)){
               
               if(static_global_defs.count(ident)){
                   pass_args = true; //pass this variable as argument
                     //TODO:
                   string search_key= "" ;
                   if(!dyn_sym_set.count(ident)){ //this var has already been mapped in this action
                        search_key = "arg_"+ to_string(var_count++);
                        dyn_sym_map[search_key].var = ident;
                        dyn_sym_map[search_key].type = toCType[activeVarsType[ident]];
                        dyn_sym_set.insert(id->name);
                   }else{
                                                                                                                                 for (auto it = dyn_sym_map.begin(); it != dyn_sym_map.end(); ++it){
                          if (it->second.var.compare(ident))
                              search_key = it->first;
                       }
                                                                                                                            }
                     *(outfile[curr])<<search_key;
               }
               else{
                   *(outfile[curr])<<ident;
                }
            }

            //case 2: if it's defined within action, emit as it is
            else if(actionVars.count(ident))
               *(outfile[curr])<<ident;

            //case 3: if neither of case 1 & 2, pass it as an argument to action.
            else{
              pass_args = true;
              string search_key= "" ;
              if(!dyn_sym_set.count(ident)){ //this var has already been mapped in this action
                 search_key = "arg_"+ to_string(var_count++);
                 dyn_sym_map[search_key].var = ident;
                 dyn_sym_map[search_key].type = toCType[activeVarsType[ident]];
                 dyn_sym_set.insert(ident);
              }else{
                for (auto it = dyn_sym_map.begin(); it != dyn_sym_map.end(); ++it){
                    if (it->second.var.compare(ident))
                       search_key = it->first;
                }
              }
              *(outfile[curr])<<search_key;
            
            }
       }
       else{     //in the analysis phase. should be fine
           *(outfile[curr])<<ident;
           if(lhs_assn && globalVars.count(ident)){ //if global variable and being modified in static mode
                if(!exit_block && !init_block)
                      static_global_defs.insert(ident);
           }
       }
   }
   else{ 
      cerr<<"ERROR: variable "<<ident<< " not defined"<<endl;
      error_count++;
   }
}

void CodeGen::visit(StrConst* sc) {    
    *(outfile[curr])<< sc->value;
}

void CodeGen::visit(NumConst* nc) {    
    *(outfile[curr])<<nc->value;
}

void CodeGen::visit(BoolConst* bc) {
    string s;
    switch(bc->b_val){
      case B_TRUE:
        s = "true";
        break;
        case B_FALSE: 
        s = "false";
        break;
        default:
        cerr<<"ERROR: bool type not known! ";
        error_count++;
        break;
     }
     s.erase(remove( s.begin(), s.end(), '\"' ),s.end());
     *(outfile[curr])<<s;

}
void CodeGen::visit(OpCode* op) {
    string str;
    str = opcodes[op->value];
    str.erase(remove( str.begin(), str.end(), '\"' ),str.end());
    *(outfile[curr]) << str; 
}
void CodeGen::visit(NullPtr* ptr) {
    *(outfile[curr]) << "NULL"; 
}

void CodeGen::visit(IdentLHS* ilhs) {
   string ident = ilhs->name->name;
   if(fcall){ //A hack to avoid warnings or errors that function name not found.
       *(outfile[curr])<<ident;
   }
   else{
       if(std::find(activeVars.begin(), activeVars.end(), ident) != activeVars.end()){
           if(within_action){
                //case 1: if it's a global variable, emit as it is or check if modified in analysis phase
                if(globalVars.count(ident)){
                   if(static_global_defs.count(ident)){
                       pass_args = true; //pass this variable as argument
                         //TODO:
                       string search_key= "" ;
                       if(!dyn_sym_set.count(ident)){ //this var has already been mapped in this action
                            search_key = "arg_"+ to_string(var_count++);
                            dyn_sym_map[search_key].var = ident;
                            dyn_sym_map[search_key].type = toCType[activeVarsType[ident]];
                            dyn_sym_set.insert(ident);
                       }else{
                                                                                                                                     for (auto it = dyn_sym_map.begin(); it != dyn_sym_map.end(); ++it){
                              if (it->second.var.compare(ident))
                                  search_key = it->first;
                           }           
                       }
                         *(outfile[curr])<<search_key;
                   }else{
                      *(outfile[curr])<<ident;
                   }
                }

                //case 2: if it's defined within action, emit as it is
                else if(actionVars.count(ident))
                   *(outfile[curr])<<ident;

                //case 3: if neither of case 1 & 2, pass it as an argument to action.
                else{
                  pass_args = true;
                  string search_key = "";
                  if(!dyn_sym_set.count(ident)){        //if variable not already replaced
                      string search_key = "arg_"+ to_string(var_count++);
                      dyn_sym_map[search_key].var = ident;
                      dyn_sym_map[search_key].type = toCType[activeVarsType[ident]];
                      dyn_sym_set.insert(ident);
                 }else{
                    for (auto it = dyn_sym_map.begin(); it != dyn_sym_map.end(); ++it){
                        if (it->second.var.compare(ident))
                           search_key = it->first;
                    }
                  }
                  *(outfile[curr])<<search_key;
                
                }
           }
           else{     //in the analysis phase. should be fine
               *(outfile[curr])<<ident;
               if(lhs_assn && globalVars.count(ident)){ //if global variable and being modified in static mode
                    if(!exit_block && !init_block)
                          static_global_defs.insert(ident);
       
               }
           }
       }
       else{
          cerr<<"ERROR: variable "<<ilhs->name->name<< " not defined"<<endl;
          error_count++;
       }
   }
}

void CodeGen::visit(IndexLHS* ixlhs){
    /*-------------if base is ident --------*/
    if( isInstanceOf<LHS, IdentLHS>(ixlhs->base)){
        std::string base_name= ((IdentLHS*)ixlhs->base)->name->name;
        /*------- Platform-specific Components --------*/
        if(activeCompVars.count(base_name)){ //This is a component type so call functions based on field. shouldn't be comp though 
           //not allowed yet. throw error
        }//end of active CompVars()
        /*-------- User Defined Variables--------------*/
        else{
            if(find(activeVars.begin(), activeVars.end(), base_name) != activeVars.end()){
                  //lhs_tree.clear();
                      *(outfile[curr])<<base_name<<"[";
                      ixlhs->index->accept(*this);
                      *(outfile[curr])<<"]";
            }
            else{
                cerr<<"ERROR: undefined variable "<<base_name<<endl;
                error_count++;
                return;
            }
        
        }
    }
    /*---------- need to check more levels ------- */
    else{
        if( isInstanceOf<LHS, DerefLHS>(ixlhs->base))
             ((DerefLHS*)ixlhs->base)->accept(*this);
        else if(isInstanceOf<LHS, IndexLHS>(ixlhs->base))
            ((IndexLHS*)ixlhs->base)->accept(*this);
    }
}

void CodeGen::visit(DerefLHS* dlhs) {
    string field_name= dlhs->field->name;
    /*--------Final Level of Dereference-------------*/
    if( isInstanceOf<LHS, IdentLHS>(dlhs->base)){ 
        string base_name= ((IdentLHS*)dlhs->base)->name->name;
        /*------------------ Framework-specific Components - use framework based values ---------*/
        if(activeCompVars.count(base_name)){

            if(lhs_assn){
               cerr<<"ERROR: Cannot allow direct modification to internal data of component "<<base_name<<endl;
               error_count++;
               // *(outfile[curr])<<set_field(dlhs->field->name,base_name);
            }
            else{
                if(get_func[mode].count(field_name)){
                    if(mode == DYNAMIC && !acc_func_dyn_cat[field_name]){
                        if(field_name == "memaddr"){
                            curr_key = "(INS_IsMemoryWrite("+base_name+") ? IARG_MEMORYWRITE_EA : (INS_IsMemoryRead("+base_name+") ? IARG_MEMORYREAD_EA  : IARG_MEMORYREAD2_EA))";
                        }
                        else
                            curr_key = get_func[mode][field_name]; //need to check if static or dyn
                    }
                    else{
                        curr_key += get_func[mode][field_name];
                        curr_key += base_name + CLOSEPARAN;
                        while(deref_level){
                          curr_key += CLOSEPARAN;
                          deref_level--;
                        }
                    }
                        
                    if(within_action){ //if mode is dynamic
                      if(!type_expr){
                          pass_args = true;
                          string search_key = ""; 
                          if(regexpr){
                              lhs_reg_expr = curr_key;
                          }
                          else{
                              if(!dyn_sym_set.count(curr_key)){
                                  search_key = "arg_"+ to_string(var_count++);
                                  dyn_sym_map[search_key].var = curr_key;
                                  dyn_sym_map[search_key].type = acc_type_dyn_Pin[field_name];
                                  dyn_sym_map[search_key].ctype = acc_type_dyn_C[field_name];
                                  dyn_sym_set.insert(curr_key);
                              }else{
                                for (auto it = dyn_sym_map.begin(); it != dyn_sym_map.end(); ++it){
                                    if (it->second.var.compare(curr_key))
                                       search_key = it->first;
                                }
                              }
                              *(outfile[curr])<<search_key;
                          }
                      }
                      else{ //type_expr
                                  type_key = curr_key;
                      }
                    } //end of within_action
                    else{
                        *(outfile[curr])<<curr_key;
                    }
                    curr_key = "";
                }//end of field found
                else{
                    cerr<<"ERROR: subfield "<<field_name<<" not valid in the current context"<<endl;
                    error_count++;
                    curr_key = "";
                    deref_level =0;
                    return;
                }
            } //end of lhs_assn if/else
        } //end of ActiveCompVar()
        /*-----------------User Defined Variable ----------------*/
        else{
            if(std::find(activeVars.begin(), activeVars.end(), base_name) != activeVars.end()){
                curr_key = base_name+"."+field_name;     
            }
            else{
              cerr<<"ERROR: "<<base_name<<" not defined"<<endl;
              error_count++;
              curr_key = "";
              deref_level =0;
              return;
            }
            *(outfile[curr])<<curr_key;
            curr_key = "";
            deref_level = 0;
        }
    }//end of isInstanceOf()
    /*---------more levels of dereference---------------*/
    else{
        deref_level++;
        if( isInstanceOf<LHS, DerefLHS>(dlhs->base))
             ((DerefLHS*)dlhs->base)->accept(*this);
        else if(isInstanceOf<LHS, IndexLHS>(dlhs->base))
            ((IndexLHS*)dlhs->base)->accept(*this);
    }
}

void CodeGen::visit(FunctionCall* fc) {
    fcall = true;
    fc->name->accept(*this);
    fcall = false;
    *(outfile[curr])<<OPENPARAN;                //check whether function name exists?
    fc->arguments->accept(*this);
    *(outfile[curr])<<CLOSEPARAN;
}

void CodeGen::visit(AssnExpression* aexpr) {
     lhs_assn = true;
     aexpr->lhs->accept(*this);
     lhs_assn= false;
     
     *(outfile[curr])<<" = ";
     
     aexpr->rhs->accept(*this);
     
}

void CodeGen::visit(ConstraintExpr* constrexpr) {
       if(constrexpr != NULL)
           constrexpr->accept(*this);
}
void CodeGen::visit(RegIDExpr* regidexpr){
   int size;
   switch(regidexpr->type){
      case REG_64_ID:
          size = 64;
      break;
      case REG_32_ID:
          size = 32;
      break;
      case REG_16_ID:
          size = 16;
      break;
   }
   string search_key = "";
   string reg_key = "";
   if(regidexpr->reg == -1){//LHS
       regexpr = true;
       if(regidexpr->reg_expr){
           regidexpr->reg_expr->accept(*this);
           reg_key = "get_id("+lhs_reg_expr+")";
           lhs_reg_expr="";
           if(within_action){
               string reg_search = "(reg_id"+to_string(size)+")"+reg_key;
               if(!dyn_sym_set.count(reg_search)){
                  search_key = "arg_"+to_string(var_count++);
                  dyn_sym_map[search_key].var = reg_key;
                  dyn_sym_map[search_key].type = "IARG_UINT32";
                  dyn_sym_map[search_key].ctype = "uint32_t";
                  dyn_sym_set.insert(reg_search);
               }
               else{
                  for (auto it = dyn_sym_map.begin(); it != dyn_sym_map.end(); ++it){
                    if (it->second.var.compare(reg_key))
                        search_key = it->first;
                  }
               }
           }
           else{
               search_key = reg_key;
           }
       }
       regexpr = false;
   }
   else{//type provided
      reg_key =  "get_id("+get_reg[size][regidexpr->reg]+")"; //TODO: check if sending the macro REG_RAX suffices?
      if(within_action){
          string reg_search = "(reg_id"+to_string(size)+")"+ get_reg[size][regidexpr->reg];
          if(!dyn_sym_set.count(reg_search)){
              search_key = "arg_"+to_string(var_count++);
              dyn_sym_map[search_key].var = reg_key;
              dyn_sym_map[search_key].type = "IARG_UINT32";
              dyn_sym_map[search_key].ctype = "uint32_t";
              dyn_sym_set.insert(reg_search);
          }
          else{
             for (auto it = dyn_sym_map.begin(); it != dyn_sym_map.end(); ++it){
                if (it->second.var.compare(reg_key))
                    search_key = it->first;
             }
          }
      }
      else{
          search_key = reg_key;
      }
   }
   *(outfile[curr])<<search_key;
}
void CodeGen::visit(RegValExpr* regvalexpr){
   int size;
   if(!within_action){
       cerr<<"ERROR: register value not available in static mode"<<endl;
       error_count++;
       return;
   }
   switch(regvalexpr->type){
      case REG_64:
          size = 64;
      break;
      case REG_32:
          size = 32;
      break;
      case REG_16:
          size = 16;
      break;
   }
   string search_key = "";
   string reg_key = "";
   if(regvalexpr->reg == -1){//LHS
       regexpr = true;
       if(regvalexpr->reg_expr){
           regvalexpr->reg_expr->accept(*this);
           reg_key = lhs_reg_expr;
           lhs_reg_expr="";
           string reg_search = "(reg_"+to_string(size)+")"+reg_key;
           if(!dyn_sym_set.count(reg_search)){
              search_key = "arg_"+to_string(var_count++);
              dyn_sym_map[search_key].var = reg_key;
              dyn_sym_map[search_key].type = "IARG_REG_VALUE";
              dyn_sym_map[search_key].ctype = "uint"+to_string(size)+"_t";
              dyn_sym_set.insert(reg_search);
           }
           else{
              for (auto it = dyn_sym_map.begin(); it != dyn_sym_map.end(); ++it){
                if (it->second.var.compare(reg_key))
                    search_key = it->first;
              }
           }
       }
       regexpr = false;
   }
   else{//type provided
      string reg_key =  get_reg[size][regvalexpr->reg];
      string reg_search = "(reg_"+to_string(size)+")"+reg_key;
      if(!dyn_sym_set.count(reg_search)){
          search_key = "arg_"+to_string(var_count++);
          dyn_sym_map[search_key].var = reg_key;
          dyn_sym_map[search_key].type = "IARG_REG_VALUE";
          dyn_sym_map[search_key].ctype = "uint"+to_string(size)+"_t";
          //dyn_sym_set.insert(reg_key);
          dyn_sym_set.insert(reg_search);
      }
      else{
         for (auto it = dyn_sym_map.begin(); it != dyn_sym_map.end(); ++it){
            if (it->second.var.compare(reg_key))
                search_key = it->first;
         }
      }
   }
   *(outfile[curr])<<search_key;
}
void CodeGen::visit(UnaryExpression* uexpr) {
    std::string op, expr;
    switch(uexpr->op){
      case NOT:
        op="!";
        break;
      case MINUS:
        op="-";
        break;
      default:
        op=" UNKNOWN_OP "; /*report Error*/
        cerr<<"ERROR: operator type not known! ";
        error_count++;
        break;
    }
    *(outfile[curr])<<op;
    uexpr->operand_expr->accept(*this);
}

void CodeGen::visit(BinaryCondExpr* bcexpr) {
    std::string lhs, rhs, op;
    bcexpr->lhs->accept(*this);
    switch(bcexpr->op){
        case L_AND:
             op= " && ";
        break;
        case L_OR:
            op =" || ";
        break;
        default:
            op=" UNKNOWN_OP ";
            cerr<<"ERROR: operator type not known! ";
            error_count++;
        break;
    }
    *(outfile[curr]) <<op;
    bcexpr->rhs->accept(*this);
}
void CodeGen::visit(BinaryLogicExpr* blexpr) {
    std::string lhs, rhs, op;
    blexpr->lhs->accept(*this);
    switch(blexpr->op){
        case AND:
             op= " & ";
        break;
        case OR:
            op =" | ";
        break;
        default:
            op=" UNKNOWN_OP ";
            cerr<<"ERROR: operator type not known! ";
            error_count++;
        break;
    }
    *(outfile[curr]) <<op;
    blexpr->rhs->accept(*this);
    
}
void CodeGen::visit(BinaryArithExpr* baexpr) {
    std::string lhs, rhs, op;
    baexpr->lhs->accept(*this);
    switch(baexpr->op){
        case PLUS:
             op= " + ";
        break;
        case MINUS:
            op =" - ";
        break;
        case MULT:
            op =" * ";
        break;
        case DIV:
            op =" / ";
        break;
        default:
            op=" UNKNOWN_OP ";
            cerr<<"ERROR: operator type not known! ";
            error_count++;
        break;
    }
    *(outfile[curr])<<op;
    baexpr->rhs->accept(*this);
}
void CodeGen::visit(BinaryCompExpr* bcexpr) {
    std::string lhs, rhs, op;
    bcexpr->lhs->accept(*this);
    switch(bcexpr->op){
        case GT:
             op= " > ";
        break;
        case GE:
            op =" >= ";
        break;
        case LT:
            op =" < ";
        break;
        case LE:
            op =" <= ";
        break;
        case EQ:
            op =" == ";
        break;
        case NEQ:
            op =" != ";
        break;
        default:
            op=" UNKNOWN_OP ";
            cerr<<"ERROR: operator type not known! ";
            error_count++;
        break;
    }
    if(isInstanceOf<Expression, StrConst> (bcexpr->rhs)){
       *(outfile[curr])<<".compare("; 
    }
    else{
        *(outfile[curr])<<op;
    }
    bcexpr->rhs->accept(*this);
    if(isInstanceOf<Expression, StrConst> (bcexpr->rhs)){
       *(outfile[curr])<<")"; 
       if(bcexpr->op == EQ){
           *(outfile[curr])<<"== 0"; 
       }
       else{
           *(outfile[curr])<<"!= 0"; 

       }
    }
}


void CodeGen::visit(TypePredExpr* typeexpr) {
   if(types.count(typeexpr->type_name) > 0){
        //TODO: if within_action, then replace is_type expression with a bool, put that in argument of dynamic, replace with var name
        string type_name = types[typeexpr->type_name];
        if(within_action){
            type_expr = true;
            typeexpr->lhs->accept(*this);
            type_expr = false;
            string new_key = "is_type(" + type_key + "," + type_name + CLOSEPARAN;
            string search_key = "arg_"+ to_string(var_count++);
            dyn_sym_map[search_key].var = new_key;
            dyn_sym_map[search_key].type = "bool";     //TODO: check this
            *(outfile[curr])<<search_key;
        }
        else{
            *(outfile[curr])<<"is_type"<<OPENPARAN;
            typeexpr->lhs->accept(*this);
            *(outfile[curr])<<", "<<type_name<<CLOSEPARAN;
        }
   }
   else{
       cerr<<"ERROR: "<<typeexpr->type_name<<" is not a valid type"<<endl;
       error_count++;
   }
}

void CodeGen::visit(ExprStatement* estmt) {
    indent(); estmt->expr->accept(*this);
    *(outfile[curr])<<SEMICOLON<<endl;

}
void CodeGen::visit(ifStmt* ifstmt) {
    indent(); *(outfile[curr])<<" if( ";
    ifstmt->condition->accept(*this);
    *(outfile[curr])<<" )"<<endl;
    indent(); *(outfile[curr])<<"{"<<endl;
    
    indentLevel[curr]++;
    ifstmt->ifBody->accept(*this);
    indentLevel[curr]--;
    
    indent(); *(outfile[curr])<<"}"<<endl;
    if(ifstmt->elseIfBranch != NULL && ifstmt->elseIfBranch->stmts.size()){
        ifstmt->elseIfBranch->accept(*this);
    }
    if(ifstmt->elseBody != NULL){
        indent(); *(outfile[curr])<<"else {"<<endl;
        indentLevel[curr]++;
        ifstmt->elseBody->accept(*this);
        indentLevel[curr]--;
        indent(); *(outfile[curr])<<"}"<<endl;
    }
}
void CodeGen::visit(elseIfStmt* elseIfstmt) {
    indent(); *(outfile[curr]) <<"else if(";
    elseIfstmt->elseIfCondition->accept(*this);
    *(outfile[curr])<<" )"<<endl;
    indent(); *(outfile[curr])<<"{"<<endl;
    indentLevel[curr]++;
    elseIfstmt->elseIfBody->accept(*this);
    indentLevel[curr]--;
    
    indent(); *(outfile[curr])<<"}"<<endl;
}
void CodeGen::visit(ForStmt* forstmt) {
    //loopLevel++;
    indent(); *(outfile[curr])<<" for( ";     //shall we do this to a header?
    
    /*---- loop init condition ----*/
    VarDefs loopVars;
    forstmt->loopInit->accept(*this);
    offset.push(1);         //assuming only single variable definition
    
    /*---- loop condition ----*/
    forstmt->loopCondition->accept(*this);
    *(outfile[curr])<<SEMICOLON;
    
    /*---- iteration jump parameter----*/
    forstmt->loopIteration->accept(*this);
    *(outfile[curr])<<" )"<<endl;
    
    /*---- Loop Body Start ----*/
    indent(); *(outfile[curr])<<"{"<<endl;
    indentLevel[curr]++;
    forstmt->loopBody->accept(*this);
    indentLevel[curr]--;
    indent(); *(outfile[curr])<<"}"<<endl;
   /*---- Loop Body End ----*/
    popActiveVars();
    //loopLevel--;
}
void CodeGen::visit(WhileStmt* whilestmt) {
    indent(); *(outfile[curr])<<" while( ";    
    /*---- while loop condition ----*/
    whilestmt->loopCondition->accept(*this);
    *(outfile[curr])<<" )"<<endl;
    /*---- while Loop Body Start ----*/
    indent(); *(outfile[curr])<<"{"<<endl;
    indentLevel[curr]++;
    whilestmt->loopBody->accept(*this);
    indentLevel[curr]--;
    indent(); *(outfile[curr])<<"}"<<endl;
   /*---- while Loop Body End ----*/
    
}

void CodeGen::visit(TypeDeclStmtList* tstmtlist) {
    int count=0;
    for(auto tdecl: tstmtlist->t_stmts){
        tdecl->accept(*this);
        count++;
    }
    offset.push(count);
}

void CodeGen::visit(TypeDeclStmt* tstmt){
    tstmt->texpr->accept(*this);
}

void CodeGen::visit(VTypeList* vtypelst) {
    int size, count=0;
    size = vtypelst->typelist.size();
    for(auto vtype: vtypelst->typelist){
        vtype->accept(*this);
        count++;
        if(count<size){
           *(outfile[curr])<< ",";
        }
    }
}

void CodeGen::visit(PrimitiveType* primtype) {
    std::string type;
    switch(primtype->type){
       case VAR:
            type = "var"; //could have a pointer set to outfile_d or outfile_s
        break;
       case INT:
            type = "int";
        break;
       case UINT32:
            type = "uint32_t";
        break;
       case UINT64:
            type = "uint64_t";
        break;
       case BOOL:
            type = "bool";
        break;
       case DOUBLE:
            type = "double";
            break;
       case CHAR:
            type = "char";
            break;
       case ADDR_INT:
            type = "ADDRINT";
            break;
       case FILE_LINE:
           type = "std::string";
           break;
       default:
            type = "UNKNOWN";
            cerr<<"ERROR: variable type not known! ";
            error_count++;
            break;
    }
    *(outfile[curr])<<type;
}

void CodeGen::visit(ArrayType* array) {
    array->type->accept(*this);
}

void CodeGen::visit(PairType* pair) {
    *(outfile[curr])<<"pair<";
    pair->type1->accept(*this);
    *(outfile[curr])<<",";
    pair->type1->accept(*this);
}

void CodeGen::visit(DictType* dict) {
   stdLib.insert("map");
   *(outfile[curr])<<"map<";
   dict->keytype->accept(*this);
   *(outfile[curr])<<",";
   dict->valuetype->accept(*this);
   *(outfile[curr])<<">";
}
void CodeGen::visit(VectType* vect) {
   stdLib.insert("vector");
   *(outfile[curr])<<"vector<";
   vect->etype->accept(*this);
   *(outfile[curr])<<">";
}

void CodeGen::visit(SetType* set_) {
   stdLib.insert("set");
   *(outfile[curr])<<"set<";
   set_->etype->accept(*this);
   *(outfile[curr])<<">";
}
void CodeGen::visit(TupleType* tuple) {
    *(outfile[curr])<<"tuple<";
    tuple->typelist->accept(*this);
    *(outfile[curr])<<">";
}

void CodeGen::visit(PrimitiveTypeDecl* ptypedecl) {

   std::string type, identifier, init;
   //if global variable, extern type identifier in func.h and then init in func.cpp
  
   indent();
   ptypedecl->type->accept(*this); 
   identifier= ptypedecl->name->name;
   *(outfile[curr]) <<" "<<identifier;
   
   if(global){
       globalVars.insert(identifier);
   }
   activeVars.push_back(identifier);
   activeVarsType[identifier] = ptypedecl->type->type;
   if(within_action){
      actionVars.insert(identifier);
   }
   if(ptypedecl->init != NULL){
       *(outfile[curr])<<" = ";
       ptypedecl->init->accept(*this);
   }
   *(outfile[curr])<<";"<<endl;
}
void CodeGen::visit(FileTypeDecl* ftypedecl) {

   std::string identifier = ftypedecl->ident->name;
  
   indent();
   stdLib.insert("iostream");
   stdLib.insert("fstream");
   *(outfile[curr])<<"fstream "<<identifier;
   
   if(ftypedecl->f_var) *(outfile[curr])<<"("<<ftypedecl->f_var->name<<")";
   else if(ftypedecl->f_name) *(outfile[curr])<<"("<<ftypedecl->f_name->value<<")";
   *(outfile[curr]) <<";"<<endl;
   
   if(global){
       //curr= FUNC_C;
       globalVars.insert(identifier);
   }
   activeVars.push_back(identifier);
   activeVarsType[identifier] = FILE_T;
   if(within_action){
      actionVars.insert(identifier);
   }
}

void CodeGen::visit(CompositeTypeDecl* ctypedecl) {
   std::string typedecl, type, identifier, initlist;
   
   identifier= ctypedecl->name->name;
   
   indent(); 
   if(isInstanceOf<CompositeType, ArrayType>(ctypedecl->type)){
      ((ArrayType*) ctypedecl->type)->accept(*this);
      *(outfile[curr])<< " "<<identifier;
      *(outfile[curr]) <<"["<<((ArrayType*) ctypedecl->type)->size<<"]";
      if(global){
          globalVars.insert(identifier);
      }
       //activeVarsType[identifier]  = ARRAY;
   }
   else{
      ctypedecl->type->accept(*this);
      *(outfile[curr])<< " "<<identifier;
      if(global){
          globalVars.insert(identifier);
      }
   }
   activeVars.push_back(identifier);
   

   if(within_action){
      actionVars.insert(identifier);
   }
   if(ctypedecl->init_list->expressions.size() != 0){
       *(outfile[curr])<<" =  {";
       ctypedecl->init_list->accept(*this);
       *(outfile[curr])<<" }";
   }
       *(outfile[curr])<<";"<<endl;
}
    

void CodeGen::visit(CommandBlock* cmdblock) {
     
     cmdlevel++;
     //Step 1: capture the type and name of associated CFE for this commandblock scope
     int scopeType = cfgid[cmdblock->component_type->type];
     std::string scopeVar = cmdblock->component_type->name->name;
     comp_map[cmdlevel] = make_pair(scopeType, scopeVar);
     activeCompVars.insert(scopeVar);
     if(cmdlevel == 0){
        curr = scopeType;
        CommandSet.insert(curr);
     } 
     //Step 2: Generate code accordingly for the CFE component. 
     cmdblock->component_type->accept(*this);
    
     //Step 3: Generate constraints , if any, applicable to command block
     if(((ConstraintExpr* )cmdblock->cond)->cond != NULL){
         indent(); *(outfile[curr])<<"if( ";
         ((ConstraintExpr*)cmdblock->cond)->cond->accept(*this);
         *(outfile[curr])<<"){"<<endl;
         indent();
         indentLevel[curr]++;
     }
     
     //Step 4: Generate declarations for all the local variables in this command block 
     if(cmdblock->localdeclarations){ 
         cmdblock->localdeclarations->accept(*this);
     }

     //Step 5: Generate code for all the statements which are part of the analysis phase 
     if(cmdblock->stmts->statements.size()){
        cmdblock->stmts->accept(*this); 
     }
     //Step 6: Process all enclosed/nested commands
     if(cmdblock->inner_blocks->nodes.size()){ 
        cmdblock->inner_blocks->accept(*this); 
     }
     //Step 7: Process and generate code for all the actions associated to this command
     if(cmdblock->actions->nodes.size()){
        cmdblock->actions->accept(*this);
        curr = scopeType; //set it back as actions must have changed it to func.cpp and func.h
     }
    
     //Step 8: All statements and code within command block processed at this point. pop all active variables now
     if(cmdblock->localdeclarations){
         popActiveVars();
     }
     if(((ConstraintExpr* )cmdblock->cond)->cond != NULL){ //close conditional brackets.
         indentLevel[curr]--;
         *(outfile[curr])<<"}"<<endl; 
     }
     if(cmdlevel == 0 && scopeType == BBL){
         indentLevel[curr]--;
         indent(); *(outfile[curr])<<"}"<<endl; //extra closing bracket for TRACE
     } 
     if(cmdlevel){      //Only close if inner command. for outercommand, close at the end.
         indentLevel[curr]--;
         indent(); *(outfile[curr])<<"}"<<endl;
     }
     cmdlevel--;

}

void CodeGen::visit(Action* action) {
    int saved_curr = curr; 
    /*each action is associated with current component type*/
    mode  = DYNAMIC;

    std::string cname = comp_map[cmdlevel].second;
    int ctype =  comp_map[cmdlevel].first;
    
    if(!activeCompVars.count(action->name->name) || cname.compare(action->name->name)){ //cannot do action on outer levels 
       cerr<<"FATAL ERROR: action cannot be performed on the given undefined component "<<action->name<<endl;
       error_count++;
       exit(1);
    }
   
    /*------------ Step  1. Generate action code to be encapsulated within callback routines  ------------*/
    
    curr = TEMP_C;              //save to temporary file for now
    outfile_t.open("temp.cpp", ios::out);
    indentLevel[curr]++;
    within_action = true;
    if(action->dyn_decls->t_stmts.size() != 0){
        action->dyn_decls->accept(*this);
    }
    action->stmts->accept(*this);
    within_action = false;
    indentLevel[curr]--;
    outfile_t.close();
    
    
    /*----------- Step 2. Generate definitions of callback routines, add any arguments to be passed from analysis phase or be used by routines*/
    int arg_count=0; 
    curr = FUNC_C;
    std::string func_sign;
    func_sign = "void func_"+to_string(actionID)+"(";
    for(auto it=dyn_sym_map.begin(); it!=dyn_sym_map.end(); it++){
       if((it->second.type).compare("NULL") && (it->second.type).find("IARG_") == std::string::npos){ //if not NULL
           func_sign+=it->second.type+" "+it->first;
       }else{
           func_sign+=it->second.ctype+" "+it->first;
       }
         if(++arg_count < dyn_sym_map.size())
            func_sign+=", ";
    }
    func_sign+=")";
    arg_count = 0;
    indent();*(outfile[curr])<<func_sign<<"{"<<endl;
    indentLevel[curr]++;
    outfile_t.open("temp.cpp", ios::in);
    string line;
    while(getline(outfile_t, line)){
        *(outfile[curr])<<line<<endl;
    }
    outfile_t.close();
    indentLevel[curr]--;
    indent(); *(outfile[curr])<<"}"<<endl;
    /*function prototype*/
    //curr = FUNC_H;
    //indent(); *(outfile[curr])<<func_sign<<";"<<endl;

    /*------------ Step  3. Insert trampoline calls to instrumentation actions as callback routines at specified locations  ------------*/
    curr = saved_curr;
    if(((ConstraintExpr *)action->cond)->cond != NULL){
        indent(); *(outfile[curr])<<"if(";
        ((ConstraintExpr *)action->cond)->cond->accept(*this);
        *(outfile[curr])<<"){"<<endl;
        indentLevel[curr]++;
    }
    indent(); *(outfile[curr])<<str[ctype]<<"_InsertCall("<<cname<<", "<<pin_loc[action->trigger]<<", (AFUNPTR)func_"<<actionID;
    if(dyn_sym_map.size()){
        for(auto it=dyn_sym_map.begin(); it!=dyn_sym_map.end(); it++){
            if((it->second.type).compare("NULL")){ //if not NULL
                
               if((it->second.type).find("IARG_") != std::string::npos){ //if not IARG values
                  *(outfile[curr])<<", "<<it->second.type;
               
               }else{
                  *(outfile[curr])<<", "<<toPinType[it->second.type];
               }
            }
            *(outfile[curr])<<", "<<it->second.var;
        }
    }
    *(outfile[curr])<<", IARG_END);"<<endl;
    if(((ConstraintExpr *)action->cond)->cond != NULL){
        indentLevel[curr]--;
        indent(); *(outfile[curr])<<"}"<<endl;
    }


    if(action->dyn_decls->t_stmts.size() != 0){
         popActiveVars();
         actionVars.empty();    //remove all action only vars as well.
    }

    dyn_sym_map.clear();
    dyn_sym_set.clear();
    var_count = 0;
    actionID++;
    pass_args = false;
    mode = STATIC;
}
 
void CodeGen::visit(ProgramBlock* prog) {
    /* global Decl go to dynamic part for now. Are two levels of nesting enough for this case? I.e. scope and component type? Shall we std::stringroduce other ways or levels to static part? */
    if(prog->globaldeclarations->t_stmts.size() != 0){
        global= true;
        curr = FUNC_C;
        prog->globaldeclarations->accept(*this); //start of dynamic file*/
        global= false;
    }
    //process and generate code for each command block
    if(prog->commandblocks->nodes.size()){
        prog->commandblocks->accept(*this);
    }
   
    //process and generate code for exit block
    if(prog->exit_stmts->statements.size()){
        curr= EXIT_C;
        indentLevel[curr]++;
        exit_block = true;
        prog->exit_stmts->accept(*this);
        exit_block = false;
        indentLevel[curr]--;
    }
    //process and generate code for init block
    if(prog->init_stmts->statements.size()){
        curr= INIT_C;
        indentLevel[curr]++;
        init_block = true;
        prog->init_stmts->accept(*this);
        init_block = false;
        indentLevel[curr]--;
    }
    for(auto &header: stdLib){
          *(outfile[FUNC_H])<<"#include <"<<header<<">"<<endl;
    }
    
    /*At the end of the program, pop all global variables. locals are popped out after scope end*/
    if(prog->globaldeclarations->t_stmts.size()){
        popActiveVars();
    }
    
    for(auto it : CommandSet){ //close instrumentation functions
        indentLevel[it]--;
        *(outfile[it])<<"}"<<endl;
    }

    if(error_count>0){
      cerr<<error_count<<" errors reported: compilation unsuccessful"<<endl;
      exit(1);
    }
   
}

void CodeGen::visit(Component* comp) {
     //curr = STAT;
     int id = cfgid[comp->type];
     std::string cname = comp_map[cmdlevel].second;
     if(cmdlevel){//inner level nesting. check if nesting is allowed.
         switch(id){
             case INS:
                if(comp_map[cmdlevel-1].first == RTN){ //generatre code through section
                    *(outfile[curr])<<"RTN_open(RTN "<<comp_map[cmdlevel-1].second<<");"<<endl;
                    indent(); *(outfile[curr])<<"for (INS "<<cname<<" = RTN_InsHead("<<comp_map[cmdlevel-1].second<<"); INS_Valid("<<cname<<"); "<<cname<<" = INS_Next("<<cname<<")){"<<endl;
                    indentLevel[curr]++;
                }
                else if(comp_map[cmdlevel-1].first == IMG){
                }
                else if(comp_map[cmdlevel-1].first == BBL){
                    indent(); *(outfile[curr])<<"for (INS "<<cname<<" = BBL_InsHead("<<comp_map[cmdlevel-1].second<<"); INS_Valid("<<cname<<"); "<<cname<<" = INS_Next("<<cname<<")){"<<endl;
                    indentLevel[curr]++;
                }
                else{
                    cerr<<"ERROR: Invalid Nesting Level";
                    error_count++;
                }
             break;
             case BBL:
                if(comp_map[cmdlevel-1].first == RTN){ //generatre code through section
                    cerr<<"Warning: Deprecated feature to access basicblocks in a routine";
                    *(outfile[curr])<<"RTN_open(RTN "<<comp_map[cmdlevel-1].second<<");"<<endl;
                    *(outfile[curr])<<"for (BBL "<<cname<<" = RTN_BblHead("<<comp_map[cmdlevel-1].second<<"); BBL_Valid("<<cname<<"); "<<cname<<" = BBL_Next("<<cname<<")){"<<endl;
                    indentLevel[curr]++;
                }
                else if(comp_map[cmdlevel-1].first == IMG){
                    cerr<<"Warning: Deprecated feature to access basicblocks through modules";
                }
                else{
                    cerr<<"ERROR: Invalid Nesting Level";
                    error_count++;
                }
             break;
             case RTN:
                 if(comp_map[cmdlevel-1].first == IMG){ //generatre code through section
                    *(outfile[curr])<<"for (SEC sec = IMG_SecHead("<<comp_map[cmdlevel-1].second<<"); SEC_Valid(sec); "<<cname<<" = SEC_Next(sec)){"<<endl;
                    indentLevel[curr]++;
                    indent(); *(outfile[curr])<<"for (RTN "<<cname<<" = SEC_RtnHead(sec); RTN_Valid("<<cname<<"); "<<cname<<" = RTN_Next("<<cname<<")){"<<endl;

                    indentLevel[curr]++;
                 }
                 else{
                    cerr<<"ERROR: Invalid Nesting Level";
                    error_count++;
                 }
             break;
             case IMG:
                cerr<<"ERROR: Module not allowed in inner level of nesting";
                error_count++;
             break;
             default:
                cerr<<"Component not supported."<<endl;
                error_count++;
                exit(1);
                break;
         }

     }
     else{ //outermost level
        if(!instrumentSet.count(id)){   //this condition checks if we already have instrumentation code for certain CFE type to avoid emitting the code again
            switch(id){
                case INS:
                    *(outfile[INST_PTYPE])<<"INS_"<<ADD_INSTR_FUNC<<"(Instruction,0);"<<endl;
                break;
                case RTN:
                    *(outfile[INST_PTYPE])<<"RTN_"<<ADD_INSTR_FUNC<<"(Routine,0);"<<endl;
                break;
                case BBL:
                    *(outfile[INST_PTYPE])<<"TRACE_"<<ADD_INSTR_FUNC<<"(Trace,0);"<<endl;
                break;
                case IMG:
                    *(outfile[INST_PTYPE])<<"IMG_"<<ADD_INSTR_FUNC<<"(ImageLoad,0);"<<endl;
                break;
                dafault:
                    cerr<<"LOOP"<<endl;
                    cerr<<" Component not supported."<<endl;
                    error_count++;
                    exit(1);
                break;

            }
                //*(outfile[INST_PTYPE])<<ADD_INSTR_FUNC<<"("<<id<<");"<<endl;
            instrumentSet.insert(id);
            printInstrFunc(id, cname);
        }
     }
}

