# ISO 11783 CAN Stack
## An MIT licensed, hardware agnostic, control-function-focused implementation of the major ISOBUS (ISO 11783) and SAE J1939 protocols in C++

## Getting Started

Check out the [tutorial website](https://delgrossoengineering.com/isobus-tutorial/index.html) for information on ISOBUS basics, how to download this library, and how to use it. The tutorials contain in-depth examples and explanations to help get your ISOBUS or J1939 project going quickly.

## Project Status

This is a work in progress.

### Complete or in-progress:

- Address Claiming: Complete :white_check_mark:
- ISO11783 Transport Protocol (BAM and Connection Mode)
    - TP BAM Rx: Complete :white_check_mark:
    - TP BAM Tx: Complete :white_check_mark:
    - TP CM Tx: Complete :white_check_mark:
    - TP CM Rx: Complete :white_check_mark:
- ISO11783 Extended Transport Protocol
    - Complete :white_check_mark:
- ISO11783 Virtual Terminal: Implemented, Needs testing :white_check_mark: :question:
- J1939/ISO11783 Diagnostic Protocol
    - Diagnostic Message 1: Complete :white_check_mark:
    - Diagnostic Message 2: Complete :white_check_mark:
    - Diagnostic Message 3: Complete :white_check_mark:
    - Diagnostic Message 11: Complete :white_check_mark:
    - Diagnostic Message 22: Complete :white_check_mark:
    - Product Identification: Complete :white_check_mark:
    - Diagnostic Protocol message Complete :white_check_mark:
    - Software and ECU Identification Complete :white_check_mark:
    - Control Function Functionalities Message: Not yet supported :x:
        - Submit an issue please if this is a priority for your project!
- PGN Requests and Requests for Repetition Rate: Complete :white_check_mark:
- Tutorial [Complete](https://delgrossoengineering.com/isobus-tutorial/index.html) :white_check_mark:
- Debian Packaging: Complete :white_check_mark: but no official package has been released yet
- ISO11783 File Server (Client-side): In progress :hourglass:
### Planned Features (in no particular order):
- ISO11783 Task Controller (currently planned to be client only)
- Common ISO11783-5 Messages (guidance command, machine selected speed, etc.)
- J1939 DM13 (Stop/Start Broadcast)
- NMEA2000 Fast Packet Protocol
- NMEA2000 ISOBUS messages (GNSS)
- Meta: Windows OS support via some common CAN driver layers (PEAK P-CAN, for example)

### Stretch Goals:

- AEF's Tractor Implement Management Protocol (maybe)
- More example hardware integrations (Right now only Socket CAN is provided out-of-the-box)
- Sequence control
- A zero-heap implementation (static buffers only, for embedded platforms)

The real limiting factor is my time, and my lack of a Vector CANoe setup. 
I work full time and only develop on this project in my evenings.
I have limited resources - no fancy VTs to test with or anything like that.
You can help by becoming a github sponsor! Help me buy a Vector CANoe license so I can iterate faster!

## Why does this project exist?

As an engineer working on fully autonomous vehicles, one thing I often see developers and companies struggle with is using J1939 and ISO11783 CAN networks. Many vehicle OEMs have home-brewed "stacks" that are fragile and/or don't fully support basic things like address claiming and dynamic address arbitration, let alone a robust transport layer. Many commercial stacks can be very expensive, or don't have a favorable license for you to do your product integration. Even some open-source C++ J1939 stacks that I've seen take a very "non-ISO11783" approach to things, which makes layering things like the virtual terminal layer on top of them very difficult or inefficient.

I want to solve those issues! The goal of this project is to provide a C++ ISOBUS 1st approach (see "a control function approach" below) to a CAN stack that is robust and efficient.

## A control function approach

I have often seen companies think of CAN devices only in terms of 8 bit addresses. When I've tried to interface with ISOBUS devices from some companies, often I'll hear "our device is at address 0x82" or something, which is (mostly) meaningless on a bus where address arbitration is possible and common. So I ask, "Do you support arbitration? Your ISO NAME says you do." To which they reply "Oh, no, we don't support arbitration." or "That requires a reboot of our device." This belies a clear misunderstanding of ISO11783. When working with ISOBUS, we should be past those kinds of issues. We should be able to have our stack and transport layers handle that stuff in a completely transparent way. Furthermore, when I think of the API I want for an ISOBUS, I want to be able to tell the stack, "I want to talk to a virtual terminal". Not, "I want to talk to address 0x29". When your job is to work on a VT operating mask, you don't want to spend a bunch of time trying to resolve what devices are what address when all you really care about is that device's function. I also want the ability to say to the API "I want to talk to *any number* of some kind of device class or function in a highly generic, very powerful way.

Of course, legacy stuff exists. Some folks do really only want to send 8 bytes from address A to address B. I want to ensure that's still possible, but push people to think in terms of control functions.

## Compilation

This library is compiled with CMake. Currently, I am testing on Ubuntu 20.04 and RHEL 9, and the built in Socket CAN integration (if you choose to use it) only works on Linux.
```
cmake -S . -B build
cmake --build build
```

## Tests
Tests are run with GTest. They can be invoked through ctest. Once the library is compiled (see above), navigate to the build directory to run tests.
```
cd build
ctest
```

## Integrating this library

You can integrate this library into your own project with CMake if you want. Adding it as a submodule to your project is one of the easier ways to integrate it today.

Make sure you have cmake installed:

Ubuntu
```
sudo apt install cmake
```

RHEL
```
sudo dnf install cmake
```

Then, submodule the repository into your project:

```
git submodule add https://github.com/ad3154/ISO11783-CAN-Stack.git <destination_folder>
git submodule update --init --recursive
```
Then, if you're using cmake, make sure to add the submodule to your project, and link it.

```
find_package(Threads)

add_subdirectory(<path to this submodule>)

target_link_libraries(<your executable name> Isobus HardwareIntegration ${CMAKE_THREAD_LIBS_INIT})
```

A full example CMakeLists.txt file can be found on the tutorial website.

## Documentation

You can view the pre-compiled doxygen here https://delgrossoengineering.com/isobus-docs

You can also generate the doxygen documentation yourself by running the `doxygen` command inside this repo's folder.

Make sure you have the prerequisites installed:

Ubuntu:
```
sudo apt install doxygen graphviz
```

RHEL:
```
sudo subscription-manager repos --enable codeready-builder-for-rhel-9-$(arch)-rpms

sudo dnf install doxygen graphviz
```

Then, generate the docs:
```
doxygen doxyfile
```

The documentation will appear in the docs/html folder. Open `index.html` in a web browser to start browsing the docs.
