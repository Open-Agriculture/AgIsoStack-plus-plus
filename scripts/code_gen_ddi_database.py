# copyright 2024 The Open-Agriculture Developers
import os, requests, re
from datetime import datetime
from pathlib import Path

ROOT_DIR = Path(__file__).parent.parent
ISOBUS_SRC_DIR = ROOT_DIR / "isobus/src"
ISOBUS_INCLUDE_DIR = ROOT_DIR / "isobus/include/isobus/isobus"

HEADER_FILE = ISOBUS_INCLUDE_DIR / "isobus_data_dictionary.hpp"
SOURCE_FILE = ISOBUS_SRC_DIR / "isobus_data_dictionary.cpp"

DDI_ARRAY_NAME = "DDI_ENTRIES"

# Regex to match the DDI array, it assumes the array consists of at most depth 2 of nested braces
DDI_ARRAY_REGEX = r"(?<=:" + DDI_ARRAY_NAME + r"\[\] = )(\{(?:[^{}]|\{(?:[^{}]|)*\})*\})"
# Regex to match the number of DDIs in the array, it assumes there are no lookups in the
# array where the index is a number, though lookups using variables as index is fine
DDI_ARRAY_LENGTH_REGEX = r"(?<=" + DDI_ARRAY_NAME + r"\[)(\d+)"
# Regex to match the file generation date
GENERATION_DATE_REGEX = r"(?<=This file was generated )(.*)(?=\.)"

if not HEADER_FILE.exists():
    exit("Could not find header file")

if not SOURCE_FILE.exists():
    exit("Could not find source file")

if os.path.exists("export.txt") and os.path.isfile("export.txt"):
    os.remove("export.txt")
    print("Removed stale export")

print("Downloading DDI database export")
exportURL = "https://www.isobus.net/isobus/exports/completeTXT"
r = requests.get(exportURL, allow_redirects=True)
print(r.headers.get('content-type'))
open("export.txt", 'wb').write(r.content)

numberOfDDIs = 0
with open("export.txt",'r', encoding="utf8") as f:
    processUnit = False
    for l_no, line in enumerate(f):
        if "DD Entity" in line and "Comment:" not in line:
            numberOfDDIs = numberOfDDIs + 1

with HEADER_FILE.open('r+', encoding="utf8") as f:
    contents = f.read()
    f.seek(0) # Move the cursor to the start of the file

    # Replace the generation date
    contents = re.sub(GENERATION_DATE_REGEX, datetime.today().strftime("%B %d, %Y"), contents)
    # Replace the number of DDIs
    contents = re.sub(DDI_ARRAY_LENGTH_REGEX, str(numberOfDDIs), contents)

    f.write(contents)
    f.truncate() # Remove the rest of the file (if any)


resultingArray = "{\n"
with open("export.txt",'r', encoding="utf8") as f:
    processUnit = False
    for l_no, line in enumerate(f):
        if "DD Entity" in line and "Comment:" not in line:
            processUnit = True
            entityLine = line.strip().split(' ', 3)
            strippedEntityLine = []

            for sub in entityLine:
                strippedEntityLine.append(sub.replace("\n", ""))
            
            print("Processing entity", line)
            resultingArray += f"		{{ {strippedEntityLine[2]}, \"{strippedEntityLine[3]}\", "

        if "Unit:" in line and processUnit:
            entityLine = line.split(' ', 1)
            if "n.a. -" in entityLine[1]:
                entityLine[1] = "None"
            if "not defined - not defined" in entityLine[1]:
                entityLine[1] = "None"
            resultingArray += f"\"{entityLine[1].strip()}\", "
            processUnit = False
        
        if "Resolution" in line:
            entityLine = line.split(' ', 1)
            entityLine[1] = entityLine[1].replace(",", ".")
            if '1' == entityLine[1].strip():
                entityLine[1] = '1.0'
            if '0' == entityLine[1].strip():
                entityLine[1] = '0.0'
            resultingArray += f"{entityLine[1].strip()}f }},\n"

resultingArray += "	}"

with SOURCE_FILE.open('r+', encoding="utf8") as f:
    contents = f.read()
    f.seek(0) # Move the cursor to the start of the file

    # Replace the generation date
    contents = re.sub(GENERATION_DATE_REGEX, datetime.today().strftime("%B %d, %Y"), contents)
    # Replace the number of DDIs
    contents = re.sub(DDI_ARRAY_LENGTH_REGEX, str(numberOfDDIs), contents)
    # Replace the DDI array
    contents = re.sub(DDI_ARRAY_REGEX, resultingArray, contents)

    f.write(contents)
    f.truncate() # Remove the rest of the file (if any)
