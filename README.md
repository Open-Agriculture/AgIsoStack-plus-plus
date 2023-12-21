![AgIsoStack++Logo](docs/images/wideLogoTransparent.png)

![Features](docs/images/features.png)

![TaskController](docs/images/taskController.png)

## About This Library

AgIsoStack++ (Formerly Isobus++) is an MIT licensed hardware agnostic ISOBUS (ISO11783) and SAE J1939 CAN stack written in C++.

The library transparently supports the entire ISOBUS/J1939 transport layer, automatic address claiming, and many of the high level ISOBUS protocols, such as:

* Task Controller Client
* Virtual Terminal Client (aka Universal Terminal)
* ISOBUS Diagnostic Protocols
* NMEA 2000 Fast Packet
* Common guidance and speed messages

## Getting Started

Check out the [tutorial website](https://isobus-plus-plus.readthedocs.io/en/latest/) for information on ISOBUS basics, how to download this library, and how to use it. The tutorials contain in-depth examples and explanations to help get your ISOBUS or J1939 project going quickly.

## Compilation

This library is compiled with CMake.

```bash
cmake -S . -B build
cmake --build build
```

See the "Integrating this library" section below for how to compile the library as part of a top level application.

### CAN Drivers

A default CAN driver plug-in will be selected for you based on your OS, but when compiling you can explicitly choose to use one of the natively supported CAN drivers by supplying the `CAN_DRIVER` variable.

* `-DCAN_DRIVER=SocketCAN` Will compile with Socket CAN support (This is the default for Linux)
* `-DCAN_DRIVER=WindowsPCANBasic` Will compile with windows support for the PEAK PCAN drivers (This is the default for Windows)
* `-DCAN_DRIVER=MacCANPCAN` Will compile with support for the MacCAN PEAK PCAN driver (This is the default for Mac OS)
* `-DCAN_DRIVER=TWAI` Will compile with support for the ESP TWAI driver (This is the preferred ESP32 driver)
* `-DCAN_DRIVER=MCP2515` Will compile with support for the MCP2515 CAN controller
* `-DCAN_DRIVER=WindowsInnoMakerUSB2CAN` Will compile with support for the InnoMaker USB2CAN adapter (Windows)
* `-DCAN_DRIVER=TouCAN` Will compile with support for the Rusoku TouCAN (Windows)

Or specify multiple using a semicolon separated list: `-DCAN_DRIVER="<driver1>;<driver2>"`

If your target hardware is not listed above, you can easily integrate your own hardware by [implementing a few simple functions](https://github.com/Open-Agriculture/AgIsoStack-plus-plus/tree/main/hardware_integration#writing-a-new-can-driver-for-the-stack).

## Examples

There are build in examples. By default, examples are not built.
The easiest way to build them is from the top level.

```bash
cmake -S . -B build -DBUILD_EXAMPLES=ON
cmake --build build
```

## Tests

Tests are run with GTest. They can be invoked through ctest. Once the library is compiled, navigate to the build directory to run tests.

```bash
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
cd build
ctest
```

## Integrating this library

You can integrate this library into your own project with CMake if you want. Multiple methods are supported to integrate with the library.

Make sure you have CMake and Git installed:

Ubuntu:

```bash
sudo apt install cmake git
```

RHEL:

```bash
sudo dnf install cmake git
```

Windows:

When using windows, the suggested way to get CMake and a working build system is to install [Visual Studio](https://visualstudio.microsoft.com/vs/community/), and select the "Desktop Development with C++" workload as well as `Git` under "Individual Components".

### Git Submodule

Adding this library as a submodule to your project is one of the easier ways to integrate it.

Submodule the repository into your project:

```bash
git submodule add https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git <destination_folder>
git submodule update --init --recursive
```

Then, if you're using CMake, make sure to add the submodule to your project, and link it.
It is recommended to use the ALIAS targets exposed, which all follow the name `isobus::<target_name>`.

```cmake
find_package(Threads)

add_subdirectory(<path to this submodule>)

target_link_libraries(<your executable name> PRIVATE isobus::Isobus isobus::HardwareIntegration isobus::Utility Threads::Threads)
```

A full example CMakeLists.txt file can be found on the tutorial website.

### Integrating with CMake FetchContent

If you don't want to use Git submodules, you can also easily integrate this library and keep it automatically updated by having CMake manage it.

1. Create a folder called `cmake` in your project if you don't already have one.
2. Inside the `cmake` folder, create a file with the following contents:

    ```cmake
    if(NOT TARGET isobus::Isobus)
        include(FetchContent)
        FetchContent_Declare(
            AgIsoStack
            GIT_REPOSITORY https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git
            GIT_TAG        main
        )
        FetchContent_MakeAvailable(AgIsoStack)
    endif()
    ```

3. In your top-level CMakeLists.txt file, add `list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)` after your `project` command
4. In your top-level CMakeLists.txt file, add `find_package(AgIsoStack MODULE)` after the above line.
5. Link the library as explained above using `target_link_libraries`

Now when you configure your CMake cache, the library will be pulled from GitHub and automatically made available for your project.

### Precompiled

We do not officially distribute this library in binary form (DLL files, for example).

### Installing The Library

You can also install the library if you want. First, from the top level directory, build AgIsoStack-plus-plus normally

```bash
cmake -S . -B build 
cmake --build build
```

For a local install, we set it to a known location

```bash
cmake --install build --prefix install
```

For a system-wide install:

```bash
sudo cmake --install build
```

Then, use a call to `find_package()` to find this package.

### Use of our SAE/ISOBUS Manufacturer Code

If you are integrating with our library to create an ISO11783 or J1939 application and are not selling your software or device containing that software for-profit, then you are welcome to use our manufacturer number in your application.

If you are creating such an application for sale as a for-profit company, then we ask that you please obtain your own manufacturer code from SAE instead of using ours.

Our manufacturer code is 1407 (decimal).

## Documentation

You can view the pre-compiled doxygen here <https://delgrossoengineering.com/isobus-docs>

You can also generate the doxygen documentation yourself by running the `doxygen` command inside this repo's folder.

Make sure you have the prerequisites installed:

Ubuntu:

```bash
sudo apt install doxygen graphviz
```

RHEL:

```bash
sudo subscription-manager repos --enable codeready-builder-for-rhel-9-$(arch)-rpms

sudo dnf install doxygen graphviz
```

Then, generate the docs:

```bash
doxygen doxyfile
```

The documentation will appear in the docs/html folder. Open `index.html` in a web browser to start browsing the docs.

## Road Map

![RoadMap](docs/images/comingSoon.png)

## Special Thanks

This project's sponsors are a big part of making this project successful. Their support helps fund new hardware and software tools to test against, which drives up quality.

Thank you:

* Franz Höpfinger [franz-ms-muc](https://github.com/franz-ms-muc)
