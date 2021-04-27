#!/usr/bin/env python
import sys
import os
import re
import os.path
DynPATH = "/your/path/to/dyn/examples/MyDSLTool"
parserPATH="/your/path/to/cinnamon"
def usage():
	print "compileToDyn.py <program.dsl>"
if len(sys.argv) != 2:
	usage()
	sys.exit(0)

#Step 1: Invoke DSL parser and generator
dslFile = sys.argv[1]
fileName = os.path.splitext(dslFile)[0];
# To be edited by Mahwish

#os.system(parserPATH + '/parser ' + dslFile+ ' ' + fileName)  
os.system(parserPATH + '/bdc ' + dslFile)  


#Step 2: Start moving code to DynInst
with open(fileName+".instr", 'r') as static_file:
	code = static_file.read();
	code = '/*--- Insert Instrumentation Code Start ---*/\n'+code+'\n/*--- Insert Instrumentation Code End ---*/'
	with open(DynPATH+"/DSLtool.C", "r") as dest_file:
		content = dest_file.read();
		content_new = re.sub(r'/\*--- Insert Instrumentation Code Start ---\*/.*?/\*--- Insert Instrumentation Code End ---\*/', code, content, flags=re.DOTALL)
		with open(DynPATH+"/DSLtool.C", "w") as replace_file:
			replace_file.write(content_new)
			print "static rule generation code:"
			print content_new
with open(fileName+".stath", 'r') as static_h_file:
	code = static_h_file.read();
	code = '/*--- Global Var Decl Start ---*/\n'+code+'\n/*--- Global Var Decl End ---*/'
	with open(DynPATH+"/DSLtool.C", "r") as dest_file:
		content = dest_file.read();
		content_new = re.sub(r'/\*--- Global Var Decl Start ---\*/.*?/\*--- Global Var Decl End ---\*/', code, content, flags=re.DOTALL)
		with open(DynPATH+"/DSLtool.C", "w") as replace_file:
			replace_file.write(content_new)
			print "global var decl - static part"
			print content_new


with open(fileName+".loader", 'r') as dynamic_file:
	code = dynamic_file.read();
	code = '/*--- Action Functions Loading Start ---*/\n'+code+'\n/*--- Action Functions Loading End ---*/'
	with open(DynPATH+"/DSLtool.C", "r") as dest_file:
		content = dest_file.read();
		content_new = re.sub(r'/\*--- Action Functions Loading Start ---\*/.*?/\*--- Action Functions Loading End ---\*/', code, content, flags=re.DOTALL)
		with open(DynPATH+"/DSLtool.C", "w") as replace_file:
			replace_file.write(content_new)
			print "dynamic handler code:"
			print content_new

with open(fileName+".dynh", 'r') as dyn_h_file:
	code = dyn_h_file.read();
	code = '/*--- Global Var Decl Start ---*/\n'+code+'\n/*--- Global Var Decl End ---*/'
	with open(DynPATH+"/libInst.C", "r") as dest_file:
		content = dest_file.read();
		content_new = re.sub(r'/\*--- Global Var Decl Start ---\*/.*?/\*--- Global Var Decl End ---\*/', code, content, flags=re.DOTALL)
		with open(DynPATH+"/libInst.C", "w") as replace_file:
			replace_file.write(content_new)
			print "global var declarations - dyn part"
			print content_new

with open(fileName+".func", 'r') as func_file:
	code = func_file.read();
	code = '/*--- Instrumentation Functions Start ---*/\n'+code+'\n/*--- Instrumentation Functions End ---*/'
	with open(DynPATH+"/libInst.C", "r") as dest_file:
		content = dest_file.read();
		content_new = re.sub(r'/\*--- Instrumentation Functions Start ---\*/.*?/\*--- Instrumentation Functions End ---\*/', code, content, flags=re.DOTALL)
		with open(DynPATH+"/libInst.C", "w") as replace_file:
			replace_file.write(content_new)
			print "function code:"
			print content_new
with open(fileName+".funch", 'r') as header_file:
	code = header_file.read();
	code = '/*--- Instrumentation Functions Prototype Start ---*/\n'+code+'\n/*--- Instrumentation Functions Prototype End ---*/'
	with open(DynPATH+"/libInst.h", "r") as dest_file:
		content = dest_file.read();
		content_new = re.sub(r'/\*--- Instrumentation Functions Prototype Start ---\*/.*?/\*--- Instrumentation Functions Prototype End ---\*/', code, content, flags=re.DOTALL)
		with open(DynPATH+"/libInst.h", "w") as replace_file:
			replace_file.write(content_new)
			print "function header code:"
			print content_new
with open(fileName+".exit", 'r') as exit_file:
	code = exit_file.read();
	code = '/*--- Exit Function Start ---*/\n'+code+'\n/*--- Exit Function End ---*/'
	with open(DynPATH+"/libInst.C", "r") as dest_file:
		content = dest_file.read();
		content_new = re.sub(r'/\*--- Exit Function Start ---\*/.*?/\*--- Exit Function End ---\*/', code, content, flags=re.DOTALL)
		with open(DynPATH+"/libInst.C", "w") as replace_file:
			replace_file.write(content_new)
			print "thread exit code:"
			print content_new
with open(fileName+".init", 'r') as exit_file:
	code = exit_file.read();
	code = '/*--- Init Function Start ---*/\n'+code+'\n/*--- Init Function End ---*/'
	with open(DynPATH+"/libInst.C", "r") as dest_file:
		content = dest_file.read();
		content_new = re.sub(r'/\*--- Init Function Start ---\*/.*?/\*--- Init Function End ---\*/', code, content, flags=re.DOTALL)
		with open(DynPATH+"/libInst.C", "w") as replace_file:
			replace_file.write(content_new)
			print "thread exit code:"
			print content_new
