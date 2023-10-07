import os, shutil, fnmatch, glob, fileinput
from datetime import datetime

def copyfiles(srcdir, dstdir, filepattern):
    def failed(exc):
        raise exc

    for dirpath, dirs, files in os.walk(srcdir, topdown=True, onerror=failed):
        for file in fnmatch.filter(files, filepattern):
            if "test" not in dirpath and "examples" not in dirpath and "CMakeFiles" not in dirpath:
                shutil.copy2(os.path.join(dirpath, file), dstdir)
                print("Copied ", file)

def write_isobus_hpp(includefiles, f):
    f.write(
f"""/*******************************************************************************
** @file        AgIsoStack.hpp
** @author      Automatic Code Generation
** @date        {datetime.today().strftime("%B %d, %Y")} at {datetime.now().strftime("%H:%M:%S")}
** @brief       Includes all important files in the AgIsoStack library.
**
** Copyright {datetime.today().strftime("%Y")} The AgIsoStack++ Developers
*******************************************************************************/

#ifndef AG_ISO_STACK_HPP
#define AG_ISO_STACK_HPP

""")

    for includefile in includefiles:
        f.write(f"#include <{includefile}>\n")

    f.write(f"\n");
    f.write(f"#endif // AG_ISO_STACK_HPP\n")

def write_library_properties(f):
        f.write(
f"""name=AgIsoStack
version=0.1.0
license=MIT
author=Adrian Del Grosso <delgrossoengineering@protonmail.com>
maintainer=Adrian Del Grosso <delgrossoengineering@protonmail.com>
sentence=A free ISOBUS (ISO11783) and J1939 CAN Stack for Teensy.
paragraph=Includes ISOBUS virtual terminal client, task controller client, and transport layer functionality. Based on the CMake AgIsoStack++ at https://github.com/Open-Agriculture/AgIsoStack-plus-plus.
category=Communication
architectures=teensy
includes=AgIsoStack.hpp
url=https://github.com/Open-Agriculture/AgIsoStack-Arduino

""")

def fixup_header_paths(fileName):
    f = open(fileName,'r')
    filedata = f.read()
    f.close()

    newdata = filedata.replace("isobus/isobus/","")
    newdata = newdata.replace("isobus/utility/","")
    newdata = newdata.replace("isobus/hardware_integration/","")

    f = open(fileName,'w')
    f.write(newdata)
    f.close()

arduinoLibPath = "arduino_library/"
sourceDir = "src"
sourcePath = arduinoLibPath + sourceDir

if os.path.exists(arduinoLibPath) and os.path.isdir(arduinoLibPath):
    shutil.rmtree(arduinoLibPath)
    print("Removed existing arduino library")

os.mkdir(arduinoLibPath)
print("Created directory ", arduinoLibPath)

os.mkdir(sourcePath)
print("Created directory ", sourcePath)

print("Copying source files to library")

copyfiles(".", sourcePath, "*.cpp")
copyfiles(".", sourcePath, "*.hpp")
copyfiles(".", sourcePath, "*.tpp")

print("Pruning unneeded files for Arduino platform")

filePruneList = [
"iop_file_interface.cpp",
"iop_file_interface.hpp",
"available_can_drivers.hpp",
"canal.h",
"canal_a.h",
"innomaker_usb2can_windows_plugin.hpp",
"InnoMakerUsb2CanLib.h",
"libusb.h",
"mac_can_pcan_plugin.hpp",
"mcp2515_can_interface.hpp",
"pcan_basic_windows_plugin.hpp",
"PCANBasic.h",
"PCBUSB.h",
"socket_can_interface.hpp",
"spi_hardware_plugin.hpp",
"spi_interface_esp.hpp",
"spi_transaction_frame.hpp",
"toucan_vscp_canal.hpp",
"twai_plugin.hpp",
"virtual_can_plugin.hpp",
"innomaker_usb2can_windows_plugin.cpp",
"mac_can_pcan_plugin.cpp",
"mcp2515_can_interface.cpp",
"pcan_basic_windows_plugin.cpp",
"socket_can_interface.cpp",
"spi_interface_esp.cpp",
"spi_transaction_frame.cpp",
"toucan_vscp_canal.cpp",
"twai_plugin.cpp",
"virtual_can_plugin.cpp",
"can_hardware_interface.hpp",
"can_hardware_interface.cpp",
"socketcand_windows_network_client.hpp",
"socketcand_windows_network_client.cpp",
"isobus_virtual_terminal_objects.cpp"
"isobus_virtual_terminal_objects.hpp"
"isobus_virtual_terminal_server_managed_working_set.hpp"
"isobus_virtual_terminal_server_managed_working_set.cpp"
"isobus_virtual_terminal_server.cpp"
"isobus_virtual_terminal_server.hpp"
"CMakeCXXCompilerId.cpp"]

for punableFile in filePruneList:
    if os.path.exists(os.path.join(sourcePath, punableFile)) and os.path.isfile(os.path.join(sourcePath, punableFile)):
        os.remove(os.path.join(sourcePath, punableFile))
        print("Pruning file ", punableFile)

print("Generating isobus.hpp from files in " + "./" + sourcePath + "/*.hpp")
headers = [os.path.normpath(i) for i in glob.glob("./" + sourcePath + "/*.hpp")]
strippedHeaders = list(map(lambda x: x.replace('arduino_library\\src\\','').replace('arduino_library/src/',''),headers))
print("Headers ", strippedHeaders)
f = open(os.path.join(sourcePath, "AgIsoStack.hpp"), "w")
write_isobus_hpp(strippedHeaders, f)
f.close()

print("Generating library.properties")
f = open(os.path.join(arduinoLibPath, "library.properties"), "w")
write_library_properties(f)
f.close()

print("Patching header file paths")
for header in headers:
    fixup_header_paths(header)
    print("Patched ", header)

sources = [os.path.normpath(i) for i in glob.glob("./" + sourcePath + "/*.cpp")]
for source in sources:
    fixup_header_paths(source)
    print("Patched ", source)
