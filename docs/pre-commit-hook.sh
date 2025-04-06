#!/bin/bash

# Pre commit hook to perform the following actions:
# * Run inplace clang-format on each committed *.cpp and *.hpp file
# * Run inplace cmake-format on each committed CMakeLists.txt file
# * Check all committed *.cpp and *.hpp file for containing calls for CANStackLogger and prevents commit if found

STAGED_FILES=$(git diff --cached --name-only --diff-filter=ACM)

CPP_HPP_FILES=()
CMAKELISTS_FILES=()

for file in $STAGED_FILES; do
  if [[ "$file" =~ \.(cpp|hpp)$ ]]; then
    CPP_HPP_FILES+=("$file")
  elif [[ "$(basename "$file")" == "CMakeLists.txt" ]]; then
    CMAKELISTS_FILES+=("$file")
  fi
done

for file in "${CPP_HPP_FILES[@]}"; do
  if [ -f "$file" ]; then
    clang-format -i "$file"
    git add "$file"
  fi
done

for file in "${CMAKELISTS_FILES[@]}"; do
  if [ -f "$file" ]; then
    cmake-format -i "$file"
    git add "$file"
  fi
done

CANSTACK_LOGGER_FORBIDDEN_METHODS=("warn" "critical" "error" "info" "debug")
EXCLUDED_FILES=("can_stack_logger.cpp" "can_stack_logger.hpp")

FILES=$(git diff --cached --name-only --diff-filter=ACM | grep -E '\.(cpp|hpp)$')

error_found=0

for file in $FILES; do
  for excluded in "${EXCLUDED_FILES[@]}"; do
    if [[ "$(basename "$file")" == "$excluded" ]]; then
      continue 2
    fi
  done

  added_lines=$(git diff --cached -U0 "$file" | grep '^+' | grep -v '^+++' | cut -c2-)

  while read -r line; do
    for pattern in "${CANSTACK_LOGGER_FORBIDDEN_METHODS[@]}"; do
      if [[ "$line" == *"CANStackLogger::$pattern"* ]]; then
        echo "Found forbidden usage in $file:"
        echo "  $line"
        error_found=1
      fi
    done
  done <<< "$added_lines"
done

if [ "$error_found" -eq 1 ]; then
  echo
  echo "Please use the LOG_WARN/CRITICAL/ERROR/INFO/DEBUG macros instead of the CANStackLogger::warn/critical/error/info/debug"
  echo "Otherwise the build will be broken with disabled CAN stack logger."
  exit 1
fi

exit 0


