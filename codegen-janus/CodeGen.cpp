#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <set>
#include <deque>
#include <stack>
#include <algorithm>
#include "CodeGen.h"
#include "util.h"
#define MAX_NEST 10
#define FILE_T "file"


string curr_key;
string type_key;
string curr_type;

typedef std::pair<int, std::string> comp_decl;
std::map<int, comp_decl> comp_map;

map<string, map_info> dyn_sym_map;            //variable->argument mapping for variables passed to instrumented action/callback function from dynamic part
map<string, map_info> dyn_sym_map_arg;        //variable->arguments mapping for VARS passed from static part to instrumented action/callback 
std::map<string, std::pair<struct var_use, struct var_use>> global_uses; //keeps track of refs and defs of global variables, first of pair = use in static and second of pair is use in dynamic


typedef std::set<std::string> VarDefs;
VarDefs activeCompVars;                         //active CFE elements
std::set<std::string> globalVars;               //set of global Vars 
std::set<std::string> actionVars;               //set of variables defined within actions
std::deque<std::string> activeVars;             //overall set of active variables (includes global and currently active)
std::map<string, string> activeVarsType;        //type of activeVars (currently tracing only primitive)
std::map <string, int> globalDeclList;          //all global decl and their location in globalh file
std::set<std::string> stdLib;                   //standard libraries to be included

int var_count = 0;                              //counter for variables being passed to instrumented actions
int cmdlevel = -1;                              //cmdlevel = 0 is outermost command, cmdlevel>0, inner commands
int attr_count = 0;
int deref_level=0;                              //dereferencing level e.g. I.src has one indirectyion, I.src.type has two
int declLineNo = 0;                             //line number of global decl in globalh file
string global_file;

std::stack<int> offset;                 //offset for variables declared in the current scope       
int nest_level[MAX_NEST];               //for indentation and format purpose


bool within_action=false;               //to determine if the current statement or access within an action
bool fcall = false;                     //to specify whether this is a function call expression
bool type_expr = false;                 
bool exit_block = false;                //to specifiy if we are within an exit block
bool init_block = false;                //to specify if we are within an init block
bool pass_args= false;                  //to specify if we need to pass arguments from static part to dynamic part in Janus


template<class Type1, class Type2>
bool isInstanceOf(Type1* Instance) {
        return (dynamic_cast<Type2*>(Instance) != NULL);
}

void popActiveVars(){
    int num = offset.top();
    offset.pop();
    for(int i=0; i< num ; i++){
        activeVars.pop_back();
    }
}
        
bool is_action_var(string ident){
  return (std::find(actionVars.begin(), actionVars.end(), ident) != actionVars.end()) ? true : false;
}
bool is_global_var(string ident){
  return (std::find(globalVars.begin(), globalVars.end(), ident) != globalVars.end()) ? true: false;
}
bool is_active_var(string ident){
  return (std::find(activeVars.begin(), activeVars.end(), ident) != activeVars.end())? true: false;
}
CodeGen::CodeGen(std::string filename){  
    outfile_s.open(filename+".stat", ios::out);         //Static Rule Generation Code
    outfile_d.open(filename+".dyn", ios::out);          //Dynamic Handler Code
    outfile_f.open(filename+".func", ios::out);         //Instrumented Callback Functions
    outfile_fh.open(filename+".funch", ios::out);       
    outfile_i.open(filename+".init", ios::out);         //Initialisation Code for Program Execution
    outfile_e.open(filename+".exit", ios::out);         //Termination Code for Program Execution
    outfile_gh.open(filename+".globalh", ios::out);     //Header with Global Definitions (Note: temp, translation in STATH and DYNH)
    outfile_sh.open(filename+".stath", ios::out);       //Global Var Definitions for Static Part
    outfile_dh.open(filename+".dynh", ios::out);        //Global Var Definitions for Dynamic Part
    global_file = filename+".globalh" ;         
    outfile[STAT] = &outfile_s;
    outfile[DYN] = &outfile_d;
    outfile[FUNC_C]= &outfile_f;
    outfile[FUNC_H] = &outfile_fh;
    outfile[EXIT_C] = &outfile_e;
    outfile[INIT_C] = &outfile_i;
    outfile[TEMP_C] = &outfile_t;
    outfile[GLOBAL_H] = &outfile_gh;
    outfile[STAT_H] = &outfile_sh;
    outfile[DYN_H] = &outfile_dh;
    curr= STAT;
    get_func[STATIC]= get_static_func;                  //Utility functions for CFE attributes available in Static part
    get_func[DYNAMIC]= get_dyn_func;                    //Utility function for CFE attributes available in Dynamic part
}
CodeGen::~CodeGen(){
    outfile_s.close();
    outfile_d.close();
    outfile_f.close();
    outfile_fh.close();
    outfile_e.close();
    outfile_i.close();

}

void CodeGen::indent(){
    for(int i=0; i<indentLevel[curr]; i++)
        *(outfile[curr])<<"    ";
}
void CodeGen::create_handler_table(){
    indentLevel[DYN]=0;
    *(outfile[DYN])<<endl<<"void create_handler_table(){"<<endl;
    indentLevel[DYN]++;
    
    for(int i=1; i<ruleID; i++){
        indent(); *(outfile[DYN])<<"htable["<<i-1<<"] = (void*)&handler_"<<i<<";"<<endl; 
    }

    *(outfile[DYN])<<"}"<<endl;
    indentLevel[DYN]=0;
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
   string vid = id->name;
   if(within_action){//access in dynamic part
       if(is_action_var(vid) || is_global_var(vid)){
           if(is_global_var(vid)){
               global_uses[vid].second.ref = 1;
               if(global_uses[vid].first.def){ //if already modified in static part
                  pass_args = true; //pass this variable as argument
                  dyn_sym_map_arg[vid].var = "var"+ to_string(var_count++);
                  dyn_sym_map_arg[vid].type = activeVarsType[vid];
                  *(outfile[curr])<<dyn_sym_map_arg[vid].var;
                   return;
               }
           }
           *(outfile[curr])<<vid;
           
       }
       else if(is_active_var(vid)){
            pass_args = true; //pass this variable as argument
            dyn_sym_map_arg[vid].var = "var"+ to_string(var_count++);
            dyn_sym_map_arg[vid].type = activeVarsType[vid];
            *(outfile[curr])<<dyn_sym_map_arg[vid].var;
       }
       else{ 
          cerr<<"ERROR: variable "<<vid<< " not defined"<<endl;
          error_count++;
       }
   }
   else{//access in static part
      if(is_active_var(vid)){
           *(outfile[curr])<<vid;
           if(is_global_var(vid)){
              if(!exit_block && !init_block)
                  global_uses[vid].first.ref = 1; //to be later used for adding to appropritate headers
           }
      }
       else{ 
          cerr<<"ERROR: variable "<<vid<< " not defined"<<endl;
          error_count++;
       }
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
void CodeGen::visit(ConstraintExpr* condexpr) {
       if(condexpr != NULL)
           condexpr->accept(*this);
}
void CodeGen::visit(RegIDExpr* regidexpr){
       // TODO
}
void CodeGen::visit(RegValExpr* regvalexpr){
        //TODO
}
void CodeGen::visit(IdentLHS* ilhs) {
   string ident = ilhs->name->name;
   if(fcall){                   //NB:A hack to avoid warnings or errors that function name not found as Cinnamon does not support function definitions a.t.m.
       *(outfile[curr])<<ident;
   }
   else{
       if(within_action){
            if(is_action_var(ident) || is_global_var(ident)) 
               *(outfile[curr])<<ident;
            else if(is_active_var(ident)){
                pass_args = true; //pass this variable as argument
                dyn_sym_map_arg[ident].var = "var"+ to_string(var_count++);
                dyn_sym_map_arg[ident].type = activeVarsType[ident];
                *(outfile[curr])<<dyn_sym_map_arg[ident].var;
            }
           else{
              cerr<<"ERROR: variable "<<ilhs->name->name<< " not defined"<<endl;
              error_count++;
           }
           if(is_global_var(ident)){
              if(lhs_assn)
                  global_uses[ident].second.def = 1; 
              else
                  global_uses[ident].second.ref = 1; 
           }
       }else{
           if(is_active_var(ident)){
               *(outfile[curr])<<ident;
               if(lhs_assn && is_global_var(ident)){ //if global variable and being modified in static mode
                  if(!exit_block && !init_block)
                      global_uses[ident].first.def = 1; 
                        
               }
           }
           else{
              cerr<<"ERROR: variable "<<ilhs->name->name<< " not defined"<<endl;
              error_count++;
           }
       }
   }
}

void CodeGen::visit(IndexLHS* ixlhs){
    /* if the base is an identifier such as I[0], table[addr]*/
    if( isInstanceOf<LHS, IdentLHS>(ixlhs->base)){
        std::string base_name= ((IdentLHS*)ixlhs->base)->name->name;
        if(activeCompVars.count(base_name)>0 ){ //This is a component type so call functions based on field. shouldn't be comp though 
            if(lhs_assn){
               cerr<<"ERROR: Cannot allow direct modification to internal data of component "<<base_name<<endl;
               error_count++;
            }
            else{
                *(outfile[curr])<<"get_index(";
                ixlhs->index->accept(*this);
                *(outfile[curr])<<","<<base_name<<");";
            }
        }
        else{          //This is a user defined, so we need to check in local or global vars
            if(find(activeVars.begin(), activeVars.end(), base_name) != activeVars.end()){
                *(outfile[curr])<<base_name<<"[";
                ixlhs->index->accept(*this);
                *(outfile[curr])<<"]";
                if(is_global_var(base_name)){
                    if(lhs_assn)
                        global_uses[base_name].second.def = 1;
                    else
                        global_uses[base_name].second.ref = 1;
                }
            }
        }
   }
   /* if the base is not simply an identifier, such as I.in[0], here the base is I.in(DerefLHS) or I[in][0] with base I[in]*/
   else{
        if(lhs_assn)
            *(outfile[curr])<<"set_index(";
        else
            *(outfile[curr])<<"get_index(";
  
        ixlhs->index->accept(*this);
        *(outfile[curr])<<",";
        
        if( isInstanceOf<LHS, DerefLHS>(ixlhs->base))
             ((DerefLHS*)ixlhs->base)->accept(*this);
        else if(isInstanceOf<LHS, IndexLHS>(ixlhs->base))
            ((IndexLHS*)ixlhs->base)->accept(*this);
       
        *(outfile[curr])<<CLOSEPARAN;
   
   }
}

void CodeGen::visit(DerefLHS* dlhs) {
    if( isInstanceOf<LHS, IdentLHS>(dlhs->base)){ //if we have I.trgaddr or x.y
        std::string base_name= ((IdentLHS*)dlhs->base)->name->name;
        std::string field_name = dlhs->field->name;
        if(activeCompVars.count(base_name) > 0){ //This is a component type so call functions based on field 
            if(lhs_assn){
               cerr<<"ERROR: Cannot allow direct modification to internal data of component "<<base_name<<endl;
               error_count++;
            }
            else{
                if(get_func[mode].count(field_name) > 0){
                    curr_key += get_func[mode][field_name] + base_name + CLOSEPARAN; //need to check if static or dyn
                    while(deref_level>0){
                      curr_key += CLOSEPARAN;
                      deref_level--;
                    }
                    if(within_action){
                          if(!type_expr){
                              if(acc_func_dyn_cat[field_name]){
                                  string search_key = "vars["+ to_string(attr_count++)+"]";
                                  dyn_sym_map[search_key].var = curr_key;
                                  dyn_sym_map[search_key].type = "uint64_t";   //TODO: change
                                  dyn_sym_map[search_key].cat = 1;
                                  *(outfile[curr])<<search_key;
                              }else{
                                  pass_args = true;
                                  string search_key = "var"+ to_string(var_count++);
                                  dyn_sym_map[search_key].var = curr_key;
                                  dyn_sym_map[search_key].type = "uint64_t"; //TODO: change
                                  dyn_sym_map[search_key].cat = 0;
                                  *(outfile[curr])<<search_key;
                              }
                          }
                          else{
                              type_key = curr_key;
                          }
                    }
                    else{ /*could be called from static part*/
                        *(outfile[curr])<<curr_key; //need to check if static or dyn
                    }
                    curr_key="";
                }else{
                    cerr<<"ERROR: "<<dlhs->field->name<<" is not a valid field"<<endl;
                    error_count++;
                }
            }
        }
        else{          //This is a user defined, so we need to check in local or global vars
                
            std::string base_name= ((IdentLHS*)dlhs->base)->name->name;
            std::string field_name = dlhs->field->name;
            if(is_active_var(base_name)){
                *(outfile[curr])<<base_name<<"."<<field_name;
                if(is_global_var(base_name)){
                    if(within_action || init_block || exit_block){
                        if(activeVarsType[base_name]== FILE_T){
                            global_uses[base_name].second.ref = 1;
                            global_uses[base_name].second.def = 1;
                        }
                        else{
                            if(lhs_assn)
                                global_uses[base_name].second.def = 1;
                            else
                                global_uses[base_name].second.ref = 1;
                        } 
                    }else{
                        if(activeVarsType[base_name]== FILE_T){
                            global_uses[base_name].first.ref = 1;
                            global_uses[base_name].first.def = 1;
                        }
                        else{
                            if(lhs_assn)
                                global_uses[base_name].first.def = 1;
                            else
                                global_uses[base_name].first.ref = 1;
                        } 
                    }
                }
            }
            else{
                cerr<<"ERROR: "<<base_name<<" not defined"<<endl;
                error_count++;
            }
        }
   }

   else{        //of the form I.src.id or x.y.z
        if(lhs_assn){
            //*(outfile[curr])<<"set_"<<dlhs->field->name<<OPENPARAN;
        
        }
        else{
            if(get_func[mode].count(dlhs->field->name) > 0){
                curr_key += get_func[mode][dlhs->field->name];
            }else{
                cerr<<"ERROR : "<<dlhs->field->name<<" is not a valid field"<<endl;
                error_count++;
            }
        }
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
    *(outfile[curr])<<OPENPARAN;               
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
    *(outfile[curr])<<op;
    bcexpr->rhs->accept(*this);
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
            string search_key = "vars["+ to_string(attr_count++) + "]";
            dyn_sym_map[search_key].var = new_key;
            dyn_sym_map[search_key].type = 8;
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
    indent(); *(outfile[curr])<<" while( ";     //shall we do this to a header?
    /*---- loop condition ----*/
    whilestmt->loopCondition->accept(*this);
    *(outfile[curr])<<" )"<<endl;
    /*---- Loop Body Start ----*/
    indent(); *(outfile[curr])<<"{"<<endl;
    indentLevel[curr]++;
    whilestmt->loopBody->accept(*this);
    indentLevel[curr]--;
    indent(); *(outfile[curr])<<"}"<<endl;
   /*---- Loop Body End ----*/
    
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
    //*(outfile[curr])<<SEMICOLON;
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
       case DOUBLE:
            type = "double";
            break;
       case CHAR:
            type = "char";
            break;
       case ADDR_INT:
            type = "uintptr_t"; //or app_pc in janus
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
    curr_type= type;
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

void CodeGen::visit(TupleType* tuple) {
    *(outfile[curr])<<"tuple<";
    //*(outfile[curr])<<"struct {}<"; //use struct instead of tuple
    tuple->typelist->accept(*this);
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
       //add extern def to func.h
       globalVars.insert(identifier);
       globalDeclList[identifier] = declLineNo++;
   }
   activeVars.push_back(identifier);
   activeVarsType[identifier] = FILE_T;
   if(within_action){
      actionVars.insert(identifier);
   }
}

/*----- Primitive Type Declarations (int, char, double etc) ------*/
void CodeGen::visit(PrimitiveTypeDecl* ptypedecl) {

   std::string type, identifier, init;
   //if global variable, extern type identifier in func.h and then init in func.cpp
  
   indent();
   ptypedecl->type->accept(*this); 
   identifier = ptypedecl->name->name;
   
   *(outfile[curr]) <<" "<<identifier;
   
   if(global){
       globalDeclList[identifier] = declLineNo++;
       globalVars.insert(identifier);
   }
   else if(within_action){
      actionVars.insert(identifier);
   }
   activeVars.push_back(identifier);
   activeVarsType[identifier] = curr_type; 
   
   if(ptypedecl->init != NULL){
       *(outfile[curr])<<" = ";
       ptypedecl->init->accept(*this);
   }
   *(outfile[curr])<<";"<<endl;
   
}
/*----- Composite Type Declarations (set, map, vector, array, pair, tuple etc) ------*/
void CodeGen::visit(CompositeTypeDecl* ctypedecl) {
   std::string typedecl, type, identifier, initlist;
   
   identifier= ctypedecl->name->name;
   
   indent(); 
   if(isInstanceOf<CompositeType, ArrayType>(ctypedecl->type)){
      ((ArrayType*) ctypedecl->type)->accept(*this);
      *(outfile[curr])<< " "<<identifier;
      *(outfile[curr]) <<"["<<((ArrayType*) ctypedecl->type)->size<<"]";
      if(global){
          globalDeclList[identifier] = declLineNo++;
          globalVars.insert(identifier);
      }
   }
   else{
      ctypedecl->type->accept(*this);
      *(outfile[curr])<< " "<<identifier;
      if(global){
          globalDeclList[identifier] = declLineNo++;
          globalVars.insert(identifier);
      }
   }
   activeVars.push_back(identifier);
   //activeVarsType[identifier] = curr_type;
   if(ctypedecl->init_list->expressions.size() != 0){
       *(outfile[curr])<<" =  {";
       ctypedecl->init_list->accept(*this);
       *(outfile[curr])<<" }";
   }
   *(outfile[curr])<<";"<<endl;
}
/*---- Process Commands of Cinnamon Program  ---- */
void CodeGen::visit(CommandBlock* cmdblock) {
     curr = STAT;
     cmdlevel++;

     //get the name and type of CFE component on which the command is applied
     string scopeVar= cmdblock->component_type->name->name;
     int scopeType = cmdblock->component_type->type;
     comp_map[cmdlevel] = make_pair(scopeType, scopeVar);
     
     activeCompVars.insert(scopeVar);
     
     //generate code to iterate over CFE components
     cmdblock->component_type->accept(*this);
    
     //generate any constraints attached to a command
     if(((ConstraintExpr* )cmdblock->cond)->cond != NULL){
         indent(); *(outfile[curr])<<"if( ";
         ((ConstraintExpr*)cmdblock->cond)->cond->accept(*this);
         *(outfile[curr])<<"){"<<endl;
         indentLevel[curr]++;
     }
     
     //Generate local variable declarations 
     if(cmdblock->localdeclarations){ 
         cmdblock->localdeclarations->accept(*this);
     }
     
     //process all the statements within a command (belong to static analysis phase)
     if(cmdblock->stmts->statements.size()){
        cmdblock->stmts->accept(*this); 
     }
     //process all the enclosed/nested commands 
     if(cmdblock->inner_blocks->nodes.size()){ 
        cmdblock->inner_blocks->accept(*this); 
     }
     //process all actions for the current command
     if(cmdblock->actions->nodes.size()){
        cmdblock->actions->accept(*this);
        curr = STAT; //set it back as actions must have changed it to func.cpp and func.h
     }
     //emit enclosing brackets } 
     if(((ConstraintExpr* )cmdblock->cond)->cond != NULL){ //close conditional brackets.
         indentLevel[curr]--;
         indent(); *(outfile[curr])<<"}"<<endl; 
     }
         
     if(cmdblock->localdeclarations){
         popActiveVars();
     }
     for(int i=0; i<nest_level[cmdlevel]; i++){
         indentLevel[curr]--;
         indent(); *(outfile[curr])<<"}"<<endl;
     }
     nest_level[cmdlevel] = 0;
     cmdlevel--;
}

/*---- Process actions and generate corresponding code ----*/
void CodeGen::visit(Action* action) {
    //Code generated in two parts. Part 1 generates rules in static part of Janus to apply instrumentation.Part 2 encloses action code within callback functions

    /*------------ Step  1. Check if the action is associated with current CFE component type  ------------*/
    int trigger = t_location[action->trigger];          //instrumentation location of the action
    
    std::string cname = comp_map[cmdlevel].second;      //name of the associated CFE/component of the current command level
    
    int ctype =  comp_map[cmdlevel].first;              //type of the associate CFE/component of the current command level
    
    
    //action can only be applied to the most recent or inner most level of CFE
    if(!activeCompVars.count(action->name->name) || cname.compare(action->name->name)){ //cannot do action on outer levels 
       cerr<<"FATAL ERROR: action cannot be performed on the given undefined component "<<action->name<<endl;
       error_count++;
       exit(1);

    }


    /*------------Step 2. Dynamic part - encapsulate code within actions in callback functions ------------*/
    
    curr = TEMP_C;      //temporarily store all the code within an action in temp file, later move to func.cpp and func.h
    outfile_t.open("temp.cpp", ios::out);
    indentLevel[curr]++;
    mode= DYNAMIC;
    within_action = true;

    //generate code for variable declared witin an action 
    if(action->dyn_decls->t_stmts.size() != 0){
        action->dyn_decls->accept(*this);
    }

    //process all the statements defined within an action. 
    //within_action = true;
    action->stmts->accept(*this);
    within_action = false;
    indentLevel[curr]--;
     
    outfile_t.close();
     
    //store arguments to be added, place holder for function parameters, callback function definitions*/
    int arg_count=0; 
    curr = FUNC_C; 
    std::string func_sign;
    func_sign = "void func_"+to_string(ruleID)+"(";
    if(pass_args){
        int count = 0 ;
        for(auto &it: dyn_sym_map_arg){
            func_sign+=it.second.type+" "+it.second.var;
            if(++count < dyn_sym_map_arg.size())
                func_sign+=", ";
        }
        for(auto &it:dyn_sym_map){
            if(count>0){
                count = 0;
                func_sign+=", ";
            }
            if(it.second.cat == 0){
                func_sign += it.second.type+" "+it.first;
                count++;
            }
        }
    }
    func_sign += ")";

    indent();*(outfile[curr])<<func_sign<<"{"<<endl;
    indentLevel[curr]++;
    
    //Copy contents from temp to func.c file
    outfile_t.open("temp.cpp", ios::in);
    string line;
    while(getline(outfile_t, line)){
        *(outfile[curr])<<line<<endl;
    }
    outfile_t.close();
    indentLevel[curr]--;
    indent(); *(outfile[curr])<<"}"<<endl;
    
    //callback function declaration/prototype
    curr = FUNC_H;
    indent(); *(outfile[curr])<<func_sign<<";"<<endl;

    
     
    //insert clean call after all the statements within an actions have been travered and code generated*/
    curr = DYN;
    indent(); *(outfile[curr])<<"void handler_"<<ruleID<<"(JANUS_CONTEXT){"<<endl;
    indentLevel[curr]++;
    indent(); *(outfile[curr])<<"instr_t * trigger = "<< GET_TRIGGER_INSTR<<endl;
    int num =0;
    
    if(dyn_sym_map.size()>0){
        indent(); *(outfile[curr])<<RESET_VAR_COUNT<<endl;
        for(auto it=dyn_sym_map.begin(); it!=dyn_sym_map.end(); it++){
            if(it->second.cat){
                indent(); *(outfile[curr])<<it->second.var<<SEMICOLON<<endl;
            }
        }
    }
    indent(); outfile_d<<INSERT_CLEAN_CALL_PREFIX<<ruleID<<", false";
    if(pass_args){
        int i=0;
        std::string arg_list="";
        for(auto &it: dyn_sym_map_arg){
            arg_list +=",OPND_CREATE_INT32(rule->reg"+to_string(i)+")";
            i++;
            arg_count++;
        }
        for(auto &it: dyn_sym_map){
            if(it.second.cat==0){
                arg_list +=","+it.second.var;
                arg_count++;
            }
        }
        *(outfile[curr])<<","<<arg_count<<arg_list<<");"<<endl;
    }
    else{
        *(outfile[curr])<<","<<arg_count<<");"<<endl;
    }
    indentLevel[curr]--;
    indent(); *(outfile[curr])<<"}"<<endl;
    
    /*------------Step 3. Static part - insert Janus rule for the action------------*/
    mode = STATIC;
    curr = STAT; 
    
    //check if any constraint is associated with an action
    if(((ConstraintExpr *)action->cond)->cond != NULL){
        indent(); *(outfile[curr])<<"if(";
        ((ConstraintExpr *)action->cond)->cond->accept(*this);
        *(outfile[curr])<<"){"<<endl;
        indentLevel[curr]++;
    }
    //insert janus rules for the action. rule generation data includes rule ID, name of CFE, instrumentation location and args to be passed
    if(pass_args && dyn_sym_map_arg.size()){ //check if any data needs to be passed from static part to instrumented callback functions

        indent(); *(outfile[curr])<<"insertCustomRule<"<<to_str[ctype]<<">("<<ruleID<<","<<action->name->name<<","<<trigger<<", true";
        for(auto &it: dyn_sym_map_arg){
            *(outfile[curr])<<","<<it.first;
        }
        *(outfile[curr])<<");"<<endl;
    }else{
        indent(); *(outfile[curr])<<"insertCustomRule<"<<to_str[ctype]<<">("<<ruleID<<","<<action->name->name<<","<<trigger<<", false, 0);"<<endl;
    }


    if(((ConstraintExpr *)action->cond)->cond != NULL){
        indentLevel[curr]--;
        indent(); *(outfile[curr])<<"}"<<endl;
    }
    

    if(action->dyn_decls->t_stmts.size() != 0)
         popActiveVars();

    pass_args = false;
    dyn_sym_map.clear();
    dyn_sym_map_arg.clear();
    var_count = 0;
    attr_count = 0;
    ruleID++;
}
 
/*--- This is the Entry point / Start of Cinnamon Program ---*/
void CodeGen::visit(ProgramBlock* prog) {

    //Step 1: Check any global declarations
    if(prog->globaldeclarations->t_stmts.size() != 0){
        global= true;
        curr = GLOBAL_H;  //common file
        prog->globaldeclarations->accept(*this); 
        outfile_gh.close();
        global= false;
    }
    //Step 2: Start generating code for command blocks
    if(prog->commandblocks->nodes.size())
        prog->commandblocks->accept(*this);
   
    //Step 3: start generartting code for exit blocks
    if(prog->exit_stmts->statements.size()){
        curr= EXIT_C;
        indentLevel[curr]++;
        exit_block = true;
        prog->exit_stmts->accept(*this);
        exit_block = false;
        indentLevel[curr]--;
    }
    //Step 4: start generartting code for init blocks
    if(prog->init_stmts->statements.size()){
        curr= INIT_C;
        indentLevel[curr]++;
        init_block = true;
        prog->init_stmts->accept(*this);
        init_block = false;
        indentLevel[curr]--;
    }
    //Step 5: All global declrations were combined in .globalh file. split them in .stath and .dynh file based on where they are accessed
    outfile_gh.open(global_file, ios::in);
    for(auto &header: stdLib){
      *(outfile[STAT_H])<<"#include <"<<header<<">"<<endl;
      *(outfile[DYN_H])<<"#include <"<<header<<">"<<endl;
    }

    for(auto &it : global_uses){
       string identifier = it.first;
       string line;
       int lineNo = globalDeclList[identifier];
       var_use static_var_use = it.second.first;
       var_use dyn_var_use = it.second.second;
       int count;
       if(static_var_use.ref || static_var_use.def){
          count = 0;
          outfile_gh.clear();
          outfile_gh.seekg(0);
          while(std::getline(outfile_gh, line)){
              if(lineNo == count){
                  *(outfile[STAT_H])<<line<<endl;
                  break;
              }
              count++;
          }
       }
       if(dyn_var_use.ref || dyn_var_use.def){
          count = 0;
          outfile_gh.clear();
          outfile_gh.seekg(0);
          while(std::getline(outfile_gh, line)){
              if(lineNo == count){
                  *(outfile[DYN_H])<<line<<endl;
                  break;
              }
              count++;
           }
       }
    }
    outfile_gh.close();
    //Step 6: pop all global variables from the stack. local ones are popped at the end of scope of each blocked enclosed by {}
    if(prog->globaldeclarations->t_stmts.size())
        popActiveVars();
    
    //TODO: closing braces for command set

    //Step 7: Report any errors
    if(error_count>0){
      cerr<<error_count<<" errors reported: compilation unsuccessful"<<endl;
      exit(1);
    }

    //Step 8: create dynamic handler table
    curr=DYN;
    create_handler_table();
}


/*----- Generate the code to iterate over the Control Flow Elements (CFEs)/Components of a command block ----*/
void CodeGen::visit(Component* comp) {
   if(cmdlevel){ //if inner block
       int upper_level = comp_map[cmdlevel-1].first;
       std::string upper_name = comp_map[cmdlevel-1].second;
       std::string comp_name = comp_map[cmdlevel].second;
       switch(comp->type){
          case INST:
              if(upper_level == LOOP){

              }
              else if(upper_level == BASICBLOCK){
                  indent(); *(outfile[curr])<<"Instruction *End = "<<upper_name<<".instrs+ "<<upper_name<<".size;"<<endl;
                  indent(); *(outfile[curr])<<"for(auto &"<<comp_name<<"="<<upper_name<<".instrs; "<<comp_name<<" < End;"<<comp_name<<"++"<<"){"<<endl; 
                  indentLevel[curr]++;
                   nest_level[cmdlevel]++;
              }
              else if(upper_level == FUNC){
                  indent(); *(outfile[curr])<<"for(auto &"<<comp_name<<": "<<upper_name<<".instrs){"<<endl; 
                  indentLevel[curr]++;
              }
              else if(upper_level == MODULE){
                  indent(); *(outfile[curr])<<"for(auto &func : jc.functions){"<<endl;  //should use name of modeule?
                  indentLevel[curr]++;
                  indent(); *(outfile[curr])<<"for(auto &"<<comp_name<<": func.instrs){"<<endl; 
                  indentLevel[curr]++;
                   nest_level[cmdlevel]+=2;
              }
              else{
                  cerr<<"ERROR: nesting not allowed or invalid"<<endl;
              }
          break;
          case BASICBLOCK:
              if(upper_level == LOOP){
              }
              else if(upper_level == FUNC){
                  indent(); *(outfile[curr])<<"for(auto &"<<comp_name<<": "<<upper_name<<".blocks){"<<endl; 
                   indentLevel[curr]++;
                   nest_level[cmdlevel]++;
              }
              else if(upper_level == MODULE){
                  indent(); *(outfile[curr])<<"for(auto &func : jc.functions){"<<endl;  //should use name of modeule?
                  indentLevel[curr]++;
                  indent(); *(outfile[curr])<<"for(auto &"<<comp_name<<": func.blocks){"<<endl; 
                  indentLevel[curr]++;
                   nest_level[cmdlevel]+=2;
              }
              else{
                  cerr<<"ERROR: nesting not allowed or invalid"<<endl;
              }
          break;
          case LOOP:
              if(upper_level == FUNC){
                  indent(); *(outfile[curr])<<"for(auto &"<<comp_name<<": "<<upper_name<<".loops){"<<endl; 
               indentLevel[curr]++;
                   nest_level[cmdlevel]++;
              }
              else if(upper_level == MODULE){
                  indent(); *(outfile[curr])<<"for(auto &"<<comp_name<<" : jc.loops){"<<endl;  //should use name of modeule?
                   indentLevel[curr]++;
                   nest_level[cmdlevel]++;
              }
              else{
                  cerr<<"ERROR: nesting not allowed or invalid"<<endl;
              }
          break;
          case FUNC:
              if(upper_level == MODULE){
                  indent(); *(outfile[curr])<<"for(auto &"<<comp_name<<" : jc.functions){"<<endl;  //should use name of modeule?
                  indentLevel[curr]++;
                  nest_level[cmdlevel]++;
              }
              else{
                  cerr<<"ERROR: nesting not allowed or invalid"<<endl;
              }
          break;
       }
   }
   else{ //outermost block
       switch(comp->type){
           case MODULE:
              indent(); *(outfile[curr])<<"JanusContext &"<<comp_map[cmdlevel].second<<" = jc;"<<endl;
               break;
           case LOOP:
               indent();*(outfile[curr])<<"for (auto &"<<comp_map[cmdlevel].second << ": jc.loops){"<<endl;
               indentLevel[curr]++;
               nest_level[cmdlevel]++;
           break;
           case FUNC:
               indent(); *(outfile[curr])<<"for (auto &"<<comp_map[cmdlevel].second << ": jc.functions){"<<endl;
               indentLevel[curr]++;
               nest_level[cmdlevel]++;
           break;
           case INST:
               indent(); *(outfile[curr])<<"for (auto &func: jc.functions){"<<endl;
               indentLevel[curr]++;
               indent(); *(outfile[curr])<<"for (auto &"<<comp_map[cmdlevel].second << ": func.instrs){"<<endl;
               indentLevel[curr]++;
               nest_level[cmdlevel]+=2;
           break;
           case BASICBLOCK:
               indent(); *(outfile[curr])<<"for (auto &func: jc.functions){"<<endl;
               indentLevel[curr]++;
               indent(); *(outfile[curr])<<"for (auto &"<<comp_map[cmdlevel].second << ": func.blocks){"<<endl;
               indentLevel[curr]++;
               nest_level[cmdlevel]+=2;
           break;
           default:
            cerr<<"ERROR: Component Type not known! ";
            error_count++;
           break;
       }
   }
}
