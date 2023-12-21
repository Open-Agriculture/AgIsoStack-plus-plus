# Examples

This directory contains examples of how to use the AgIsoStack library.

## Building all examples

The examples here can all be build simultaneously from the top level directory of the repository:

```bash
cmake -S . -B build -DBUILD_EXAMPLES=ON
cmake --build build
```

## Building a single example

Another approach, more akin to how most consumers would use the library, is to install the AgIsoStack first.
And then, use a call to `find_package()` to find this installation.

First, from the top level directory, build AgIsoStack-plus-plus normally

```bash
cmake -S . -B build 
cmake --build build
```

We install it to a known location, here we use `install` folder relative to the root of the repository as an example.

```bash
cmake --install build --prefix install
```

Now, build an example of your choosing, here we use the `diagnostic protocol` as an example, but any of the examples will work. Note that we need to tell CMake where to find the installed package, which is two directories up from the example directory. Hence, the `../../install` argument to `CMAKE_PREFIX_PATH`.

```bash
cd examples/diagnostic_protocol
cmake -S . -B build -DCMAKE_PREFIX_PATH=../../install
cmake --build build
```

Then, run the example. The name of the executable will be different depending on which example you built.

```bash
./build/DiagnosticProtocolExample
```
