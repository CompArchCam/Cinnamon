# Pin integration

This directory contains templates and place holders for Cinnamon code to be integrated with Pin.  To start, copy the `MyDSLTool` directory to Pin's tools directory:

```shell-session
cp -r MyDSLTool path-to-your-pin-root-dir/source/tools
```

Now your `PinPATH` in `compileToPin.py` will be `your-pin-root-dir/source/tools/MyDSLTool`.  Compile the tool:

```shell-session
cd your-pin-root-dir/source/tools/MyDSLTool
make obj-intel64/MyDSLTool.so
```

Run the tool:

```shell-session
your-pin-root-dir/pin -t obj-intel64/MyDSLTool.so -- <target_binary>
```
