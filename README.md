<div align="center">

# AgIsoStack++ 🚜

— <ins>**Ag**</ins>riculture <ins>**ISO**</ins>-11783 <ins>**stack**</ins> for C<ins>**++**</ins>

*The completely free open-source ISOBUS library for everyone - from hobbyists to industry!*

[Documentation & Tutorials](https://agisostack-plus-plus.readthedocs.io/en/latest/index.html) | [Issues & Suggestions](https://github.com/Open-Agriculture/AgIsoStack-plus-plus/issues) | [Discussions](https://github.com/Open-Agriculture/AgIsoStack-plus-plus/discussions) | [Discord](https://discord.gg/uU2XMVUD4b) | [Telegram](https://t.me/+kzd4-9Je5bo1ZDg6)

[![Last Commit](https://img.shields.io/github/last-commit/Open-Agriculture/AgIsoStack-plus-plus)](https://github.com/Open-Agriculture/AgIsoStack-plus-plus/commits/main)
[![License](https://img.shields.io/github/license/Open-Agriculture/AgIsoStack-plus-plus)](https://github.com/Open-Agriculture/AgIsoStack-plus-plus/blob/main/LICENSE)
[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=ad3154_ISO11783-CAN-Stack&metric=alert_status)](https://sonarcloud.io/summary/new_code?id=ad3154_ISO11783-CAN-Stack)
[![Coverage](https://sonarcloud.io/api/project_badges/measure?project=ad3154_ISO11783-CAN-Stack&metric=coverage)](https://sonarcloud.io/summary/new_code?id=ad3154_ISO11783-CAN-Stack)

</div>

![AgIsoStack++Logo](docs/images/wideLogoTransparent.png)

![Features](docs/images/features.png)

![TaskController](docs/images/taskController.png)

> AgIsoStack++ simplifies implementing ISOBUS functionalities by providing a transparent and well-documented library. This allows you to concentrate on your application, without getting bogged down in rules defined by standards and guidelines.

- [Features](#features)
- [In Detail](#in-detail)
- [Getting Started](#getting-started)
- [Roadmap](#roadmap)
- [Community](#community)

## Features

- Platform independent C++ library
- Virtual Terminal Client (Universal Terminal)
- Auxiliary control (AUX-N)
- Task Controller Client and Server
- ISOBUS shortcut button (ISB)
- The complete backbone of the ISO11783 standard
- NMEA 2000 Fast Packet Protocol
- Common guidance and speed messages
- Hardware drivers for many common CAN controllers

### Hardware drivers

Building of the non-default hardware drivers can be enabled by passing the name of the driver to the CAN_DRIVER cmake variable.
Multiple drivers could be passed by separating them with semicolon, make sure to encompass them with apostrophes in this case.

Example:

```
cmake .. "-DCAN_DRIVER=VirtualCAN;SocketCAN"
```

The following hardware drivers present in the stack:

#### Windows CAN drivers

##### WindowsPCANBasic

An interface for using a PEAK PCAN devices using the [PCAN basic API provided by the PEAK System Gmbh.](https://www.peak-system.com/PCAN-Basic.239.0.html)

##### TouCAN

An interface for using a TouCAN USB probe, via the VSCP CANAL API.

The CANAL API is documented at [https://docs.vscp.org/canal/1.0/#](https://docs.vscp.org/canal/1.0/#)

The driver library for this plugin is located at [https://github.com/rusoku](https://github.com/rusoku)

##### WindowsInnoMakerUSB2CAN

An interface for using an [INNO-Maker USB2CAN](https://www.inno-maker.com/product/usb-can)

##### SYS_TEC

An interface for using a [SYS TEC sysWORXX USB CAN](https://www.systec-electronic.com/en/products/interfaces-gateways-amp-controls/sysworxx-usb-can-module1) device.

##### NTCAN

An interface for using a ESD NTCAN driver.

#### Linux CAN drivers

##### SocketCAN

A driver for working with SocketCAN interfaces on Linux. Both physical and virtual (vcan) interfaces are supported.

#### MAC CAN drivers

##### MacCANPCAN

A driver for using a PEAK PCAN device through the [MacCAN PCBUSB driver](https://www.mac-can.com).

#### Embedded/bare metal CAN drivers

##### TWAI

Driver for the Espressif ESP32 Two-Wire Automotive Interface

##### MCP2515

Driver for the MCP2515 SPI CAN interface

#### Virtual CAN drivers

##### VirtualCAN

A driver for creating a virtual CAN bus that can be used for (automated) testing. 

It could be only used for communicating between functions within the same process.

## In Detail

ISOBUS (based on the ISO-11783 standard) defines how agricultural machinery should communicate with each other on a CANbus network. Cross compatibility is achieved when different manufacturers carefully follow this standard when developing their devices. This means that a tractor from one manufacturer can communicate with an implement from another manufacturer, and vice versa.

AgIsoStack++ provides an easy-to-use interface for your application to communicate on the ISOBUS network in a compliant manner, without the need to worry about the details of the standard.
The library is is written in modern C++11 and uses the STL whenever possible. It is designed to be easy to understand is fully documented.

## Getting Started

Check out the [tutorial website](https://agisostack-plus-plus.readthedocs.io/en/latest/) for information on ISOBUS basics, how to download this library, and how to use it. The tutorials contain in-depth examples and explanations to help get your ISOBUS or J1939 project going quickly.

### Use of our SAE/ISOBUS Manufacturer Code

If you are integrating with our library to create an ISO11783 or J1939 application and are not selling your software or device containing that software for-profit, then you are welcome to use our manufacturer number in your application.

If you are creating such an application for sale as a for-profit company, then we ask that you please obtain your own manufacturer code from SAE instead of using ours.

Our manufacturer code is 1407 (decimal).

## Roadmap

![RoadMap](docs/images/comingSoon.png)

## Community

Join us on [Discord](https://discord.gg/uU2XMVUD4b) for support, to share your project, and good vibes in general! Alternatively, you can also join us on [Telegram](https://t.me/+kzd4-9Je5bo1ZDg6).

## Special Thanks

This project's sponsors are a big part of making this project successful. Their support helps fund new hardware and software tools to test against, which drives up quality.

Thank you:

- Franz Höpfinger [franz-ms-muc](https://github.com/franz-ms-muc)
- Balázs Gunics [gunicsba](https://github.com/gunicsba)
