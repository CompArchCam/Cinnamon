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
#include "defs.h"
#define MAX_NEST 10
//#define FILE_T 1
#define FILE_T "file"
#define STATIC 0
#define DYNAMIC 1

string curr_type;
string curr_key;                        
string type_key;

std::set<std::string> stdLib;

typedef std::pair<int, std::string> comp_decl;
std::map<int, comp_decl> comp_map;      //key: command level, value: Type and ID of associated control flow component

std::set<std::string> activeCompVars;   //all control flow components currently active
std::deque<std::string> activeVars;     //all vars currently active, popped out at the end of the scope
std::set<std::string> globalVars;       //all global vars
std::set<std::string> actionVars;       //all vars within an action (also part of activeVars)
std::map<string, string> activeVarsType;   //not currently used except for File
std::map <string, int> globalDeclList;          //all global decl and their location in globalh file
std::map<string, std::pair<struct var_use, struct var_use>> global_uses; //refs and defs of global variables, first of pair = use in static and second of pair is use in dynamic
map<int, arg_map> dyn_sym_map;          //variable->argument mapping for variables passed to instrumented action/callback function

string global_file;
std::string inFile;                     //input binary name
string upper_name;                      //ID of CFE encapsulating current CFE

std::stack<int> offset;                 //used to keep track of number of declared in current scope 
int nest_level[MAX_NEST];               //nesting level of CFE for loops
int cmd_argcount[MAX_NEST];
std::map<int,int> cid;                  //maps the lexer assigned ID to ids in the range of 0-4
string comp_type[MAX_NEST];             //TODO:can it be replaced with comp_map[cmdlevel].type or name?

int c_count = 0;
int var_count = 0;      
int deref_level = 0;                    //dereference level for exprs such as X.y.z (deref_level = 2)
int declLineNo = 0;                     //line number of global decl in globalh file
int cmdlevel = -1;                      //command level
int dist;                               //


bool within_action = false;             //if we are within an action.
bool type_expr = false;
bool pass_args = false;                 //set to true if args need to be passed to instrumented routine
bool fcall = false;                     //used as a hack for function names. TODO: function decl currently not supported
bool exit_block = false;                //to specifiy if we are within an exit block
bool init_block = false;                //to specify if we are within an init block


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

void CodeGen::indent(){
    for(int i=0; i<indentLevel[curr]; i++)
        *(outfile[curr])<<"    ";
}
CodeGen::CodeGen(std::string filename){  
    outfile_ins.open(filename+".instr", ios::out);
    outfile_load.open(filename+".loader", ios::out);
    outfile_f.open(filename+".func", ios::out);
    outfile_fh.open(filename+".funch", ios::out);
    outfile_e.open(filename+".exit", ios::out);
    outfile_i.open(filename+".init", ios::out);
    outfile_gh.open(filename+".globalh", ios::out);     //Header with Global Definitions (Note: temp, translation in STATH and DYNH)
    outfile_sh.open(filename+".stath", ios::out);       //Global Var Definitions for Static Part
    outfile_dh.open(filename+".dynh", ios::out);        //Global Var Definitions for Dynamic Part
    global_file = filename+".globalh" ;
    
    inFile = filename;
    outfile[INSTR] = &outfile_ins;
    outfile[LOAD] = &outfile_load;
    outfile[FUNC_C]= &outfile_f;
    outfile[FUNC_H]= &outfile_fh;
    outfile[EXIT_C] = &outfile_e;
    outfile[INIT_C] = &outfile_i;
    outfile[TEMP_C] = &outfile_t;
    outfile[GLOBAL_H] = &outfile_gh;
    outfile[STAT_H] = &outfile_sh;
    outfile[DYN_H] = &outfile_dh;
    curr= INSTR;
    get_func[STATIC]= get_static_func;
    get_func[DYNAMIC]= get_dyn_func; 
    cType[STATIC] = cTypeStat; 
    cType[DYNAMIC] = cTypeDyn;
    dType[STATIC] = dTypeStat; 
    //dType[DYNAMIC] = dTypeDyn;
    cid[INST] = 0;
    cid[BASICBLOCK] = 1;
    cid[LOOP] = 2;
    cid[FUNC] = 3;
    cid[MODULE] = 4;
    generate_template[cid[INST]] = &CodeGen::generate_template_instr;
    generate_template[cid[BASICBLOCK]] = &CodeGen::generate_template_basicblock;
    generate_template[cid[LOOP]] = &CodeGen::generate_template_loop;
    generate_template[cid[FUNC]] = &CodeGen::generate_template_func;
    generate_template[cid[MODULE]] = &CodeGen::generate_template_module;
}
CodeGen::~CodeGen(){
    outfile_ins.close();
    outfile_load.close();
    outfile_f.close();
    outfile_fh.close();
    outfile_i.close();
    outfile_e.close();
    outfile_t.close();
    outfile_gh.close();
    outfile_sh.close();
    outfile_dh.close();

}

void CodeGen::create_handler_table(){
    indentLevel[LOAD]=0;
    for(int i=0; i<actionID; i++){
        indent(); *(outfile[LOAD])<<"BPatch_function *func_"<<i<<" = findFuncByName (appImage, (char *) \"func_"<<i<<"\");"<<endl; 
    }
    if(actionID > 0){
        *(outfile[LOAD])<<"if(";
        for(int i=0; i< actionID; i++){
           *(outfile[LOAD])<<"!func_"<<i;
           if(i < actionID-1) //last one
               *(outfile[LOAD])<<" || ";
        }
        *(outfile[LOAD])<<")" <<endl;
        indentLevel[LOAD]++;
        indent(); *(outfile[LOAD])<< RTN_FAILURE <<endl;

        indentLevel[LOAD]--;
    }
    indentLevel[LOAD]=0;
}
void CodeGen::visit(StatementList* stmtlst){
    
    if(stmtlst->statements.size() != 0)
    {
        for(auto stmt: stmtlst->statements){
            stmt->accept(*this);
        }
    }
}

void CodeGen::visit(NodeList* nl) {
    for(auto node: nl->nodes){
        node->accept(*this);
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
   if(within_action){ //or mode = dynamic
       if(is_action_var(vid) || is_global_var(vid)){
           if(is_global_var(vid)){
               global_uses[vid].second.ref = 1;
               if(global_uses[vid].first.def){ //if already modified in static part
                  pass_args = true; //pass this variable as argument
                  //dyn_sym_map[var_count].ctype = "uint64_t"; //todo: change later
                  dyn_sym_map[var_count].ctype = activeVarsType[vid]; 
                  dyn_sym_map[var_count].dtype = "BPatch_constExpr"; //todo: change later
                  dyn_sym_map[var_count].initval = vid;
                  *(outfile[curr])<<"var"<<var_count;
                  var_count++;
                  return;
               }
           }
           *(outfile[curr])<<vid;
       }
       else if(is_active_var(vid)){
            pass_args = true; //pass this variable as argument
          dyn_sym_map[var_count].ctype = activeVarsType[vid]; 
          //dyn_sym_map[var_count].ctype = "uint64_t"; //todo: change later
          dyn_sym_map[var_count].dtype = "BPatch_constExpr"; //todo: change later
          dyn_sym_map[var_count].initval = vid;
          *(outfile[curr])<<"var"<<var_count;
          var_count++;
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
void CodeGen::visit(NullPtr* ptr) {
    *(outfile[curr]) << "NULL";
}
void CodeGen::visit(ConstraintExpr* cond) {
    if(cond != NULL)
      cond->accept(*this);
}
void CodeGen::visit(RegIDExpr* regidexpr){
}
void CodeGen::visit(RegValExpr* regvalexpr){
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

void CodeGen::visit(IdentLHS* ilhs) {
   string ident = ilhs->name->name;
   if(fcall){
      *(outfile[curr])<<ident;
   }
   else{
       if(within_action){
            if(is_action_var(ident) || is_global_var(ident)) 
               *(outfile[curr])<<ident;
            else if(is_active_var(ident)){
                pass_args = true; //pass this variable as argument
               dyn_sym_map[var_count].ctype = activeVarsType[ident]; 
               dyn_sym_map[var_count].dtype = "BPatch_constExpr"; 
               dyn_sym_map[var_count].initval = ident;
               *(outfile[curr])<<"var"<<var_count<<endl;
               var_count++;

            }
           else{
              cerr<<"ERROR: variable "<<ident<< " not defined"<<endl;
              error_count++;
           }
          if(is_global_var(ident)){
              if(lhs_assn)
                  global_uses[ident].second.def = 1;
              else
                  global_uses[ident].second.ref = 1;
         }
       }
       else{
           if(is_active_var(ident)){
               *(outfile[curr])<<ident;
               if(lhs_assn && is_global_var(ident)){ //if global variable and being modified in static mode
                   if(!exit_block && !init_block)
                      global_uses[ident].first.def = 1; 
                    
               }
           }
           else{
              cerr<<"ERROR: variable "<<ident<< " not defined"<<endl;
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
    if( isInstanceOf<LHS, IdentLHS>(dlhs->base)){
        std::string base_name= ((IdentLHS*)dlhs->base)->name->name;
        std::string field_name = dlhs->field->name;
        //1. Check if the attribute belongs to a CFE compoenent
        if(activeCompVars.count(base_name)){
            if(lhs_assn){
               cerr<<"ERROR: Cannot allow direct modification to internal data of component "<<base_name<<endl;
               error_count++;
            }
            else{
                //1.1 if access is within instrumented code
                if(mode == DYNAMIC){
                    //1.1.1 check whether field/attribute is accessable dynamically
                    if(get_func[mode].count(field_name)){
                        pass_args = true;
                        dyn_sym_map[var_count].dtype = get_func[mode][field_name];
                        dyn_sym_map[var_count].ctype = cType[mode][field_name];
                        if(init.count(field_name))
                            dyn_sym_map[var_count].initval = init[field_name];
                    }
                    //1.1.2 if attribute/field not accessible dynamically but avaialble statically, pass on the value as argument to callback snippet
                    else if(get_func[STATIC].count(field_name)){
                        curr_key += get_func[STATIC][field_name] + base_name + CLOSEPARAN;
                        while(deref_level>0){
                          curr_key += CLOSEPARAN;
                          deref_level--;
                        }
                        pass_args = true;
                        dyn_sym_map[var_count].dtype = dType[STATIC][field_name];
                        dyn_sym_map[var_count].ctype = cType[STATIC][field_name];
                        dyn_sym_map[var_count].initval = curr_key;
                        curr_key = ""; //may need to remove
                    }
                    //1.1.3 attribute not accessible or supported
                    else{
                        cerr<<"ERROR: "<<field_name<<" is not a valid field"<<endl;
                        error_count++;
                    }

                    if(!type_expr){
                        *(outfile[curr])<<"var"<<var_count++;
                    }
                    else{
                        type_key = curr_key;
                    }
                }
                //1.2 if access is within analysis/static code and attribute is available
                else if(mode == STATIC && get_func[mode].count(field_name)){
                    curr_key += get_func[mode][field_name] + base_name + CLOSEPARAN;
                    while(deref_level>0){
                      curr_key += CLOSEPARAN;
                      deref_level--;
                    }
                    *(outfile[curr])<<curr_key;
                    curr_key=""; 
                }
                else{
                    cerr<<"ERROR: "<<field_name<<" is not a valid field"<<endl;
                    error_count++;
                }
                curr_key="";
            }
        }
        //2 attribute/field may belong to a user defined variable,check in active vars
        else{          
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

   else{ //NOT an instance of IdentLHS, explore further
        if(lhs_assn){
            //*(outfile[curr])<<"set_"<<dlhs->field->name<<OPENPARAN;
        
        }
        else{
            if(get_func[mode].count(dlhs->field->name) > 0){
                curr_key += get_func[mode][dlhs->field->name];
            }else{
                cerr<<"ERROR: "<<dlhs->field->name<<" is not a valid field"<<endl;
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
     /*if lhs has Component.field, then we just do assignment*/
     /*if lhs has ident, it should not simply be a component. if it's component then an error. should be predefined*/
     /*if rhs has ident, it should also not be simply a component. */
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
        string type_name = types[typeexpr->type_name];
        if(within_action){
            type_expr = true;
            typeexpr->lhs->accept(*this);
            type_expr = false;
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
         type = "uintptr_t"; 
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
    curr_type = type;
    *(outfile[curr])<<type;
}

void CodeGen::visit(ArrayType* array) {
    array->type->accept(*this);
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
        globalVars.insert(identifier);
        globalDeclList[identifier] = declLineNo++;
     }
     activeVars.push_back(identifier);
     activeVarsType[identifier] = FILE_T;
     if(within_action){
       actionVars.insert(identifier);
     }
}

void CodeGen::visit(PairType* pair) {
    *(outfile[curr])<<"pair<";
    pair->type1->accept(*this);
    *(outfile[curr])<<",";
    pair->type1->accept(*this);
}

void CodeGen::visit(DictType* dict) {
   /*PrimitiveType* keytype;   //add such to record values, if a variable is prim type, it is not recorded type. 
   VType* valuetype;*/
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
   indent();
   ptypedecl->type->accept(*this); 
   identifier= ptypedecl->name->name;
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

   if(ctypedecl->init_list->expressions.size() != 0){
       *(outfile[curr])<<" =  {";
       ctypedecl->init_list->accept(*this);
       *(outfile[curr])<<" }";
   }
   *(outfile[curr])<<";"<<endl;
}
/*---- Process Commands of Cinnamon Program  ---- */
void CodeGen::visit(CommandBlock* cmdblock) {
     curr = INSTR;
     cmdlevel++;

     //get the name and type of CFE component on which the command is applied
     string scopeVarName= cmdblock->component_type->name->name;
     int scopeVarType= cmdblock->component_type->type;
     
     dist = 0;
     if(cmdlevel == 0) c_count =0;
     cmd_argcount[cmdlevel] = 0;
     //start of module block
     comp_map[cmdlevel] = make_pair(scopeVarType, scopeVarName);

     activeCompVars.insert(scopeVarName);
    
     //generate code to iterate over CFE components
     cmdblock->component_type->accept(*this);
     
     indentLevel[curr]++;
    
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
     }
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
    
    indentLevel[curr]--;
    nest_level[cmdlevel] = 0;
    cmdlevel--;
}
/*---- Process actions and generate corresponding code ----*/
void CodeGen::visit(Action* action) {
    /*each action is associated with current component type*/
     std::string compName = action->name->name;
    /*------------Step 1. Static part - only insert custom rule if static part condition not always TRUE------------*/
    //mode = INSTR;
    int trigger = t_location[action->trigger];          //instrumentation location of the action

    std::string cname = comp_map[cmdlevel].second;      //name of the associated CFE/component of the current command level
    
    int ctype =  comp_map[cmdlevel].first;              //type of the associate CFE/component of the current command level

    //action can only be applied to the most recent or inner most level of CFE
    if(!activeCompVars.count(compName) || cname.compare(compName)){ //cannot do action on outer levels 
        cerr<<"FATAL ERROR: action cannot be performed on the given undefined component "<<compName<<endl;
        error_count++;
        exit(1);
    }
    //Add instArgs
    indent(); *(outfile[curr])<<DEFINE_ARGS_VECTOR<<actionID<<";"<<endl;
    
    if(((ConstraintExpr *)action->cond)->cond != NULL){
        indent(); *(outfile[curr])<<"if(";
        ((ConstraintExpr *)action->cond)->cond->accept(*this);
        *(outfile[curr])<<"){"<<endl;
        indentLevel[curr]++;
    }
    /*------------Step 2. Dynamic part - encapsulate code within actions in callback functions ------------*/
    //mode= LOAD;
    
    mode = DYNAMIC;
    /*----------------- save to temp file--------------------------*/
    curr = TEMP_C;
    outfile_t.open(inFile+".temp", ios::out);
    indentLevel[curr]++;
    within_action = true;
    
    //generate code for variable declared witin an action 
    if(action->dyn_decls->t_stmts.size() != 0){
        action->dyn_decls->accept(*this);
    }

    //process all the statements defined within an action. 
    action->stmts->accept(*this);
    within_action = false;
    indentLevel[curr]--;
     
    outfile_t.close();
    /*---------------- close the temp file -----------------------------*/ 
    
    /*-- Step 2.1 : Go through statements in the action, store arguments to be added, place holder for function parameters*/
    int arg_count=0; 
    curr = FUNC_C;

    std::string func_sign;              //callback function signature
    
    func_sign = "void func_"+to_string(actionID)+"(";
    if(pass_args || dyn_sym_map.size()){
      for(auto it: dyn_sym_map){
        func_sign += it.second.ctype+ " var"+to_string(arg_count);
        if(++arg_count < dyn_sym_map.size()){
            func_sign+=",";
        }
      }
    }
    func_sign+=")";
    indent();*(outfile[curr])<<func_sign<<"{"<<endl;
    indentLevel[curr]++;
    
    //Copy contents from temp to func.c file
    outfile_t.open(inFile+".temp", ios::in);
    string line;
    while(getline(outfile_t, line)){
        *(outfile[curr])<<line<<endl;
    }
    outfile_t.close();
    indentLevel[curr]--;
    indent(); *(outfile[curr])<<"}"<<endl;
    
    /*function prototype*/
    curr = FUNC_H;
    indent(); *(outfile[curr]) <<"LIB_EXPORT "<<func_sign<<";"<<endl;
   
/*------------Step 3. Static part - insert code snippets for the instrumented action------------*/
    mode = STATIC;
    curr = INSTR;
    //Add instrumentaiton arguments
    if(pass_args || dyn_sym_map.size()){ 
      int argcount=0;
      for(auto it: dyn_sym_map){
        indent(); *(outfile[curr])<<it.second.dtype<<" arg"<<cmd_argcount[cmdlevel];
        if(!it.second.initval.empty())
           *(outfile[curr])<<"("<<it.second.initval<<")";
        *(outfile[curr])<<";"<<endl;
        indent(); *(outfile[curr])<<"instArgs"<<actionID<<".push_back(&arg"<<cmd_argcount[cmdlevel]<<" );"<<endl;
        cmd_argcount[cmdlevel]++;
      } 
    }
    //insert snippet for instrumented callback functions
    if(ctype == INST){  //if component/CFE is instruction, emit additional code
        indent();*(outfile[INSTR])<<DECLARE_INS_POINTS<<actionID<<";"<<endl;
        indent();*(outfile[INSTR])<<"appImage->findPoints("<<comp_map[cmdlevel].second<<".second, points"<<actionID<<");"<<endl; 
        compName = "points"+to_string(actionID)+"[0]";
    }
    indent(); *(outfile[curr])<<"insertSnippetAt<"<<comp_type[cmdlevel]<<">(app,"<<compName<<", func_"<<actionID<<","<<trigger<<","<<pass_args<<", instArgs"<<actionID<<");"<<endl;
    
    //insert closing brackets for static constraints
    if(((ConstraintExpr *)action->cond)->cond != NULL){
        indentLevel[curr]--;
        indent(); *(outfile[curr])<<"}"<<endl;
    }



    pass_args = false;

    if(action->dyn_decls->t_stmts.size() != 0)
         popActiveVars();

     dyn_sym_map.clear();
    curr = INSTR; 
    var_count = 0;
    actionID++;
}
 
void CodeGen::visit(ProgramBlock* prog) {
    /* global Decl go to dynamic part for now. Are two levels of nesting enough for this case? I.e. scope and component type? Shall we std::stringroduce other ways or levels to static part? */
    
    //Step 1: Check any global declarations 
    if(prog->globaldeclarations->t_stmts.size() != 0){
        global= true;
        curr = GLOBAL_H;
        prog->globaldeclarations->accept(*this); //start of dynamic file*/
        outfile_gh.close();
        global= false;
    }
    //Step 2: Start generating code for command blocks
    if(prog->commandblocks->nodes.size())
        prog->commandblocks->accept(*this);
    
    //Step 3: start generating code for exit blocks
    if(prog->exit_stmts->statements.size()){
        curr= EXIT_C;
        exit_block = true;
        indentLevel[curr]++;
        prog->exit_stmts->accept(*this);
        exit_block = false;
        indentLevel[curr]--;
    }
    //Step 4: start generating code for init blocks
    if(prog->init_stmts->statements.size()){
        curr= INIT_C;
        init_block = true;
        indentLevel[curr]++;
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
    
    if(prog->globaldeclarations->t_stmts.size() != 0)
        popActiveVars();
    
    curr=LOAD;
    create_handler_table();
   //Step 7: Report any errors
    if(error_count>0){
      cerr<<error_count<<" errors reported: compilation unsuccessful"<<endl;
      exit(1);
    }
   
}
void CodeGen::generate_template_module(bool called_direct){
   string curr_name;
   bool generate_code = true;
   if(called_direct){
       if(cmdlevel==0){
          curr_name = comp_map[cmdlevel].second;
       }
       else{//error: module cannot be nested inside another comp
          return; 
       }
   }else{
       if(cmdlevel >0 && comp_map[cmdlevel-1].first == MODULE){
          generate_code = false;
          curr_name = comp_map[cmdlevel-1].second;
       }
       else{
           curr_name = "Comp"+to_string(c_count); 
           c_count++;
       }
      // upper_name = curr_name;
   }
   if(generate_code){
       indent();*(outfile[INSTR])<<"for(auto &"<<curr_name<<": *(appImage->getModules())){"<<endl;
       indentLevel[curr]++;
       nest_level[cmdlevel]++;
   }
   upper_name = curr_name;
   return;
}
void CodeGen::generate_template_instr(bool called_direct){
    int upper_id;
    string curr_name;
    if(called_direct){
        if(cmdlevel==0){
            dist  = 3;
            curr_name = comp_map[cmdlevel].second;
        }else{
            if( cid[comp_map[cmdlevel-1].first] == 3 or cid[comp_map[cmdlevel-1].first] == 4) //based on numbering
               dist = (cid[comp_map[cmdlevel-1].first] - cid[comp_map[cmdlevel].first]) - 1; 
            else
               dist = cid[comp_map[cmdlevel-1].first] - cid[comp_map[cmdlevel].first];
            curr_name = comp_map[cmdlevel].second;
        }
    }
    else{//error. cannot be called otherwise
       return; 
    }
    if(dist)
        (this->*generate_template[cid[INST]+1])(false);
    indent();*(outfile[INSTR])<<DECLARE_INS_VECTOR<<endl;
    indent();*(outfile[INSTR])<<upper_name<<"->getInstructions(AllInstrs);"<<endl;
    indent();*(outfile[INSTR])<<"for (auto &"<<curr_name<<": AllInstrs){"<<endl; //TODO: change in the for loop as well (for auto &L : AllLoops)
   indentLevel[curr]++;
   nest_level[cmdlevel]++;

}
void CodeGen::generate_template_basicblock(bool called_direct){
    string curr_name;
    int upper_id;
    bool generate_code = true;
    if(called_direct){
        if(cmdlevel==0){
            dist  = 2;
            upper_id = cid[FUNC];
            curr_name = comp_map[cmdlevel].second;
        }
        else{
            if( cid[comp_map[cmdlevel-1].first] == 3 || cid[comp_map[cmdlevel-1].first] == 4) //based on numbering
               dist = (cid[comp_map[cmdlevel-1].first] - cid[comp_map[cmdlevel].first]) - 1; 
            else
               dist = cid[comp_map[cmdlevel-1].first] - cid[comp_map[cmdlevel].first];
            if(comp_map[cmdlevel-1].first == LOOP)
                upper_id = cid[comp_map[cmdlevel-1].first];
            else
                upper_id = cid[FUNC];
        }
        curr_name = comp_map[cmdlevel].second; 
    }
    else{
       dist--; 
       if(cmdlevel > 0 && comp_map[cmdlevel-1].first == BASICBLOCK) {
           curr_name = comp_map[cmdlevel-1].second; //no need to generate further code
           generate_code = false;
       }
       else{ 
           curr_name = "Comp"+to_string(c_count++);
       }
       upper_id = cid[FUNC];
    }
    if(dist) (this->*generate_template[upper_id])(false);
    if(generate_code){
        indent();*(outfile[INSTR])<<DECLARE_BB_VECTOR<<endl;
        indent();*(outfile[INSTR])<<upper_name<<"->getCFG()->getAllBasicBlocks(AllBlocks);"<<endl;
        indent();*(outfile[INSTR])<<"for(auto &"<<curr_name<<" : AllBlocks){"<<endl;
        indentLevel[curr]++;
        nest_level[cmdlevel]++;
        upper_name = curr_name;
    }
   return;
}

void CodeGen::generate_template_loop(bool called_direct){
    string curr_name;
    if(called_direct){
        if(cmdlevel==0){
            dist  = 2;
        }else{
            dist = cid[comp_map[cmdlevel-1].first] - cid[LOOP]; //should always be positive   
        }
        curr_name = comp_map[cmdlevel].second;
    }else{
        return;
        //cannot be called indirectly 
    } 
       
    if(dist>0)(this->*generate_template[cid[LOOP]+1])(false);
    indent();*(outfile[INSTR])<<DECLARE_LOOP_VECTOR<<endl;
    indent();*(outfile[INSTR])<<" BPatch_flowGraph * CFG = "<<upper_name<<"->getCFG();"<<endl;
    indent();*(outfile[INSTR])<<" CFG->getLoops(AllLoops);"<<endl;
    indent();*(outfile[INSTR])<<"for (auto &"<<curr_name<<": AllLoops){"<<endl; //TODO: change in the for loop as well (for auto &L : AllLoops)
    indentLevel[curr]++;
    nest_level[cmdlevel]++;
   return;
}
void CodeGen::generate_template_func(bool called_direct){
    string curr_name;
    bool generate_code = true;
    if(called_direct){
        if(cmdlevel==0){
            dist  = 0;
            upper_name = "appImage";
            curr_name = comp_map[cmdlevel].second;
        }
        else{
           dist = cid[MODULE] - cid[FUNC]; 
           upper_name = comp_map[cmdlevel-1].second;
        }
        curr_name = comp_map[cmdlevel].second; 
    }
    else{
       dist--; 
       if(cmdlevel>0 && comp_map[cmdlevel-1].first == FUNC) {
           curr_name = comp_map[cmdlevel-1].second; //no need to generate further code
           generate_code = false;
       }
       else{ 
           curr_name = "Comp"+to_string(c_count++);
       }
    }
    if(dist) (this->*generate_template[cid[FUNC]+1])(false);
    if(generate_code){
       indent();*(outfile[INSTR])<<"for(auto &"<<curr_name<<" : *("<<upper_name<<"->getProcedures())){"<<endl;
       indentLevel[curr]++;
       nest_level[cmdlevel]++;
       upper_name = curr_name;
    }
   return;
}
void CodeGen::visit(Component* comp) {
     //check if it's a valid type
     (this->*generate_template[cid[comp->type]])(true);
     comp_type[cmdlevel] = bpatch_type[comp->type];
     return;
}
