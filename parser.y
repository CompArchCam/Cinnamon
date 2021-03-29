%{
#include <stdio.h>
#include <iostream>
#include "lexer.h"
#include "AST.h"
extern int yylex(void);
void yyerror(ProgramBlock*& program, const char *s) {printf("ERROR: %s\n", s);}
%}

%locations
%parse-param {ProgramBlock*& program}
%union{
    char* str;
    int number;
    int token;

    Node*               node;
    ProgramBlock*       prog;
    Statement*          stmt;
    TypeDeclStmt*         typedeclstmt;
    SelectionStmt*      selectstmt;
    Identifier*         ident;
    Component*          component;
    Expression*         expr;
    TypeDecl*           typedecl;
    VType*              typespec;
    LHS*                lhs;
    CompositeType*      comptype;
    PrimitiveType*       primtype;

    ExpressionList*     exprlist;
    StatementList*      stmtlist;
    SelectStatementList*  selectstmtlist;
    TypeDeclStmtList*   typedeclstmtlist;
    VTypeList*          typespeclist;
    NodeList*        nodelist;

}
%token IDENTIFIER STR_CONST NUM

%token <token> REG_64 REG_64_ID REG_32 REG_32_ID REG_16 REG_16_ID MEM CONST REG_64_SP REG_64_FP REG_64_PC REG_64_RAX REG_64_RBX

%token <token> GT GE LT LE EQ NEQ
%token <token> PLUS MINUS MULT DIV L_AND L_OR AND OR XOR NOT INIT_OP
%token <token> CONJUN DISJUN L_BRACE R_BRACE L_PARAN R_PARAN L_SQR R_SQR COMMA SEMICOLON COLON DOT
%token <token> OTHER
%token <token> IF ELSE WHILE FORLOOP RETURN GLOBAL LOCAL DO DEF ISTYPE TYPEOF USAGE INIT
%token <token> T_AT T_BEFORE T_AFTER T_ENTRY T_EXIT T_ITER WHERE_C
%token <token> B_TRUE B_FALSE NULL_PTR
%token <token> SCOPE RULE
%token <token> TUPLE DICT VECT SET PAIR INT VAR CHAR DOUBLE BOOL UINT32 UINT64 ADDR_INT FILE_IO FILE_LINE
%token <token> FUNC INST LOOP BASICBLOCK SSANODE MODULE
%token <token> OP_CALL OP_MOV OP_ADD OP_SUB OP_MUL OP_DIV OP_NOP OP_RETURN OP_BRANCH OP_CMP OP_LOAD OP_STORE OP_GETPTR 
%left PLUS MINUS
%left MULT DIV

%type <str> IDENTIFIER
%type <number> NUM
%type <str> STR_CONST
%type <prog> prog
%type <node> action command_block
%type <nodelist> actions command_blocks
%type <stmtlist> stmts end_block init_block
%type <typedeclstmtlist> type_decl_stmts
%type <selectstmtlist> else_if_stmts
%type <stmt> stmt select_stmt expr_stmt loop_stmt 
%type <typedeclstmt> type_decl_stmt
%type <selectstmt> else_if_stmt
%type <ident> ident

%type <expr> expr cond assgnExpr callExpr type_pred_expr 
%type <expr> arithlogicExpr logical_or_expr logical_and_expr
%type <expr> equality_expr relational_expr 
%type <expr> arith_expr prod_expr factor and_expr or_expr xor_expr
%type <expr> reg_id_expr reg_val_expr
%type <expr> literal
%type <lhs> lhs
%type <component> component_decl
%type <exprlist> call_args
%type <token> lop uop eop rop aop mop
%type <token> trigger c_type
%type <token> bool_const
%type <token> opcode register reg_id_type reg_type

%type <typedecl> type_decl_expr primitive_type_decl composite_type_decl file_type_decl
%type <typespec> v_type
%type <primtype> primitive_type
%type <comptype> composite_type tuple array dict pair set vect
%type <exprlist> composite_init
%type <typespeclist> type_list
%start prog
%%


prog:
  type_decl_stmts init_block command_blocks end_block{$$ = new ProgramBlock($3, $1, $2, $4); program = $$;} | type_decl_stmts command_blocks init_block end_block{$$ = new ProgramBlock($2, $1, $3, $4); program = $$;}
;
end_block: 
        {$$= new StatementList(); }| T_EXIT L_BRACE stmts R_BRACE {$$ = new StatementList(); $$->statements.push_back($3); } /*need to make sure it only accesses global vars- this will be ensured by looking up identifiers in the global and local lists. local lists will be empty by now. Only global ones exist*/ 
;
init_block: 
        {$$= new StatementList(); }| INIT L_BRACE stmts R_BRACE {$$ = new StatementList(); $$->statements.push_back($3); } /*need to make sure it only accesses global vars- this will be ensured by looking up identifiers in the global and local lists. local lists will be empty by now. Only global ones exist*/ 
;
command_blocks:
 {$$ = new NodeList();} | command_block {$$ = new NodeList(); $$->nodes.push_back($1);} | command_blocks command_block{$1->nodes.push_back($2);}
;

command_block:  
   component_decl cond L_BRACE type_decl_stmts stmts actions command_blocks R_BRACE {$$ = new CommandBlock($1,$2 ,$4, $5, $6, $7);} | component_decl cond L_BRACE type_decl_stmts stmts command_blocks actions R_BRACE {$$ = new CommandBlock($1,$2 ,$4, $5, $7, $6);}
;
cond: 
   {$$ = new ConstraintExpr();} | WHERE_C L_PARAN expr R_PARAN{$$ = new ConstraintExpr($3);}
;

type_decl_stmts:
        {$$ = new TypeDeclStmtList();} | type_decl_stmt {$$= new TypeDeclStmtList(); $$->t_stmts.push_back($1);} | type_decl_stmts type_decl_stmt { $1->t_stmts.push_back($2);}
;
type_decl_stmt:
        type_decl_expr SEMICOLON {$$= new TypeDeclStmt($1);}
;

type_decl_expr: primitive_type_decl {$$=$1;} | composite_type_decl {$$=$1;} | file_type_decl {$$ = $1;}
;

primitive_type_decl: primitive_type ident {$$ = new PrimitiveTypeDecl($1, $2);} | primitive_type ident INIT_OP expr {$$= new PrimitiveTypeDecl($1, $2, $4);}
;

file_type_decl: FILE_IO ident L_PARAN STR_CONST R_PARAN {$$ = new FileTypeDecl($2, new StrConst($4));} | FILE_IO ident {$$ = new FileTypeDecl($2);}

primitive_type:
        INT {$$ = new PrimitiveType(INT);}| ADDR_INT {$$ = new PrimitiveType(ADDR_INT);} | UINT32 {$$ = new PrimitiveType(UINT32);} | UINT64 {$$ = new PrimitiveType(UINT64);} |DOUBLE {$$= new PrimitiveType(DOUBLE);} | BOOL {$$ = new PrimitiveType(BOOL);} | CHAR {$$= new PrimitiveType(CHAR);} | VAR {$$= new PrimitiveType(VAR);} | FILE_LINE{$$= new PrimitiveType(FILE_LINE);};
composite_type_decl: 
        composite_type ident {$$ = new CompositeTypeDecl($1, $2);} | composite_type ident INIT_OP L_BRACE composite_init R_BRACE {$$= new CompositeTypeDecl($1, $2, $5);}
;
composite_type:
        tuple {$$=$1;}| array {$$=$1;} | dict{ $$=$1;} | pair {$$=$1;} | vect {$$=$1;} | set {$$=$1;}
;
tuple:
        TUPLE LT type_list GT {$$= new TupleType($3);}
;
composite_init: expr {$$= new ExpressionList(); $$->expressions.push_back($1); }|  composite_init COMMA expr {$1->expressions.push_back($3);}
;
type_list: 
        primitive_type {$$= new VTypeList(); $$->typelist.push_back($1);} | type_list COMMA primitive_type{$$->typelist.push_back($3);}
;
pair:
        PAIR LT primitive_type COMMA primitive_type GT {$$ = new PairType($3, $5);}
;
dict:
        DICT LT primitive_type COMMA v_type GT {$$ = new DictType($3, $5);}
;
vect:
        VECT LT primitive_type GT {$$ = new VectType($3);}
;
set:
        SET LT primitive_type GT {$$ = new SetType($3);}
;
v_type:
        primitive_type {$$=$1;}| composite_type{$$=$1;}
;
array:
        v_type L_SQR NUM R_SQR {$$ = new ArrayType($1, $3);}
;
actions:      
        {$$ = new NodeList();} | action {$$ = new NodeList(); $$->nodes.push_back($1);} | actions action {$1->nodes.push_back($2);}
;
action:
        trigger ident cond L_BRACE type_decl_stmts stmts R_BRACE {$$ = new Action($1, $2, $3, $5, $6);}
;
stmts:
         {$$= new StatementList(); }|stmt {$$ = new StatementList(); $$->statements.push_back($1);} | stmts stmt {$1->statements.push_back($2);}
;

stmt:           
        select_stmt {$$ = $1;} | loop_stmt {$$ = $1;} | expr_stmt { $$ = $1;} | type_decl_stmt {$$=$1;}
;
loop_stmt:
        FORLOOP L_PARAN primitive_type_decl SEMICOLON expr SEMICOLON expr R_PARAN L_BRACE stmts R_BRACE {$$ = new ForStmt($3, $5, $7, $10);} | WHILE L_PARAN expr R_PARAN L_BRACE stmts R_BRACE {$$ = new WhileStmt($3, $6);}
;

expr_stmt:        
        { $$ = new ExprStatement();}| expr SEMICOLON {$$= new ExprStatement($1);}
;
select_stmt:
     IF L_PARAN expr R_PARAN L_BRACE stmts R_BRACE else_if_stmts ELSE L_BRACE stmts R_BRACE {$$ = new ifStmt($3, $6, $8, $11);} | IF L_PARAN expr R_PARAN L_BRACE stmts R_BRACE else_if_stmts {$$ = new ifStmt($3, $6, $8, NULL);}| IF L_PARAN expr R_PARAN L_BRACE stmts R_BRACE ELSE L_BRACE stmts R_BRACE {$$ = new ifStmt($3, $6, NULL, $10);}| IF L_PARAN expr R_PARAN L_BRACE stmts R_BRACE {$$ = new ifStmt($3, $6, NULL, NULL);}
;
else_if_stmts:
    else_if_stmt  {$$ = new SelectStatementList(); $$->stmts.push_back($1);} |  else_if_stmts else_if_stmt {$1->stmts.push_back($2);} 
;
else_if_stmt:
    ELSE IF L_PARAN expr R_PARAN L_BRACE stmts R_BRACE {$$ = new elseIfStmt($4, $7);}
;
expr:               
        lhs{$$=$1;} | literal{$$=$1;} | assgnExpr{$$=$1;} | callExpr{$$=$1;} | arithlogicExpr {$$=$1;} | type_pred_expr{$$=$1;} | reg_val_expr {$$=$1;} | reg_id_expr{$$=$1;} 
;
reg_val_expr: 
      L_PARAN reg_type R_PARAN register { $$ = new RegValExpr($2, $4);} |  L_PARAN reg_type R_PARAN lhs { $$ = new RegValExpr($2, $4);}
;
reg_id_expr: 
      L_PARAN reg_id_type R_PARAN register { $$ = new RegIDExpr($2, $4);} |  L_PARAN reg_id_type R_PARAN lhs { $$ = new RegIDExpr($2, $4);}
;
reg_type:
     REG_64{$$= REG_64;} | REG_16{$$= REG_16;} | REG_32{$$= REG_32;}
;
reg_id_type:
     REG_64_ID{$$= REG_64_ID;} | REG_16_ID{$$ = REG_16_ID;} | REG_32_ID {$$= REG_32_ID;}
;
register:
     REG_64_FP{$$ = REG_64_FP;} | REG_64_SP {$$ = REG_64_SP;} | REG_64_PC {$$ = REG_64_PC;} | REG_64_RAX {$$ = REG_64_RAX;} | REG_64_RBX {$$ = REG_64_RBX;}
;
     

arithlogicExpr:      
        logical_or_expr {$$= $1;}
;
type_pred_expr:
        L_PARAN expr R_PARAN ISTYPE reg_type {$$= new TypePredExpr($2, "reg");} | expr ISTYPE reg_type {$$=new TypePredExpr($1, "reg");} | L_PARAN expr R_PARAN ISTYPE MEM {$$= new TypePredExpr($2, "mem");} |  expr ISTYPE MEM {$$=new TypePredExpr($1, "mem");} |  L_PARAN expr R_PARAN ISTYPE CONST {$$= new TypePredExpr($2, "Const");} | expr ISTYPE CONST {$$= new TypePredExpr($1, "Const");} 
;
logical_or_expr: 
        logical_and_expr {$$= $1;} | logical_or_expr L_OR logical_and_expr {$$ = new BinaryCondExpr($1, $3,L_OR);}
;

logical_and_expr:     
        or_expr{$$= $1;} | logical_and_expr L_AND or_expr {$$ = new BinaryCondExpr($1, $3, L_AND);}
;
or_expr:
        xor_expr {$$= $1;} | or_expr OR xor_expr {$$ = new BinaryLogicExpr($1, $3, OR);}
;
xor_expr:
        and_expr {$$= $1;} | xor_expr XOR and_expr {$$ = new BinaryLogicExpr($1, $3, XOR);}
;
and_expr:       
        equality_expr {$$= $1;} | and_expr AND equality_expr {$$ = new BinaryLogicExpr($1, $3,AND);}
;
equality_expr: 
        relational_expr {$$= $1;} | equality_expr eop relational_expr {$$= new BinaryCompExpr($1, $3, $2);} 
;

relational_expr: 
        arith_expr {$$= $1;} | relational_expr rop arith_expr {$$ = new BinaryCompExpr($1, $3, $2);}
;

arith_expr:
        prod_expr {$$=$1;} | prod_expr aop arith_expr {$$ = new BinaryArithExpr($1, $3, $2);}
;

prod_expr:
        factor {$$= $1;} | factor mop prod_expr {$$= new BinaryArithExpr($1, $3, $2);}
;

factor: 
        L_PARAN expr R_PARAN {$$= $2;} | ident {$$= $1;}  | literal {$$=$1;} | uop factor {$$ = new UnaryExpression($1, $2);}
;

component_decl:
        c_type ident{$$=new Component($1, $2);}
;
c_type: MODULE{$$=MODULE;} | INST{$$=INST;} | FUNC{$$=FUNC;} | LOOP{$$=LOOP;} | SSANODE{$$=SSANODE;} | BASICBLOCK{$$=BASICBLOCK;}
;
ident:  
        IDENTIFIER{$$ = new Identifier($1);}
;

assgnExpr: 
        lhs INIT_OP expr {$$ = new AssnExpression($1, $3);} 
;

lhs: 
        ident {$$ = new IdentLHS($1);} | lhs L_SQR expr R_SQR {$$ = new IndexLHS($1, $3);} | lhs DOT ident {$$= new DerefLHS($1, $3);}
;

callExpr: 
        lhs L_PARAN call_args R_PARAN {$$ = new FunctionCall($1, $3);}
;

call_args: 
        {$$ = new ExpressionList();}| expr {$$ = new ExpressionList(); $$->expressions.push_back($1);}| call_args COMMA expr {$1->expressions.push_back($3);}
;

literal:        
        STR_CONST {$$= new StrConst($1);} | NULL_PTR {$$ = new NullPtr();} | NUM {$$ = new NumConst($1);}| bool_const {$$ = new BoolConst($1);} | opcode {$$ = new OpCode($1);}
;
bool_const:
        B_TRUE{$$=B_TRUE;} | B_FALSE {$$= B_FALSE;}
;
opcode:
        OP_CALL{$$=OP_CALL;} | OP_MOV{$$=OP_MOV;} | OP_ADD{$$=OP_ADD;} | OP_SUB{$$=OP_SUB;} | OP_MUL{$$=OP_MUL;} | OP_DIV{$$=OP_DIV;} | OP_NOP{$$=OP_NOP;} | OP_LOAD{$$=OP_LOAD;} | OP_STORE{$$=OP_STORE;} | OP_RETURN{$$=OP_RETURN;} | OP_GETPTR{$$=OP_GETPTR;} | OP_CMP{$$=OP_CMP;} | OP_BRANCH{$$=OP_BRANCH;}
;
trigger:  
        T_AT{$$=T_AT;} | T_BEFORE{$$= T_BEFORE;} | T_AFTER{$$= T_AFTER;} |T_ENTRY{$$=T_ENTRY;} | T_EXIT{$$=T_EXIT;} | T_ITER{$$=T_ITER;}
;
uop:     
        NOT{$$=NOT;} | MINUS{$$=MINUS;}
;
rop:    
        LT {$$= LT;} | LE{$$=LE;} | GT{$$=GT;} | GE {$$=GE;}
;
eop:    
        EQ {$$=EQ;} | NEQ{$$=NEQ;}
;
lop:    
        L_AND{$$=L_AND;} | L_OR{$$=L_OR;}
;
aop:    
        PLUS {$$ = PLUS;} | MINUS {$$= MINUS;} 
;
mop:    
        DIV{$$=DIV;} | MULT {$$=MULT;}
;

%%
