%{
#include <stdio.h>
#include <string.h>
#include "AST.h"
#include "parser.hpp"
#define SAVE_TOKEN yylval.str = strdup(yytext);
void showError();
%}

%option noyywrap
%option never-interactive 

numbers     ([0-9])+
identifier  ([a-zA-Z_][a-zA-Z_0-9]*)
string_const    (\"(\\.|[^"])*\")
comment     "//".*"\n"
%%
[ \t\n]                 ;
"true"                  {return B_TRUE;}
"false"                 {return B_FALSE;}
":"      		   {return (COLON);}
";"                        {return (SEMICOLON);}
"."                        {return (DOT);}
"{"                     { return L_BRACE; }
"}"                     { return R_BRACE; }
"("                     { return L_PARAN; }
")"                     { return R_PARAN; }
"]"                     { return R_SQR; }
"["                     { return L_SQR; }
","                     { return COMMA; }
"Λ"                     {return CONJUN;}
"V"                     {return DISJUN;}
"scope"                 {return SCOPE;}
"return"                { return RETURN; }
"func"                   { return FUNC;}
"if"                    { return IF; }
"else"                  { return ELSE; }
"while"                 { return WHILE; }
"where"                 {return WHERE_C;}
"def"                  {return DEF;}
"do"                    {return DO;}
"reg"                   {return REG_64;}
"reg_16"                {return REG_16;}
"reg_32"                {return REG_32;}
"reg_id"                {return REG_64_ID;}
"reg_16_id"             {return REG_16_ID;}
"reg_32_id"             {return REG_32_ID;}
"mem"                   {return MEM;}
"Const"                 {return CONST;}
"fp"                    {return REG_64_FP;}
"sp"                    {return REG_64_SP;}
"r0"                    {return REG_64_RAX;}
"r1"                    {return REG_64_RBX;}
"pc"                    {return REG_64_PC;}
"NULL"               {return NULL_PTR;}
"global"                {return GLOBAL;}
"local"                 {return LOCAL;}
"init"                  {return INIT;}
"rule"                  {return RULE;}
"IsType"                {return ISTYPE;}
"TypeOf"                {return TYPEOF;}
"tuple"                 {return TUPLE;}
"char"                  {return CHAR;}
"pair"                  {return PAIR;}
"var"                   {return VAR;}
"uint32"                {return UINT32;}
"uint64"                {return UINT64;}
"bool"                  {return BOOL;}
"file"                  {return FILE_IO;}
"line"                  {return FILE_LINE;}
"int"                   {return INT;}
"addr"                   {return ADDR_INT;}
"double"                {return DOUBLE;}
"dict"                  {return DICT;}
"vector"                {return VECT;}
"set"                   {return SET;}
"inst"                  {return INST;}
"ssanode"               {return SSANODE;}
"loop"                  {return LOOP;}
"module"                {return MODULE;}
"basicblock"            {return BASICBLOCK;}
"func"                  {return FUNC;}
"at"                    {return T_AT;}
"before"                {return T_BEFORE;}
"after"                 { return T_AFTER;}
"entry"                 {return T_ENTRY;}
"exit"                  {return T_EXIT;}
"iter"                  {return T_ITER;}
"for"                   {return FORLOOP;}
"usage"                 {return USAGE;}
"Call"                  {return OP_CALL;}
"Mov"                   {return OP_MOV;}
"Add"                   {return OP_ADD;}
"Sub"                   {return OP_SUB;}
"Mul"                   {return OP_MUL;}
"Div"                   {return OP_DIV;}
"Nop"                   {return OP_NOP;}
"Load"                  {return OP_LOAD;}
"Store"                 {return OP_STORE;}
"Return"                {return OP_RETURN;}
"GetPtr"                {return OP_GETPTR;}
"Cmp"                   {return OP_CMP;}
"Branch"                {return OP_BRANCH;}
"+"                     { return PLUS; }
"-"                     { return MINUS; }
"*"                     { return MULT; }
"/"                     { return DIV; }
"!"                     { return NOT; }
"||"                     { return L_OR; }
"&&"                     { return L_AND; }
"|"                     {return OR;}
"&"                     {return AND;}
"^"                     {return XOR;}
">="                    { return GE; }
"<="                    { return LE; }
"=="                    { return EQ; }
"!="                    { return NEQ; }
"="                     { return INIT_OP; }
">"                     { return GT; }
"<"                     { return LT; }
{string_const}          {SAVE_TOKEN; return (STR_CONST);}
{identifier}            {SAVE_TOKEN; return (IDENTIFIER);}
{numbers}               {yylval.number = atoi(yytext); return (NUM);}
{comment}               {/*do nothing*/}
%%


void showError(){
    printf("Other input");
}
