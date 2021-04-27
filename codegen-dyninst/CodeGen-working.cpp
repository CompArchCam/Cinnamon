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
#define FILE_T 1
#define STATIC 0
#define DYNAMIC 1
struct var_use{
    int ref;
    int def;
    var_use(): ref(0), def(0) {}
};
typedef struct {
    string dtype; 
    string initval;
    string ctype;
} arg_map;
typedef std::set<std::string> VarDefs;
int c_count=0;
VarDefs activeCompVars;
std::string scopeVarName;
int scopeVarType;
std::set<std::string> stdLib;
std::deque<std::string> activeVars;
std::map<string, int> activeVarsType;
std::stack<int> offset;
typedef std::pair<int, std::string> comp_decl;
std::map<int, comp_decl> comp_map;
int cmdlevel = -1;
int nest_level[MAX_NEST];
int cmd_argcount[MAX_NEST];
arg_data rule_data;
std::map<string, std::pair<struct var_use, struct var_use>> global_uses; //static and dynamic
string upper_name;
std::map<int,int> cid;
bool pass_args= false;
bool fcall = false;
//string comp_type;
string comp_type[MAX_NEST];
string curr_key;
string type_key;
int dist;
bool within_action=false;
bool type_expr = false;
int var_count = 0;
int deref_level=0;
map<int, arg_map> dyn_sym_map;
std::string inFile;


std::set<std::string> globalVars;
std::set<std::string> actionVars;

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
void CodeGen::indent(){
    for(int i=0; i<indentLevel[curr]; i++)
        *(outfile[curr])<<"    ";
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
    outfile_ins.open(filename+".instr", ios::out);
    outfile_load.open(filename+".loader", ios::out);
    outfile_f.open(filename+".func", ios::out);
    outfile_fh.open(filename+".funch", ios::out);
    outfile_e.open(filename+".exit", ios::out);
    outfile_i.open(filename+".init", ios::out);
    //outfile_t.open(filename+"_temp.cpp", ios::out);
    inFile = filename;
    outfile[INSTR] = &outfile_ins;
    outfile[LOAD] = &outfile_load;
    outfile[FUNC_C]= &outfile_f;
    outfile[FUNC_H]= &outfile_fh;
    outfile[EXIT_C] = &outfile_e;
    outfile[INIT_C] = &outfile_i;
    outfile[TEMP_C] = &outfile_t;
    curr= INSTR;
    get_func[STATIC]= get_static_func;//not needed 
    get_func[DYNAMIC]= get_dyn_func; //not needed
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
    outfile_i.close();
    outfile_e.close();
    outfile_t.close();

}

void CodeGen::create_handler_table(){
    indentLevel[LOAD]=0;
    cout<<"actionID: "<<actionID<<endl; 
    for(int i=0; i<actionID; i++){
        indent(); *(outfile[LOAD])<<"BPatch_function *func_"<<i<<" = findFuncByName (appImage, (char *) \"func_"<<i<<"\");"<<endl; 
    }
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
                  dyn_sym_map[var_count].ctype = "uint64_t"; //todo: change later
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
          dyn_sym_map[var_count].ctype = "uint64_t"; //todo: change later
          dyn_sym_map[var_count].dtype = "BPatch_constExpr"; //todo: change later
          dyn_sym_map[var_count].initval = vid;
          *(outfile[curr])<<"var"<<var_count;
          var_count++;
       }
       else{ 
          cout<<"ERROR: variable "<<vid<< " not defined"<<endl;
          error_count++;
       }
   }
   else{//access in static part
      if(is_active_var(vid)){
           *(outfile[curr])<<vid;
           if(is_global_var(vid)){
              global_uses[vid].first.ref = 1; //to be later used for adding to appropritate headers
           }
      }
       else{ 
          cout<<"ERROR: variable "<<vid<< " not defined"<<endl;
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
        cout<<"ERROR: bool type not known! ";
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
               dyn_sym_map[var_count].ctype = "uint64_t"; //todo: change later
               dyn_sym_map[var_count].dtype = "BPatch_constExpr"; //todo: change later
               dyn_sym_map[var_count].initval = ident;
               *(outfile[curr])<<"var"<<var_count<<endl;
               var_count++;

            }
           else{
              cout<<"ERROR: variable "<<ident<< " not defined"<<endl;
              error_count++;
           }
       }
       else{
           if(is_active_var(ident)){
               *(outfile[curr])<<ident;
               if(lhs_assn && is_global_var(ident)){ //if global variable and being modified in static mode
                  global_uses[ident].first.def = 1; 
                    
               }
           }
           else{
              cout<<"ERROR: variable "<<ident<< " not defined"<<endl;
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
               // *(outfile[curr])<<"set_index"<<base_name; //should not have component on LHS, because func call not allowed on lhs
               cout<<"ERROR: Cannot allow direct modification to internal data of component "<<base_name<<endl;
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
        if(activeCompVars.count(base_name) > 0){ //This is a component type so call functions based on field 
            if(lhs_assn){
               cout<<"ERROR: Cannot allow direct modification to internal data of component "<<base_name<<endl;
               error_count++;
               // *(outfile[curr])<<set_field(dlhs->field->name,base_name);
            }
            else{
                if(mode == DYNAMIC){
                    if(get_func[mode].count(dlhs->field->name)){
                       //add relevant value
                        pass_args = true;
                        dyn_sym_map[var_count].dtype = get_func[mode][dlhs->field->name];
                        dyn_sym_map[var_count].ctype = cType[mode][dlhs->field->name];
                        if(init.count(dlhs->field->name))
                            dyn_sym_map[var_count].initval = init[dlhs->field->name];
                    }
                    else if(get_func[STATIC].count(dlhs->field->name)){
                       //add const_Expr
                        curr_key += get_func[STATIC][dlhs->field->name] + base_name + CLOSEPARAN; //need to check if static or dyn
                        while(deref_level>0){
                          curr_key += CLOSEPARAN;
                          deref_level--;
                        }
                        pass_args = true;
                        dyn_sym_map[var_count].dtype = dType[STATIC][dlhs->field->name];
                        dyn_sym_map[var_count].ctype = cType[STATIC][dlhs->field->name];
                        dyn_sym_map[var_count].initval = curr_key;
                        curr_key = ""; //may need to remove
                    }
                    else{
                        cout<<"ERROR: "<<dlhs->field->name<<" is not a valid field"<<endl;
                        error_count++;
                    }
                    if(!type_expr){
                       // string search_key = "var"+ to_string(var_count);
                       // var_count++;
                        *(outfile[curr])<<"var"<<var_count++;
                    }
                    else{
                        type_key = curr_key;
                    }
                }
                else if(mode == STATIC && get_func[mode].count(dlhs->field->name)){
                    curr_key += get_func[mode][dlhs->field->name] + base_name + CLOSEPARAN; //need to check if static or dyn
                    while(deref_level>0){
                      curr_key += CLOSEPARAN;
                      deref_level--;
                    }
                    *(outfile[curr])<<curr_key; //need to check if static or dyn
                    curr_key=""; //may need to remove
                }
                else{
                    cout<<"ERROR: "<<dlhs->field->name<<" is not a valid field"<<endl;
                    error_count++;
                }
                curr_key=""; //may need to add
            }
        }
        else{          //This is a user defined, so we need to check in local or global vars
                
        }
   }

   else{ //NOT and instance of IdentLHS, explore further
        if(lhs_assn){
            //*(outfile[curr])<<"set_"<<dlhs->field->name<<OPENPARAN;
        
        }
        else{
            if(get_func[mode].count(dlhs->field->name) > 0){
                curr_key += get_func[mode][dlhs->field->name];
            }else{
                cout<<"ERROR: "<<dlhs->field->name<<" is not a valid field"<<endl;
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
        cout<<"ERROR: operator type not known! ";
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
            cout<<"ERROR: operator type not known! ";
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
            cout<<"ERROR: operator type not known! ";
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
            cout<<"ERROR: operator type not known! ";
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
            cout<<"ERROR: operator type not known! ";
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
            #if 0
            string new_key = "is_type(" + type_key + "," + type_name + CLOSEPARAN;
            string search_key = "vars["+ to_string(var_count++) + "]";
            dyn_sym_map[search_key].var = new_key;
            dyn_sym_map[search_key].type = 8;
            *(outfile[curr])<<search_key;
           #endif
        }
        else{
            *(outfile[curr])<<"is_type"<<OPENPARAN;
            typeexpr->lhs->accept(*this);
            *(outfile[curr])<<", "<<type_name<<CLOSEPARAN;
        }
   }
   else{
       cout<<"ERROR: "<<typeexpr->type_name<<" is not a valid type"<<endl;
       error_count++;
   }
}
void CodeGen::visit(RuleExpr* ruleexpr){
    ruleexpr->arg->accept(*this); //add this to arg list as well
    *(outfile[curr])<<" = ";
    ruleexpr->value->accept(*this);
    *(outfile[curr])<<";"<<endl;

}

void CodeGen::visit(ExprStatement* estmt) {
    /* expression statements are part of action so go to dynamic handler*/
    indent(); estmt->expr->accept(*this);
    *(outfile[curr])<<SEMICOLON<<endl;

}
//TODO: merge ifelse statements into one
void CodeGen::visit(ifStmt* ifstmt) {
     /* selection statements are part of action so go to dynamic handler*/
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
         type = "uint64_t"; //or app_pc in janu??
         break;
       case FILE_LINE:
        type = "line";
        break;
       default:
            type = "UNKNOWN";
            cout<<"ERROR: variable type not known! ";
            error_count++;
            break;
    }
    *(outfile[curr])<<type;
}

void CodeGen::visit(ArrayType* array) {
    array->type->accept(*this);
}
void CodeGen::visit(FileTypeDecl* ftypedecl) {
   std::string identifier = ftypedecl->ident->name;
          //if global variable, extern type identifier in func.h and then init in func.cpp

    indent();
    *(outfile[curr])<<"FILE "<<identifier;

    if(ftypedecl->f_var) *(outfile[curr])<<"("<<ftypedecl->f_var->name<<")";
    else if(ftypedecl->f_name) *(outfile[curr])<<"("<<ftypedecl->f_name->value<<")";
     *(outfile[curr]) <<";"<<endl;

    if(global){
                       //add extern def to func.h
       curr = FUNC_H;
       indent(); *(outfile[curr])<<"extern ";
       *(outfile[curr])<<"FILE "<<identifier;
       if(ftypedecl->f_var){
           *(outfile[curr])<<"("<<ftypedecl->f_var->name<<")";
        }
        else if(ftypedecl->f_name){
           *(outfile[curr])<<"("<<ftypedecl->f_name->value<<")";
        }
        *(outfile[curr]) <<";"<<endl;
        curr= FUNC_C;
       //TODO: add it to global vars list as well.
        globalVars.insert(identifier);
     }
    //TODO: need to store the type of the var as well.
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
void CodeGen::visit(TupleType* tuple) {
    *(outfile[curr])<<"tuple<";
    //*(outfile[curr])<<"struct {}<"; //use struct instead of tuple
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
       //add extern def to func.h
      curr = FUNC_H;
      indent(); *(outfile[curr])<<"extern ";
      ptypedecl->type->accept(*this);
      *(outfile[curr])<<" "<<identifier<<";"<<endl;
      activeVars.push_back(identifier);
      globalVars.insert(identifier);
      curr= FUNC_C;
   }
   else if(within_action){
      actionVars.insert(identifier);
      activeVars.push_back(identifier);
   }
   else
       activeVars.push_back(identifier);
   if(ptypedecl->init != NULL){
       *(outfile[curr])<<" = ";
       ptypedecl->init->accept(*this);
   }
  // if(!pass_args)
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
          curr = FUNC_H;
          globalVars.insert(identifier);
          *(outfile[curr])<<"extern ";
          ((ArrayType*) ctypedecl->type)->accept(*this);
          *(outfile[curr])<< " "<<identifier;
          *(outfile[curr]) <<"["<<((ArrayType*) ctypedecl->type)->size<<"];"<<endl;
          curr = FUNC_C; 
      }
   }
   else{
      ctypedecl->type->accept(*this);
      *(outfile[curr])<< " "<<identifier;
      if(global){
          curr = FUNC_H;
          globalVars.insert(identifier);
          *(outfile[curr])<<"extern ";
          ctypedecl->type->accept(*this);
          *(outfile[curr])<< " "<<identifier<<";"<<endl;
          curr = FUNC_C; 
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
void CodeGen::visit(CommandBlock* cmdblock) {
    /*scope, command->conditions make the static part, should declarations go to dynamic or there could be static and dynamic part pf declaration? i.e. anything within do at{} belongs to dynamic. we could have statement outside that which apply to static. may have additional labels saying whether to have static or dynamic part*/
     curr = INSTR;
     cmdlevel++;
     dist = 0;
     if(cmdlevel == 0) c_count =0;
     cmd_argcount[cmdlevel] = 0;
     scopeVarName= cmdblock->component_type->name->name;
     scopeVarType= cmdblock->component_type->type;
     //start of module block
     comp_map[cmdlevel] = make_pair(scopeVarType, scopeVarName);

     activeCompVars.insert(scopeVarName);
     
     cmdblock->component_type->accept(*this);
     indentLevel[curr]++;
    //Step 3: check if any condition attached
    if(((ConstraintExpr* )cmdblock->cond)->cond != NULL){
        indent(); *(outfile[curr])<<"if( ";
        ((ConstraintExpr*)cmdblock->cond)->cond->accept(*this);
        *(outfile[curr])<<"){"<<endl;
         indentLevel[curr]++;
    }
     /*local declarations become part of all the actions in the current scope?*/ 
     if(cmdblock->localdeclarations){ 
         cmdblock->localdeclarations->accept(*this);
     }
    //Step 4: all the statements within a command
    if(cmdblock->stmts->statements.size()){
         cmdblock->stmts->accept(*this);
    }
    //Step 5: check all the enclosed commands 
      if(cmdblock->inner_blocks->nodes.size()){ //TODO: how to check if inner_block is empty.make it list??
          cout<<"size of inner blocks: "<<cmdblock->inner_blocks->nodes.size()<<endl;
          cmdblock->inner_blocks->accept(*this);
       }
    //Step 6: check actions for the current command
    if(cmdblock->actions->nodes.size()){
        cmdblock->actions->accept(*this);
         //curr = STAT; //set it back as actions must have changed it to func.cpp and func.h
    }
    if(((ConstraintExpr* )cmdblock->cond)->cond != NULL){ //close conditional brackets.
        indentLevel[curr]--;
        indent(); *(outfile[curr])<<"}"<<endl;
    }
    /*commandlists*/
    // cmdblock->commandlist->accept(*this);
         
     if(cmdblock->localdeclarations){
         popActiveVars();
     }
     
    /* indentLevel[curr]--;
     indent(); *(outfile[curr])<<"}"<<endl;
     */
     for(int i=0; i<nest_level[cmdlevel]; i++){
        indentLevel[curr]--;
        indent(); *(outfile[curr])<<"}"<<endl;
    }
    nest_level[cmdlevel] = 0;
    cmdlevel--;
}

void CodeGen::visit(Action* action) {
   /*everything inside this makes a dynamic action- can think about it later whether the actions can be labelled as static or dynamic*/
    
    /*each action is associated with current component type*/
     std::string compName = action->name->name;
    /*------------Step 1. Static part - only insert custom rule if static part condition not always TRUE------------*/
    //mode = INSTR;
    int trigger = t_location[action->trigger];
    std::string cname = comp_map[cmdlevel].second;
    int ctype =  comp_map[cmdlevel].first;
    //Add instArgs
    indent(); *(outfile[curr])<<DEFINE_ARGS_VECTOR<<actionID<<";"<<endl;
    if(((ConstraintExpr *)action->cond)->cond != NULL){
        indent(); *(outfile[curr])<<"if(";
        ((ConstraintExpr *)action->cond)->cond->accept(*this);
        *(outfile[curr])<<"){"<<endl;
        indentLevel[curr]++;
    }
    if(!activeCompVars.count(compName) || cname.compare(compName)){ //cannot do action on outer levels 
       cerr<<"FATAL ERROR: action cannot be performed on the given undefined component "<<compName<<endl;
      error_count++;
     exit(1);
     }



    /*------------Step 2. Dynamic part - only insert custom rule if static part condition not always TRUE------------*/
    //mode= LOAD;
    
    mode = DYNAMIC; // no need for it though
    /*----------------- save to temp file--------------------------*/
    curr = TEMP_C;
    outfile_t.open(inFile+".temp", ios::out);
    indentLevel[curr]++;
    within_action = true;
    if(action->dyn_decls->t_stmts.size() != 0){
        action->dyn_decls->accept(*this);
    }
    action->stmts->accept(*this);
    within_action = false;
    indentLevel[curr]--;
     
    outfile_t.close();
    /*---------------- close the temp file -----------------------------*/ 
    mode = STATIC;
    curr = INSTR;
    //TODO: Add instrumentaiton arguments
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
    else{
    }
    indent(); *(outfile[curr])<<"insertSnippetAt<"<<comp_type[cmdlevel]<<">(app,"<<compName<<", func_"<<actionID<<","<<trigger<<","<<pass_args<<", instArgs"<<actionID<<");"<<endl;
    if(((ConstraintExpr *)action->cond)->cond != NULL){
        indentLevel[curr]--;
        indent(); *(outfile[curr])<<"}"<<endl;
    }
    
    /*-- Step 2.1 : Go through statements in the action, store arguments to be added, place holder for function parameters*/
    int arg_count=0; 
    curr = FUNC_C; 
    indent(); *(outfile[curr]) <<"void func_"<<actionID<<"(";
    if(pass_args || dyn_sym_map.size()){
      for(auto it: dyn_sym_map){
        *(outfile[curr])<<it.second.ctype<< " var"<<arg_count;
        if(++arg_count < dyn_sym_map.size()){
            *(outfile[curr])<<",";
        }
      }
    }//TODO: add arguments from translated table if any
    *(outfile[curr])<<"){"<<endl;
    indentLevel[curr]++;
    //TODO: Copy contents from temp to func.c file
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
    indent(); *(outfile[curr]) <<"LIB_EXPORT void func_"<<actionID<<"(";
    arg_count = 0;
    if(pass_args || dyn_sym_map.size()){
      for(auto it: dyn_sym_map){
        *(outfile[curr])<<it.second.ctype<< " var"<<arg_count;
        if(++arg_count < dyn_sym_map.size()){
            *(outfile[curr])<<",";
        }
      }
    }
    //TODO: add arguments from translated table if any
    *(outfile[curr])<<");"<<endl;
    
    pass_args = false;

    if(action->dyn_decls->t_stmts.size() != 0)
         popActiveVars();

     dyn_sym_map.clear();
    //TODO: also clear mapping table
    curr = INSTR; 
    var_count = 0;
    actionID++;
}
 
void CodeGen::visit(ProgramBlock* prog) {
    /* global Decl go to dynamic part for now. Are two levels of nesting enough for this case? I.e. scope and component type? Shall we std::stringroduce other ways or levels to static part? */
    
    if(prog->globaldeclarations->t_stmts.size() != 0){
        global= true;
        curr = FUNC_C;
        prog->globaldeclarations->accept(*this); //start of dynamic file*/
        global= false;
    }
    if(prog->commandblocks->nodes.size())
        prog->commandblocks->accept(*this);
   
    if(prog->exit_stmts->statements.size()){
        curr= EXIT_C;
        indentLevel[curr]++;
        prog->exit_stmts->accept(*this);
        indentLevel[curr]--;
    }
    if(prog->init_stmts->statements.size()){
        curr= INIT_C;
        indentLevel[curr]++;
        prog->init_stmts->accept(*this);
        indentLevel[curr]--;
    }

    if(prog->globaldeclarations->t_stmts.size() != 0)
        popActiveVars();
    
    curr=LOAD;
    create_handler_table();
    
    if(error_count>0){
      cout<<error_count<<" errors reported: compilation unsuccessful"<<endl;
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
    cout<<"entered instr"<<endl;
    if(called_direct){
        if(cmdlevel==0){
            dist  = 3;
          //  upper_id = cid[MODULE];
            curr_name = comp_map[cmdlevel].second;
        }else{
            //if( cid[cmdlevel-1] == 4 or cid[cmdlevel-1] == 5) //based on numbering
            if( cid[comp_map[cmdlevel-1].first] == 3 or cid[comp_map[cmdlevel-1].first] == 4) //based on numbering
               dist = (cid[comp_map[cmdlevel-1].first] - cid[comp_map[cmdlevel].first]) - 1; 
            else
               dist = cid[comp_map[cmdlevel-1].first] - cid[comp_map[cmdlevel].first];
        //    upper_id = cid[cmdlevel-1];
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
    cout<<"entered bb"<<endl;
    if(called_direct){
        if(cmdlevel==0){
            dist  = MAX_NEST-1;
            upper_id = cid[MODULE];
            curr_name = comp_map[cmdlevel].second;
        }
        else{
            //if( cid[cmdlevel-1] == 4 || cid[cmdlevel-1] == 5) //based on numbering
            if( cid[comp_map[cmdlevel-1].first] == 3 || cid[comp_map[cmdlevel-1].first] == 4) //based on numbering
               dist = (cid[comp_map[cmdlevel-1].first] - cid[comp_map[cmdlevel].first]) - 1; 
            else
               dist = cid[comp_map[cmdlevel-1].first] - cid[comp_map[cmdlevel].first];
            //if(cid[cmdlevel-1] == cid[LOOP])
            if(comp_map[cmdlevel-1].first == LOOP)
                upper_id = cid[comp_map[cmdlevel-1].first];
            else
                upper_id = cid[FUNC];
        }
        curr_name = comp_map[cmdlevel].second; 
    }
    else{
       dist--; 
       //comp_map[cmdlevel-1].first == BASICBLOCK) {
       if(cmdlevel > 0 && comp_map[cmdlevel-1].first == BASICBLOCK) {
           curr_name = comp_map[cmdlevel-1].second; //no need to generate further code
           generate_code = false;
       }
       else{ 
           curr_name = "Comp"+to_string(c_count++);
       }
       upper_id = cid[FUNC];
       //upper_name = curr_name;
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
    cout<<"entered func"<<endl;
    if(called_direct){
        if(cmdlevel==0){
            dist  = 0;
            //upper_id = cid[MODULE];
            upper_name = "appImage";
            curr_name = comp_map[cmdlevel].second;
        }
        else{
           dist = cid[MODULE] - cid[FUNC]; 
           upper_name = comp_map[cmdlevel-1].second;
           //upper_name = cid[cmdlevel-1];
        }
        curr_name = comp_map[cmdlevel].second; 
    }
    else{
       dist--; 
       //if(cid[cmdlevel-1] == FUNC) {
       if(cmdlevel>0 && comp_map[cmdlevel-1].first == FUNC) {
           curr_name = comp_map[cmdlevel-1].second; //no need to generate further code
           generate_code = false;
       }
       else{ 
           curr_name = "Comp"+to_string(c_count++);
       }
       //upper_name = curr_name;
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
