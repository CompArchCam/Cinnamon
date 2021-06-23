SRC=
ARG=
#ifndef TARGET
#$(error TARGET is not set. set it to pin, janus or dyninst)
#endif
ifeq ($(TARGET),pin)
    SRC = codegen-pin
    ARG=TARGET_PIN
endif
ifeq ($(TARGET),janus)
    SRC = codegen-janus
    ARG=TARGET_JANUS
endif
ifeq ($(TARGET),dyninst)
    SRC = codegen-dyninst
    ARG=TARGET_DYN
endif
bdc: main.cpp parser.cpp lexer.cpp AST.o CodeGen.o parse-assembly/assem.o parse-assembly/wrtodyn.o
	g++ -g -std=c++11 -O1 -D$(ARG) -o bdc main.cpp parser.cpp lexer.cpp AST.o CodeGen.o
CodeGen.o: $(SRC)/CodeGen.h $(SRC)/CodeGen.cpp Visitor.h $(SRC)/util.h
	g++ -O2 -c -std=c++11 $(SRC)/CodeGen.cpp

AST.o:  AST.h AST.cpp Visitor.h
	g++ -O1 -c AST.cpp

parse-assembly/assem.o: parse-assembly/assem.cpp
	clang++ parse-assembly/assem.cpp -o parse-assembly/assem.o

parse-assembly/wrtodyn.o: parse-assembly/wrtodyn.cpp
	clang++ parse-assembly/wrtodyn.cpp -o parse-assembly/wrtodyn.o

parser.cpp: parser.y
	bison -t -o parser.cpp --defines=parser.hpp parser.y

lexer.cpp: lexer.lex
	lex -o lexer.cpp --header-file=lexer.h lexer.lex

.PHONY: clean
clean:
	rm -f bdc parser.cpp lexer.cpp lexer.h parser.hpp *.o
