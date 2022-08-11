# ISO11783 CAN Stack
## An MIT licensed, hardware agnostic, control-function-focused implementation of the major ISOBUS and J1939 protocols in C++
This library is intended to eventually provide the following features:
- ISO11783 Transport Protocol (BAM and Connection Mode)
- ISO11783 Extended Transport Protocol
- ISO11783 Virtual Terminal (Up to version 3 initially, maybe up to 6 later)
- ISO11783 File Server
- ISO11783 Task Controller (currently planned to be client only)
- Common ISO11783-5 Messages (guidance command, machine selected speed, etc.)
- J1939 DM1/2
- J1939 DM14
- NMEA2000 Fast Packet Protocol
- NMEA2000 ISOBUS messages (GNSS)
- AEF's Tractor Implement Management Protocol (maybe)
- More example hardware integrations (Right now only Socket CAN is provided out-of-the-box)
- more???

The real limiting factor is my time, and my lack of a Vector CANoe setup. 
I work full time and only develop on this project in my evenings.
I have limited resources - no fancy VTs to test with or anything like that.
You can help by becoming a github sponsor! Help me buy a Vector CANoe license so I can iterate faster!

## IMPORTANT NOTE
This project is currently in its infancy, and is *very much* a work in progress.
I mean it! I would not use it yet until I finish the basic transport layer, add a bunch of docs and a bunch of testing!
But... I do appreciate you stopping by and any feedback or PRs you might want to provide.

## Why does this project exist?
As an engineer working on fully autonomous vehicles, one thing I often see developers and companies struggle with is using J1939 and ISO11783 CAN networks. Many vehicle OEMs have home-brewed "stacks" that are fragile and/or don't fully support basic things like address claiming and dynamic address arbitration, let alone a robust transport layer. Many commercial stacks can be very expensive, or don't have a favorable license for you to do your product integration. Even some open-source C++ J1939 stacks that I've seen take a very "non-ISO11783" approach to things, which makes layering things like the virtual terminal layer on top of them very difficult or inefficient.

I want to solve those issues! The goal of this project is to provide a C++ ISOBUS 1st approach (see "a control function approach" below) to a CAN stack that is robust and efficient.

## A control function approach
I have often seen companies think of CAN devices only in terms of 8 bit addresses. When I've tried to interface with ISOBUS devices from some companies, often I'll hear "our device is at address 0x82" or something, which is (mostly) meaningless on a bus where address arbitration is possible and common. So I ask, "Do you support arbitration? Your ISO NAME says you do." To which they reply "Oh, no, we don't support arbitration." or "That requires a reboot of our device." This belies a clear misunderstanding of ISO11783. When working with ISOBUS, we should be past those kinds of issues. We should be able to have our stack and transport layers handle that stuff in a completely transparent way. Furthermore, when I think of the API I want for an ISOBUS, I want to be able to tell the stack, "I want to talk to a virtual terminal". Not, "I want to talk to address 0x29". When your job is to work on a VT operating mask, you don't want to spend a bunch of time trying to resolve what devices are what address when all you really care about is that device's function. I also want the ability to say to the API "I want to talk to *any number* of some kind of device class or function in a highly generic, very powerful way.

Of course, legacy stuff exists. Some folks do really only want to send 8 bytes from address A to address B. I want to ensure that's still possible, but push people to think in terms of control functions.

## Compilation
This library is compiled with CMake. Currently, I am only testing on Ubuntu 20.04, and the built in Socket CAN integration (if you choose to use it) only works on Linux.
```
cmake -S . -B build
cmake --build build
```
You can add it to your own project with CMake if you want!
More instructions coming soon.
