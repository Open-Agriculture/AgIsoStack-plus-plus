# Examples

The examples here can all be build from top level. 
Another approach, more akin to how most consumers would use it, is to install the ISO11783 CAN Stack library.
Then, use a call to `find_package()` to find this package. 

The same process can be used for all examples, however `diagnostic_protocol` is just shown as a reference.

First, build the Isobus-plus-plus CAN Stack.
```
# Build the library like normal
cd /path/to/root/directory/of/Isobus-plus-plus
cmake -S . -B build 
cmake --build build
# Install it locally in a directory called "install"
cmake --install build --prefix install
```


Now, build an example of your choosing.
```
# Now, go in and build the example
cd examples/diagnostic_protocol
# Here, because we installed locally, you must supply the path to install
cmake -S . -B build -DCMAKE_PREFIX_PATH=../../install
cmake --build build
```

Finally, run the example.
```
./build/DiagnosticProtocolExampleTarget
```