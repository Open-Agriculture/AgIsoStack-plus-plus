# Contributing to ISOBUS++

We accept public contributions that follow our code of conduct, pass all automated pre-merge checks, and pass a manual code review by a repo maintainer.

## What Are The Requirements?

Our style rules are based loosely on Autosar and MISRA suggestions and try to ensure the highest quality possible.

* Contributions must follow the style defined in our `.clang-format` file. You can ensure you pass this check by running `find . -iname *.hpp -o -iname *.cpp | xargs clang-format -i` at the root of the repo.
* Contributions should follow these additional style requirements, which will be check in code reviews.
	* Function names `snake_case`
	* Constant values `CAPITALIZED_SNAKE`
	* Constants on the left in `==` and `!=` checks. Like this: `if (5 == value)` NOT `if (value == 5)`. This is to prevent accidentally omitting an `=` in the operator and creating a runtime bug.
* Doxygen should compile with no warnings when run at the root of the project `doxygen doxyfile`
* Copyright notice must be included in each file
* Builds must pass the compilation github action
* `NULL` should not be used when `nullptr` can be used
* Prefer C++ over C
* C++11 or earlier is required to help provide the maximum compatibility to all modern compilers, even for embedded ones that do not support the most up-to-date versions of the standard, such as ARM Compiler 5 and 6.

## Setting up a Development Environment With GUI

You can easily set up a linux PC or virutal machine to develop for this project!

We suggest using Ubuntu or Raspian, but other systems may also work. Windows development is supported with PEAK CAN hardware devices.

### Install The Prerequisites

Linux:

Install git, a working CXX compiler, cmake, and git

```
sudo apt install cmake build-essentials git clang-format graphviz gdb
```

Install your favorite IDE if you want. We suggest Visual Studio Code because it's easy and we will use it in the remainder of this guide.

VS Code can be installed via the snap store, or by downloading it from Microsoft.

![VSCodeInstall](docs/images/vscodeInstall.png)

Clone the repo:

```
git clone https://github.com/ad3154/ISO11783-CAN-Stack.git
```

Then, open it in Code by clicking `File -> Open Folder`.

Install the C++ extension pack in Code.

![ExtensionPackInstall](docs/images/cppExtensionPack.png)

Then, build the project by clicking the "build" button and select a compiler.

![BuildTheProject](docs/images/buildingProjectFromCode.png)

This will compile the static libraries that are the core of this project.

## Building Examples

If you want to build the examples and debug with them, you'll need to tell that to CMake.

Go into the CMake Tools extension settings, and add a new configure-time argument for `-DBUILD_EXAMPLES=true`. You may need to scroll down a ways to get to it.

![CmakeToolsSettings](docs/images/cmakeToolsSettings.png)

![CompileArgs](docs/images/cmakeToolsExamples.png)

Then recompile with the same "build" button as before. Now the examples will be built.

Now, you can easily start debugging by clicking on the launch debugger button and selecting the target you want to debug.

![LaunchDebug](docs/images/launchDebug.png)

### A Note on Integration

If you are integrating this library with your project, you may need to adapt these instructions to fit your project, but this guide should be enough to get you going with the [example projects](https://github.com/ad3154/ISO11783-CAN-Stack/tree/main/examples).
