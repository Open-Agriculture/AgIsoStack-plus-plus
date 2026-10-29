# Examples

This directory contains examples of how to use the AgIsoStack library.

## Building all examples

The examples here can all be build simultaneously from the top level directory of the repository:

```bash
cmake -S . -B build -DBUILD_EXAMPLES=ON
cmake --build build
```

To install the built examples along with the library, run:

```bash
cmake --install build --prefix install
```

The executables and their required object pool files are installed in `bin`
under the installation prefix. Set `CMAKE_INSTALL_BINDIR` when configuring to
choose a different directory. The virtual terminal and seeder examples load
their object pool files from the current working directory, so run those
examples from the installed executable directory. CAN examples still require
the appropriate CAN driver and hardware setup.

Packaging tools can stage the installation using `DESTDIR`:

```bash
DESTDIR="$PWD/staging" cmake --install build --prefix /usr
```

Examples are only installed by the top-level project when `BUILD_EXAMPLES=ON`.

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
