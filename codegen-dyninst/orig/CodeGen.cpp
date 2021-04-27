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

typedef std::set<std::string> VarDefs;
VarDefs activeCompVars;
std::string scopeVarName;
int scopeVarType;
std::set<std::string> stdLib;
std::deque<std::string> activeVars;
std::stack<int> offset;

arg_data rule_data;

bool pass_args= false;
string comp_type;
string curr_key;
string type_key;

bool within_action=false;
bool type_expr = false;
int var_count = 0;
int deref_level=0;
map<string, attribute> symbol_table;
map<string, map_info> dyn_sym_map;
comp target_comp;
std::string inFile;
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
        
CodeGen::CodeGen(std::string filename){  
    outfile_ins.open(filename+"_instr.cpp", ios::out);
    outfile_load.open(filename+"_loader.cpp", ios::out);
    outfile_f.open(filename+"_func.cpp", ios::out);
    outfile_fh.open(filename+"_func.h", ios::out);
    outfile_e.open(filename+"_exit.cpp", ios::out);
    outfile_i.open(filename+"_init.cpp", ios::out);
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
    get_func[0]= get_static_func;//not needed 
    get_func[1]= get_dyn_func; //not needed
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
int CodeGen::generateTemplateCode(int type, std::string name){
     int currLevel=1;
     switch(type){
         case INST:
            if(scopeVarType == MODULE){
                indent(); *(outfile[INSTR])<<"for(auto &Comp"<<currLevel<<" : "<<scopeVarName<<".getProcedures()){"<<endl;
                currLevel++;
                indentLevel[curr]++;
                indent(); *(outfile[INSTR])<<"for(auto &Comp"<<currLevel<<" : Comp"<<currLevel-1<<".findPoint(BPatch_locInstruction)){"<<endl;
                currLevel++;
                indent(); *(outfile[INSTR])<<"Dyninst::InstructionAPI::Instruction::Ptr "<<name<<" = Comp"<<currLevel<<".getInsnAtPoint();"<<endl;
                indentLevel[curr]++;
            }
            else if(scopeVarType == FUNC){
                indent(); *(outfile[INSTR])<<"for(auto &Comp"<<currLevel<<" : "<<scopeVarName<<".findPoint(BPatch_locInstruction)){"<<endl;
                indent(); *(outfile[INSTR])<<"Dyninst::InstructionAPI::Instruction::Ptr "<<name<<" = Comp"<<currLevel<<".getInstructionAtPoint();"<<endl;
                //indent(); *(outfile[INSTR])<<"for( &Comp"<<currLevel<<" : "<<scopeVarName<<".getCFG->getAllBasicBlocks()){"<<endl;
                currLevel++;
                indentLevel[curr]++;
            }
            else{
                cout<<"invalid access "<<name<<" in specified scope"<<endl; 
                error_count++;
            }
         break;
         case FUNC:
         break;
         case LOOP:
            if(scopeVarType == MODULE){
                indent(); *(outfile[INSTR])<<"for(auto &Comp"<<currLevel<<" : "<<scopeVarName<<".getProcedures()){"<<endl;
                indentLevel[curr]++;
                indent(); *(outfile[INSTR])<<"std::vector< BPatch_basicBlockLoop * > AllLoops;"<<endl;
                indent(); *(outfile[INSTR])<< "Comp"<<currLevel<<"->getCFG()->getLoops(AllLoops);"<<endl;
                currLevel++;
            }
            else if(scopeVarType == FUNC){
            }
            else{
                cout<<"invalid access "<<name<<" in specified scope"<<endl; 
                error_count++;
            }
         break;
         case MODULE:
            *(outfile[INSTR])<<"";
         break;
         case SSANODE:
            *(outfile[INSTR])<<"AllVars";
        break;
        case BASICBLOCK:
            if(scopeVarType == MODULE){
                indent(); *(outfile[INSTR])<<"for(auto &Comp"<<currLevel<<" : "<<scopeVarName<<".getProcedures()){"<<endl;
                indentLevel[curr]++;
                indent(); *(outfile[INSTR])<<"std::set< BPatch_basicBlock * > AllBlocks;"<<endl;
                indent(); *(outfile[INSTR])<< "Comp"<<currLevel<<"->getCFG()->getAllBasicBlocks(AllBlocks);"<<endl;
                currLevel++;
            }
            else if(scopeVarType == FUNC){
            }
            else{
                cout<<"invalid access "<<name<<" in specified scope"<<endl; 
                error_count++;
            }
        break;
        default:
            *(outfile[INSTR])<<"UNKNOWN";
            cout<<"ERROR: Component Type not known! ";
            error_count++;
        break;
     }
     return currLevel;
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
   if(std::find(activeVars.begin(), activeVars.end(), id->name) != activeVars.end()){
       *(outfile[curr])<<id->name;
   }
   else{ 
      cout<<"ERROR: variable "<<id->name<< " not defined"<<endl;
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
   if(std::find(activeVars.begin(), activeVars.end(), ident) != activeVars.end()){
       *(outfile[curr])<<ident;
   }
   else{
      cout<<"ERROR: variable "<<ilhs->name->name<< " not defined"<<endl;
      error_count++;
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
            else
                if(get_func[mode].count(dlhs->field->name) > 0){
                    curr_key += get_func[mode][dlhs->field->name] + base_name + CLOSEPARAN; //need to check if static or dyn
                    while(deref_level>0){
                      curr_key += CLOSEPARAN;
                      deref_level--;
                    }
                    if(within_action){
                          if(!type_expr){
                              string search_key = "vars["+ to_string(var_count++)+"]";
                              dyn_sym_map[search_key].var = curr_key;
                              dyn_sym_map[search_key].type = 64;
                              *(outfile[curr])<<search_key;
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
                    cout<<"ERROR: "<<dlhs->field->name<<" is not a valid field"<<endl;
                    error_count++;
                }
        }
        else{          //This is a user defined, so we need to check in local or global vars
                
        }
   }

   else{
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
    *(outfile[curr])<<fc->name->name<<OPENPARAN;                //check whether function name exists?
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
   if(types.count(typeexpr->type_name->name) > 0){
        //TODO: if within_action, then replace is_type expression with a bool, put that in argument of dynamic, replace with var name
        string type_name = types[typeexpr->type_name->name];
        if(within_action){
            type_expr = true;
            typeexpr->lhs->accept(*this);
            type_expr = false;
            string new_key = "is_type(" + type_key + "," + type_name + CLOSEPARAN;
            string search_key = "vars["+ to_string(var_count++) + "]";
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
}

void CodeGen::visit(ifElseStmt* ifelsestmt) {
    indent(); *(outfile[curr])<<" if( ";     //shall we do this to a header?
    ifelsestmt->condition->accept(*this);
    *(outfile[curr])<<" )"<<endl;
    indent(); *(outfile[curr])<<"{"<<endl;
    
    indentLevel[curr]++;
    ifelsestmt->ifBody->accept(*this);
    indentLevel[curr]--;
    
    indent(); *(outfile[curr])<<"}"<<endl;
    indent(); *(outfile[curr])<<"else{"<<endl;
    
    indentLevel[curr]++;
    ifelsestmt->elseBody->accept(*this);
    indentLevel[curr]--;
    
    indent(); *(outfile[curr])<<"}"<<endl;
}
void CodeGen::visit(ifElseIfStmt* ifelseifstmt) {
 /* selection statements are part of action so go to dynamic handler*/
    /*could use a label to decide whether to spit to static or dynamic. default- Dynamic, part of rule*/
    indent(); *(outfile[curr])<<" if( ";     //shall we do this to a header?
    ifelseifstmt->ifcondition->accept(*this);
    *(outfile[curr])<<" )"<<endl;
    indent(); *(outfile[curr])<<"{"<<endl;
    
    indentLevel[curr]++;
    ifelseifstmt->ifBody->accept(*this);
    indentLevel[curr]--;
    
    indent(); *(outfile[curr])<<"}"<<endl;
    indent(); *(outfile[curr])<<"else if(";
    ifelseifstmt->elsecondition->accept(*this);
    *(outfile[curr])<<" )"<<endl;
    indent(); *(outfile[curr])<<"{"<<endl;
    
    indentLevel[curr]++;
    ifelseifstmt->elseBody->accept(*this);
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

void CodeGen::visit(TypeDeclList* typedecllst) {
    int count=0;
    for(auto tdecl: typedecllst->typedecls){
        tdecl->accept(*this);
        count++;
    }
    offset.push(count);
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
       case DOUBLE:
            type = "double";
            break;
       case CHAR:
            type = "char";
            break;
       default:
            type = "UNKNOWN";
            cout<<"ERROR: variable type not known! ";
            error_count++;
            break;
    }
    *(outfile[curr])<<type;
    if(pass_args)       
        rule_data.type = type;
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
   /*PrimitiveType* keytype;   //add such to record values, if a variable is prim type, it is not recorded type. 
   VType* valuetype;*/
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

void CodeGen::visit(PrimitiveTypeDecl* ptypedecl) {

   std::string type, identifier, init;
   //if global variable, extern type identifier in func.h and then init in func.cpp
  
   indent();
   ptypedecl->type->accept(*this); 
   identifier= ptypedecl->name->name;
   if(pass_args) rule_data.id = identifier;
   *(outfile[curr]) <<" "<<identifier;
   
   if(global){
       //add extern def to func.h
       curr = FUNC_H;
       indent(); *(outfile[curr])<<"extern ";
       ptypedecl->type->accept(*this);
       *(outfile[curr])<<" "<<identifier<<";"<<endl;
       curr= FUNC_C;
   }
   activeVars.push_back(identifier);
   if(ptypedecl->init != NULL){
       *(outfile[curr])<<" = ";
       ptypedecl->init->accept(*this);
   }
   if(!pass_args)
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
    
void CodeGen::visit(Command* cmd) {
    // Type and condition are static, action is dynamic
    int currLevel;
    target_comp.name = cmd->component_type->name->name;
    target_comp.type = cmd->component_type->type;
    indent(); //put outfile[curr] to outfile_s
    //currLevelName = scopeVarName; 
    currLevel = generateTemplateCode(target_comp.type, target_comp.name);
    if(target_comp.type != INST){
        if(currLevel>1){
            *(outfile[curr])<<"for(auto &"<<target_comp.name<<": Comp"<<currLevel-1<<".";
        }
        else{
            *(outfile[curr])<<"for(auto &"<<target_comp.name<<": "<<scopeVarName<<".";
        }
    }
    //*(outfile[curr])<<"for(auto &"<<target_comp.name<<": "<<scopeVarName<<".";
    
    activeCompVars.insert(target_comp.name);
    cmd->component_type->accept(*this);
    if(target_comp.type == INST)
        *(outfile[curr])<<"{"<<endl;
    else
        *(outfile[curr])<<"){"<<endl;

    indentLevel[curr]++;//for loop indentation
    
    indent(); *(outfile[curr])<< "if("; 
   
    //mode = INSTR;
    mode = 0; //no need for this though
    //TODO: check if condition is always true
    if(isInstanceOf<Expression, BoolConst>(cmd->conditions)){
      int b_val = ((BoolConst*)cmd->conditions)->b_val;
      if(b_val == (int)B_TRUE){
        //TODO: if condition always true and no further actions in static part, make it purely dynamic without inserting rules? 
      }
    }
    cmd->conditions->accept(*this);
    *(outfile[curr])<<"){"<<endl; 
    indentLevel[curr]++;
    
    cmd->actions->accept(*this);

    indentLevel[curr]--;
    indent(); *(outfile[curr])<<"}"<<endl;//end of if condtion
    while(currLevel>1){
       indentLevel[curr]--;
       indent(); *(outfile[curr])<<"}"<<endl;//end of extra for loops
       currLevel--;
    }
    indentLevel[curr]--;
    indent(); *(outfile[curr])<<"}"<<endl;//end of for loop
    
    /* do static part of action here- i.e. inserting rewrite-rulei, also need to know trigger*/
    //generate_action_rule(trigger,c_ident, i_ident, position);
}

void CodeGen::visit(CommandBlock* cmdblock) {
    /*scope, command->conditions make the static part, should declarations go to dynamic or there could be static and dynamic part pf declaration? i.e. anything within do at{} belongs to dynamic. we could have statement outside that which apply to static. may have additional labels saying whether to have static or dynamic part*/
     curr = INSTR;
     scopeVarName= cmdblock->component_type->name->name;
     scopeVarType= cmdblock->component_type->type;
     //start of module block
     
     activeCompVars.insert(scopeVarName);
     //TODO: emit different code for each type of scope
     if(scopeVarType == FUNC){
     
         *(outfile[curr])<<"for(auto &"<<scopeVarName<<": appImage->getProcedures()";
     }
     else{
         *(outfile[curr])<<"for(auto &"<<scopeVarName<<": appImage->getModules()";
     }
#if 0 //experiment- do I need it?
     cmdblock->component_type->accept(*this);
#endif
     *(outfile[curr])<<"){"<<endl;
    
     indentLevel[curr]++;
     /*local declarations become part of all the actions in the current scope?*/ 
     if(cmdblock->localdeclarations){ 
         cmdblock->localdeclarations->accept(*this);
     }
     
     /*commandlists*/
     cmdblock->commandlist->accept(*this);
         
     if(cmdblock->localdeclarations){
         popActiveVars();
     }
     
     indentLevel[curr]--;
     indent(); *(outfile[curr])<<"}"<<endl;
     
}

void CodeGen::visit(Action* action) {
   /*everything inside this makes a dynamic action- can think about it later whether the actions can be labelled as static or dynamic*/
    
    /*each action is associated with current component type*/
     std::string compName;
    /*------------Step 1. Static part - only insert custom rule if static part condition not always TRUE------------*/
    //mode = INSTR;
    mode = 0; // no need for it though
    if(target_comp.type == INST){
        compName ="Comp"+currLevel;
    }
    else{
        compName = target_comp.name;
    
    }
    int trigger = t_location[action->trigger];
    if(((RuleExpr*) action->metadata)->arg != NULL){
         pass_args = true;
         action->metadata->accept(*this); 
    }
    if(pass_args){
        indent(); *(outfile[curr])<<"insertSnippetAt<"<<comp_type<<">(app,"<<compName<<", func_"<<actionID<<","<<trigger<<", true, "<<rule_data.id<<");"<<endl;
    }else{
        indent(); *(outfile[curr])<<"insertSnippetAt<"<<comp_type<<">(app,"<<compName<<", func_"<<actionID<<","<<trigger<<", false, 0);"<<endl;
    }



    /*------------Step 2. Dynamic part - only insert custom rule if static part condition not always TRUE------------*/
    //mode= LOAD;
    mode= 0; //no need for it though
    
    /*----------------- save to temp file--------------------------*/
    curr = TEMP_C;
    outfile_t.open(inFile+"_temp.cpp", ios::out);
    indentLevel[curr]++;
    if(action->dyn_decls->typedecls.size() != 0){
        action->dyn_decls->accept(*this);
    }
    within_action = true;
    action->stmts->accept(*this);
    within_action = false;
    indentLevel[curr]--;
     
    outfile_t.close();
    /*---------------- close the temp file -----------------------------*/ 
    
    /*-- Step 2.1 : Go through statements in the action, store arguments to be added, place holder for function parameters*/
    int arg_count=0; 
    curr = FUNC_C; 
    //indent(); *(outfile[curr]) <<"void func_"<<actionID<<"("<<FUNC_ARGS<<target_comp.name;
    indent(); *(outfile[curr]) <<"void func_"<<actionID<<"(";
    if(pass_args)
        *(outfile[curr])<<rule_data.type<<" "<<rule_data.id;
    //TODO: add arguments from translated table if any
    *(outfile[curr])<<"){"<<endl;
    indentLevel[curr]++;
    //TODO: Copy contents from temp to func.c file
    outfile_t.open(inFile+"_temp.cpp", ios::in);
    string line;
    while(getline(outfile_t, line)){
        *(outfile[curr])<<line<<endl;
    }
    outfile_t.close();
    indentLevel[curr]--;
    indent(); *(outfile[curr])<<"}"<<endl;
    /*function prototype*/
    curr = FUNC_H;
    //indent(); *(outfile[curr]) <<"void func_"<<actionID<<"("<<FUNC_ARGS<<target_comp.name;
    indent(); *(outfile[curr]) <<"LIB_EXPORT void func_"<<actionID<<"(";
    if(pass_args){
        *(outfile[curr])<<rule_data.type; //don't necessarily need the name
        //*(outfile[curr])<<rule_data.type<<" "<<rule_data.id;
    }
    //TODO: add arguments from translated table if any
    *(outfile[curr])<<");"<<endl;
#if 0
    curr = LOAD;
    indent(); *(outfile[curr])<<"void handler_"<<actionID<<"(JANUS_CONTEXT){"<<endl;
    indentLevel[curr]++;
    //TODO: if target_comp is  not instr, then use a different name
    indent(); *(outfile[curr])<<"instr_t *"<<target_comp.name<<" = " GET_TRIGGER_INSTR<<endl;
    /*TODOif(target_comp.type == insturction){
     indent(); outfile_d<<target_comp.name<<" = " GET_TRIGGER_INSTR<<endl;
    }
    else{
     indent(); outfile_d<<target_comp.name<<" = " GET_TRIGGER_INSTR<<endl;
    }*/
    /*TODO: Now first go through current internal data structurs, see if they need to be added
     
    /*insert clean call after all the statements have been travered*/
    
    int num =0;
    if(dyn_sym_map.size()>0){
        indent(); *(outfile[curr])<<RESET_VAR_COUNT<<endl;
        for(auto it=dyn_sym_map.begin(); it!=dyn_sym_map.end(); it++){
            indent(); *(outfile[curr])<<it->second.var<<SEMICOLON<<endl;
        }
    }
    indent(); outfile_d<<INSERT_CLEAN_CALL_PREFIX<<actionID<<", false";
    if(pass_args){
        arg_count++;
        *(outfile[curr])<<","<<arg_count<<","<<INSERT_RULE_DATA<<");"<<endl;
    }
    else{
        *(outfile[curr])<<","<<arg_count<<");"<<endl;
    }
    //TODO: add arguments from translated table if any
    indentLevel[curr]--;
    indent(); *(outfile[curr])<<"}"<<endl;
#endif 
    
    pass_args = false;

    if(action->dyn_decls->typedecls.size() != 0)
         popActiveVars();

     dyn_sym_map.clear();
    //TODO: also clear mapping table
    curr = INSTR; 
    var_count = 0;
    actionID++;
}
 
void CodeGen::visit(ProgramBlock* prog) {
    /* global Decl go to dynamic part for now. Are two levels of nesting enough for this case? I.e. scope and component type? Shall we std::stringroduce other ways or levels to static part? */
    
    if(prog->globaldeclarations->typedecls.size() != 0){
        global= true;
        curr = FUNC_C;
        prog->globaldeclarations->accept(*this); //start of dynamic file*/
        global= false;
    }
    prog->commandblocks->accept(*this);
   
    if(prog->exit_stmts->statements.size() != 0){
        curr= EXIT_C;
        indentLevel[curr]++;
        prog->exit_stmts->accept(*this);
        indentLevel[curr]--;
    }

    if(prog->globaldeclarations->typedecls.size() != 0)
        popActiveVars();
    
    curr=LOAD;
    create_handler_table();
    
    if(error_count>0){
      cout<<error_count<<" errors reported: compilation unsuccessful"<<endl;
      exit(1);
    }
   
}

void CodeGen::visit(Component* comp) {
    /* This just makes a for loop tempelate to go over all the instructions in the program*/ 
    /*currently janus context has only fucntions and loops listed - other components can be accessed through these which means they cannot be in the scope which is the outermost construct*/
     //TODO: see where we coming from. can we get all basic blocks, from modules.

     //Hierarchy: appImage->Modules->getProcedures()->getCFG()->getAllBasicBlocks()
     //Hierarchy: appImage->Modules->getProcedures()->getCFG()->getLoops()
     switch(comp->type){
         case INST:
            //*(outfile[INSTR])<< "getInstructions()";
            comp_type = "BPatch_point";
         break;
         case FUNC:
            *(outfile[INSTR])<<"getProcedures()";
            comp_type = "BPatch_function";
         break;
         case LOOP:
            if(scopeVarType == MODULE){
            
            }
            else if (scopeVarType == FUNC){
            
            }
            *(outfile[INSTR])<<"AllLoops"; //TODO: change in the for loop as well (for auto &L : AllLoops)
            //*(outfile[INSTR])<<"getCFG()->getLoops()";
            comp_type = "BPatch_basicBlockLoop";
         break;
         case MODULE:
            *(outfile[INSTR])<<"";
            comp_type = "BPatch_module";
         break;
         case SSANODE:
            *(outfile[INSTR])<<"AllVars";
            comp_type = "Variable";
        break;
        case BASICBLOCK:
            *(outfile[INSTR])<<"AllBlocks"; //TODO: change in the for loop as well (for auto &L : AllBlocks)
            //*(outfile[INSTR])<<"getCFG()->getAllBasicBlocks";
            comp_type = "BPatch_basicBlock";
        break;
        default:
            *(outfile[INSTR])<<"UNKNOWN";
            cout<<"ERROR: Component Type not known! ";
            error_count++;
        break;
     }
}
#if 0
void CodeGen::visit(Component* comp) {
    /* This just makes a for loop tempelate to go over all the instructions in the program*/ 
    /*currently janus context has only fucntions and loops listed - other components can be accessed through these which means they cannot be in the scope which is the outermost construct*/
     //TODO: see where we coming from. can we get all basic blocks, from modules.

     //Hierarchy: appImage->Modules->getProcedures()->getCFG()->getAllBasicBlocks()
     //Hierarchy: appImage->Modules->getProcedures()->getCFG()->getLoops()
     switch(comp->type){
         case INST:
            *(outfile[INSTR])<< ".instrs";
            comp_type = "BPatch_point";
         break;
         case FUNC:
            *(outfile[INSTR])<<".getProcedures()";
            //*(outfile[INSTR])<<".functions";
            comp_type = "BPatch_function";
         break;
         case LOOP:
            *(outfile[INSTR])<<".loops";
            comp_type = "BPatch_basicBlockLoop";
         break;
         case MODULE:
            *(outfile[INSTR])<<"";
            comp_type = "BPatch_module";
         break;
         case SSANODE:
            *(outfile[INSTR])<<"AllVars";
            comp_type = "Variable";
        break;
        case BASICBLOCK:
            *(outfile[INSTR])<<".blocks";
            comp_type = "BPatch_basicBlock";
        break;
        default:
            *(outfile[INSTR])<<"UNKNOWN";
            cout<<"ERROR: Component Type not known! ";
            error_count++;
        break;
     }
}
#endif
