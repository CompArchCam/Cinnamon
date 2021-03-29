#include <stdio.h>
#include <iostream>
#include <string>
#include "lexer.h"
#include "AST.h"
#ifdef TARGET_PIN
#include "codegen-pin/CodeGen.h"
#elif defined TARGET_JANUS
#include "codegen-janus/CodeGen.h"
#elif defined TARGET_DYN
#include "codegen-dyninst/CodeGen.h"
#endif
int main(int argc, char** argv)
{
    if (argc < 2) {
        cout << "Expecting input file name as arguments" << endl;
        return 1;
    }
    FILE* infile = fopen(argv[1], "r");
    if (infile == NULL) {
        cout << "Cannot open file " << argv[1] << endl;
        return 1;
    }
    yyin = infile;
    cout <<"------------Parsing Begin--------"<<endl;
    ProgramBlock* output;
    int out = yyparse(output);
    if(out == 1){ cout <<"parsing failed"<<endl; exit(1);}
    cout<< output <<endl;
    cout<< "-----------Parsing End-----------"<<endl;
    string outfile(argv[1]);
    cout<<"Outfile name: "<<outfile<<endl;
    size_t pos = outfile.find(".dsl");
    if( pos != std::string::npos)
        outfile.erase(pos, outfile.length());
    cout<<"Outfile name: "<<outfile<<endl;
    CodeGen code_generator(outfile);
    output->accept(code_generator);
   
   return 0;
}
