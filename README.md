# Cinnamon

This directory contains the code for the Cinnamon language compiler.  This compiler is described in the paper:

Cinnamon: A Domain-Specific Language for Binary Profiling and Monitoring,
Mahwish Arif, Ruoyu Zhou, Hsi-Ming Ho and Timothy M. Jones,
CGO 2021

Please cite this paper if you produce any work that builds upon this code and / or data.

## Licence

Cinnamon is released under an Apache licence.

## Building Cinnamon

Cinnamon can currently target three different binary frameworks; Janus, Pin and Dyninst.  To build the compiler:

```shell-session
export CINNAMON_ROOT = /path/to/cinnamon-source
cd $(CINNAMON_ROOT)
```

To build the Cinnamon backend for Janus:

```shell-session
make TARGET=janus
```

To build the Cinnamon backend for Pin:

```shell-session
make TARGET=pin
```

To build the Cinnamon backend for Dyninst:

```shell-session
make TARGET=dyninst
```

## Compiling a sample program

Cinnamon sample programs are available in the  `tests` directory.  The following commands will compile the Cinnamon program `ins.dsl` and integrate the resulting code into one of the target frameworks. You will need to set the path to your target framework installation in the respective scripts:

```shell-session
$(CINNAMON_ROOT)/Scripts/compileToJanus.py $CINNAMON_ROOT/tests/ins.dsl
$(CINNAMON_ROOT)/Scripts/compileToPin.py $CINNAMON_ROOT/tests/ins.dsl
$(CINNAMON_ROOT)/Scripts/compileToDyn.py $CINNAMON_ROOT/tests/ins.dsl
```

After this, the final tool can be built and run using the target framework's build instructions.

If you just want to compile the Cinnamon DSL code and not yet integrate it into a target framework, run the following command.  This will generate a number of different files containing relevant code for the cinnamon program:

```shell-session
cd $CINNAMON_ROOT
./bdc $CINNAMON_ROOT/tests/ins.dsl
```


## Target frameworks

### Janus

You can get the Janus implementation with placeholders, templates and utility libraries for Cinnamon from the main Janus repository at https://github.com/timothymjones/Janus.git, then switch to the `cinnamon` branch.

```shell-session
git clone https://github.com/timothymjones/Janus.git
cd Janus
git checkout -b cinnamon origin/cinnamon
```

Next set `JanusPATH` in `compileToJanus.py` to be the location that you have cloned Janus.

Once the code for Janus has been generated and integrated (after running the `compileToJanus.py` script from above), you can build the final tool using the following commands:

```shell-session
(cd build; cmake ..; make -j8)
```

To run the final tool on the target binary:

```shell-session
./janus/jdsl_run <target_binary>
```

### Pin

Everything required for Pin is contained within the `targets/Pin` directory.  Copy the `MyDSLTool` directory to `path-to-your-pin-root-dir/source/tools`, where `path-to-your-pin-root` should be self-explanatory.

Next set `PinPATH=your-pin-root-dir/source/tools/MyDSLTool` in `compileToPin.py`.

Once the code for Pin has been generated and integrated (after running the `compileToPin.py` script from above), you can build the final tool using the following commands:

```shell-session
cd your-pin-root-dir/source/tools/MyDSLTool
make obj-intel64/MyDSLTool.so
```

To run the final tool on the target binary:

```shell-session
your-pin-root-dir/pin -t obj-intel64/MyDSLTool.so -- <target_binary>
```

### Dyninst

You can obtain Dyninst version 10.1.0 as follows:

```shell-session
wget https://github.com/dyninst/dyninst/archive/v10.1.0.tar.gz``
tar xzvf v10.1.0.tar.gz
```

Once extracted, add `c_LoadInsn` and `c_StoreInsn` into `enum InsnCategory` in `dyninst-10.1.0/instructionAPI/h/InstructionCategories.h` and then build by following the Dyninst build instructions.

Everything else required for Dyninst is contained within the `targets/Dyninst` directory.  Copy the `MyDSLTool` directory to `path-to-your-dyn-root-dir/examples`, where `path-to-your-dyn-root-dir` should be self-explanatory.

Next set `DynPATH=path-to-your-dyn-root-dir/examples/MyDSLTool` in `compileToDyn.py`.

Once the code for Dyninst has been generated and integrated (after running the `compileToDyn.py` script from above), you can build the final tool using the following commands:

```shell-session
cd path-to-your-dyn-root-dir/examples/MyDSLTool
make
```

To run the final tool on the target binary:

```shell-session
path-to-your-dyn-root-dir/examples/MyDSLTool/DSLtool -m static -o <output_binary> <input_binary>
```
