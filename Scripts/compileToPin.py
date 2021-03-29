#!/usr/bin/env python
import sys
import os
import re
import os.path
PinPATH = "your-path-to-pin/source/tools/MyDSLTool"
parserPATH="your-path-to-cinnamon/cinnamon"
def usage():
	print "compileToPin.py <program.dsl>"
if len(sys.argv) != 2:
	usage()
	sys.exit(0)

#Step 1: Invoke DSL parser and generator
dslFile = sys.argv[1]
fileName = os.path.splitext(dslFile)[0];

exit_status = os.system(parserPATH + '/bdc ' + dslFile)  
if exit_status != 0:
    print "Compilation Unsuccessful"
    sys.exit(0)

#Step 2: Start moving code to Pin
with open(fileName+".ins", 'r') as ins_file:
	code = ins_file.read();
	code = '/*--- INS Event Starts ---*/\n'+code+'\n/*--- INS Event Ends ---*/'
	with open(PinPATH+"/MyDSLTool.cpp", "r") as dest_file:
		content = dest_file.read();
		content_new = re.sub(r'/\*--- INS Event Starts ---\*/.*?/\*--- INS Event Ends ---\*/', code, content, flags=re.DOTALL)
		with open(PinPATH+"/MyDSLTool.cpp", "w") as replace_file:
			replace_file.write(content_new)
			print "INS handler code:"
			print content_new

with open(fileName+".ptype", 'r') as ins_file:
	code = ins_file.read();
	code = '/*--- Register Instrumentation Function Starts ---*/\n'+code+'\n/*--- Register Instrumentation Function Ends ---*/'
	with open(PinPATH+"/MyDSLTool.cpp", "r") as dest_file:
		content = dest_file.read();
		content_new = re.sub(r'/\*--- Register Instrumentation Function Starts ---\*/.*?/\*--- Register Instrumentation Function Ends ---\*/', code, content, flags=re.DOTALL)
		with open(PinPATH+"/MyDSLTool.cpp", "w") as replace_file:
			replace_file.write(content_new)
			print "Registering instrumentation routines:"
			print content_new

with open(fileName+".bbl", 'r') as bbl_file:
	code = bbl_file.read();
	code = '/*--- BBL Event Starts ---*/\n'+code+'\n/*--- BBL Event Ends ---*/'
	with open(PinPATH+"/MyDSLTool.cpp", "r") as dest_file:
		content = dest_file.read();
		content_new = re.sub(r'/\*--- BBL Event Starts ---\*/.*?/\*--- BBL Event Ends ---\*/', code, content, flags=re.DOTALL)
		with open(PinPATH+"/MyDSLTool.cpp", "w") as replace_file:
			replace_file.write(content_new)
			print "BBL handler code:"
			print content_new

with open(fileName+".img", 'r') as bbl_file:
	code = bbl_file.read();
	code = '/*--- IMG Event Starts ---*/\n'+code+'\n/*--- IMG Event Ends ---*/'
	with open(PinPATH+"/MyDSLTool.cpp", "r") as dest_file:
		content = dest_file.read();
		content_new = re.sub(r'/\*--- IMG Event Starts ---\*/.*?/\*--- IMG Event Ends ---\*/', code, content, flags=re.DOTALL)
		with open(PinPATH+"/MyDSLTool.cpp", "w") as replace_file:
			replace_file.write(content_new)
			print "IMG handler code:"
			print content_new

with open(fileName+".rtn", 'r') as bbl_file:
	code = bbl_file.read();
	code = '/*--- RTN Event Starts ---*/\n'+code+'\n/*--- RTN Event Ends ---*/'
	with open(PinPATH+"/MyDSLTool.cpp", "r") as dest_file:
		content = dest_file.read();
		content_new = re.sub(r'/\*--- RTN Event Starts ---\*/.*?/\*--- RTN Event Ends ---\*/', code, content, flags=re.DOTALL)
		with open(PinPATH+"/MyDSLTool.cpp", "w") as replace_file:
			replace_file.write(content_new)
			print "RTN handler code:"
			print content_new

with open(fileName+".func", 'r') as func_file:
	code = func_file.read();
	code = '/*--- Callback Functions Start ---*/\n'+code+'\n/*--- Callback Functions End ---*/'
	with open(PinPATH+"/MyDSLTool.cpp", "r") as dest_file:
		content = dest_file.read();
		content_new = re.sub(r'/\*--- Callback Functions Start ---\*/.*?/\*--- Callback Functions End ---\*/', code, content, flags=re.DOTALL)
		with open(PinPATH+"/MyDSLTool.cpp", "w") as replace_file:
			replace_file.write(content_new)
			print "function code:"
			print content_new

with open(fileName+".funch", 'r') as header_file:
	code = header_file.read();
	code = '/*--- Include Headers Start ---*/\n'+code+'\n/*--- Include Headers End ---*/'
	with open(PinPATH+"/MyDSLTool.cpp", "r") as dest_file:
		content = dest_file.read();
		content_new = re.sub(r'/\*--- Include Headers Start ---\*/.*?/\*--- Include Headers End ---\*/', code, content, flags=re.DOTALL)
		with open(PinPATH+"/MyDSLTool.cpp", "w") as replace_file:
			replace_file.write(content_new)
			print "function header code:"
			print content_new
with open(fileName+".exit", 'r') as exit_file:
	code = exit_file.read();
	code = '/*--- Exit Function Starts ---*/\n'+code+'\n/*--- Exit Function Ends ---*/'
	with open(PinPATH+"/MyDSLTool.cpp", "r") as dest_file:
		content = dest_file.read();
		content_new = re.sub(r'/\*--- Exit Function Starts ---\*/.*?/\*--- Exit Function Ends ---\*/', code, content, flags=re.DOTALL)
		with open(PinPATH+"/MyDSLTool.cpp", "w") as replace_file:
			replace_file.write(content_new)
			print "thread exit code:"
			print content_new

with open(fileName+".init", 'r') as init_file:
	code = init_file.read();
	code = '/*--- Init Function Starts ---*/\n'+code+'\n/*--- Init Function Ends ---*/'
	with open(PinPATH+"/MyDSLTool.cpp", "r") as dest_file:
		content = dest_file.read();
		content_new = re.sub(r'/\*--- Init Function Starts ---\*/.*?/\*--- Init Function Ends ---\*/', code, content, flags=re.DOTALL)
		with open(PinPATH+"/MyDSLTool.cpp", "w") as replace_file:
			replace_file.write(content_new)
			print "thread exit code:"
			print content_new
