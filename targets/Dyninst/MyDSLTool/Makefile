#
# A GNU Makefile
#
# This Makefile builds the Dyninst mutator
#

# Make sure to set the DYNINST_ROOT environment variable to the directory where
# Dyninst is installed. The directory should contain the include and
# $(PLATFORM)/lib directories

# Or uncomment the following line and change the path accordingly
# DYNINST_ROOT = /change/me

ifeq ($(DYNINST_ROOT),)
$(error DYNINST_ROOT is not set)
endif

#ifeq ($(PLATFORM),)
#$(error PLATFORM is not set)
#endif

DYNINST_INCLUDE = $(DYNINST_ROOT)/include
#DYNINST_LIB =  $(DYNINST_ROOT)/$(PLATFORM)/lib
DYNINST_LIB =  $(DYNINST_ROOT)/lib

# These should point to where libelf and libdwarf are installed
#LOCAL_INC_DIR = /usr/local/include
#LOCAL_LIBS_DIR = /usr/local/lib

LOCAL_INC_DIR = ${HOME}/dyninst/bin/include/
LOCAL_LIBS_DIR = ${HOME}/dyninst/bin/lib

LIBELF=/p/paradyn/packages/libelf/lib
LIBDWARF=/p/paradyn/packages/libdwarf/lib

CXX = g++
#CXXFLAGS = -g -Wall -std=gnu++11
CXXFLAGS = -g -Wall -std=c++11
LIBFLAGS = -fpic -shared

CC = gcc
CFLAGS = -Wall -pedantic -g -std=gnu99

all: DSLtool libInst.so

DSLtool: DSLtool.o
	$(CXX) $(CXXFLAGS) -L$(DYNINST_LIB) \
		-L$(LIBELF) -L$(LIBDWARF) \
                -o DSLtool DSLtool.o DSL_Gen.o\
		-lcommon \
		-ldyninstAPI \
                -linstructionAPI \
		-lpthread \
                -lboost_system

libInst.so: libInst.C
	$(CXX) $(CXXFLAGS) $(LIBFLAGS)  libInst.C -o libInst.so  

DSLtool.o: DSLtool.C DSL_Gen.o
	$(CXX) $(CXXFLAGS) -I$(DYNINST_INCLUDE)  -c DSLtool.C

DSL_Gen.o: DSL_Gen.C
	$(CXX) $(CXXFLAGS) -I$(DYNINST_INCLUDE)  -c DSL_Gen.C

clean:
	rm -f DSLtool *.so *.o
