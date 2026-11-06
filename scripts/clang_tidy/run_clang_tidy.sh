#!/bin/bash
#
# Runs clang-tidy with the configuration from .clang-tidy extended by the project specific checks
# from scripts/clang_tidy/custom_checks.yaml, and checks the changed lines for non-ASCII characters.
#
# Requirements:
#   * clang-tidy 22 or newer, including clang-tidy-diff and run-clang-tidy, for example: pip install clang-tidy==22.1.8
#   * A compilation database, for example: cmake -S . -B build -DBUILD_TESTING=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
#
# Usage:
#   scripts/clang_tidy/run_clang_tidy.sh [-p <build dir>] [--base <git ref>]   Check the lines changed since <git ref> (default: origin/main)
#   scripts/clang_tidy/run_clang_tidy.sh [-p <build dir>] --all               Check all files in the compilation database
#
# Only files in the compilation database are checked. Header files are not compiled on their own,
# so changed lines in a header are only checked when a changed source file includes that header.
# The executables can be overridden with the CLANG_TIDY, CLANG_TIDY_DIFF and RUN_CLANG_TIDY environment variables.

set -euo pipefail

REPO_ROOT=$(git rev-parse --show-toplevel)
SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
BUILD_DIR="$REPO_ROOT/build"
BASE_REF="origin/main"
CHECK_ALL=0
# Prints the first of the given commands that is available
find_command() {
  local candidate
  for candidate in "$@"; do
    if command -v "$candidate" > /dev/null 2>&1; then
      echo "$candidate"
      return 0
    fi
  done
  return 0
}

# The pip package installs clang-tidy-diff.py and run-clang-tidy.py, except on Windows where the .py suffix is dropped
CLANG_TIDY=${CLANG_TIDY:-clang-tidy}
CLANG_TIDY_DIFF=${CLANG_TIDY_DIFF:-$(find_command clang-tidy-diff.py clang-tidy-diff)}
RUN_CLANG_TIDY=${RUN_CLANG_TIDY:-$(find_command run-clang-tidy.py run-clang-tidy)}
JOBS=$(nproc 2>/dev/null || echo 4)
SOURCE_PATHSPEC=('*.cpp' '*.hpp' ':!hardware_integration/lib/**')

while [ $# -gt 0 ]; do
  case "$1" in
    -p)
      BUILD_DIR="$2"
      shift 2
      ;;
    --base)
      BASE_REF="$2"
      shift 2
      ;;
    --all)
      CHECK_ALL=1
      shift
      ;;
    -h | --help)
      sed -n '3,16p' "$0" | cut -c3-
      exit 0
      ;;
    *)
      echo "Unknown argument: $1"
      exit 2
      ;;
  esac
done

if [ ! -f "$BUILD_DIR/compile_commands.json" ]; then
  echo "No compile_commands.json found in $BUILD_DIR, configure CMake with -DCMAKE_EXPORT_COMPILE_COMMANDS=ON first."
  exit 2
fi
BUILD_DIR=$(cd "$BUILD_DIR" && pwd)

CLANG_TIDY_VERSION=$("$CLANG_TIDY" --version | sed -n 's/.*version \([0-9][0-9]*\).*/\1/p' | head -n 1)
if [ -z "$CLANG_TIDY_VERSION" ] || [ "$CLANG_TIDY_VERSION" -lt 22 ]; then
  echo "clang-tidy 22 or newer is required for the custom checks, found: $("$CLANG_TIDY" --version | grep -m 1 version)"
  exit 2
fi

WORK_DIR=$(mktemp -d)
trap 'rm -rf "$WORK_DIR"' EXIT

is_windows() {
  case "$(uname -s)" in
    MINGW* | MSYS* | CYGWIN*) return 0 ;;
    *) return 1 ;;
  esac
}

# The clang-tidy python scripts are native programs, so on Windows they need Windows style paths
native_path() {
  if is_windows; then
    cygpath -m "$1"
  else
    echo "$1"
  fi
}

CONFIG_FILE="$WORK_DIR/clang-tidy.yaml"
cat "$REPO_ROOT/.clang-tidy" "$SCRIPT_DIR/custom_checks.yaml" > "$CONFIG_FILE"

# clang-tidy-diff and run-clang-tidy cannot forward --experimental-custom-checks, so they call this wrapper instead
if is_windows; then
  CLANG_TIDY_WRAPPER="$WORK_DIR/clang-tidy.cmd"
  printf '@"%s" --experimental-custom-checks %%*\r\n' "$(cygpath -w "$(command -v "$CLANG_TIDY")")" > "$CLANG_TIDY_WRAPPER"
else
  CLANG_TIDY_WRAPPER="$WORK_DIR/clang-tidy"
  printf '#!/bin/sh\nexec "%s" --experimental-custom-checks "$@"\n' "$(command -v "$CLANG_TIDY")" > "$CLANG_TIDY_WRAPPER"
  chmod +x "$CLANG_TIDY_WRAPPER"
fi

COMMON_ARGS=(-config-file="$(native_path "$CONFIG_FILE")" -clang-tidy-binary "$(native_path "$CLANG_TIDY_WRAPPER")" -j "$JOBS" -quiet -warnings-as-errors='*')

cd "$REPO_ROOT"

if [ "$CHECK_ALL" -eq 1 ] && [ -z "$RUN_CLANG_TIDY" ]; then
  echo "run-clang-tidy was not found, install it with clang-tidy or set the RUN_CLANG_TIDY environment variable."
  exit 2
fi
if [ "$CHECK_ALL" -eq 0 ] && [ -z "$CLANG_TIDY_DIFF" ]; then
  echo "clang-tidy-diff was not found, install it with clang-tidy or set the CLANG_TIDY_DIFF environment variable."
  exit 2
fi

if [ "$CHECK_ALL" -eq 1 ]; then
  # Only check the project sources, not dependencies like googletest that are part of the compilation database
  "$RUN_CLANG_TIDY" -p "$(native_path "$BUILD_DIR")" "${COMMON_ARGS[@]}" '[/\\](isobus|hardware_integration|utility|test|examples)[/\\]'
  exit 0
fi

# Compare the working tree against the merge base, so uncommitted changes are checked as well
MERGE_BASE=$(git merge-base "$BASE_REF" HEAD)
echo "Checking the lines changed since $BASE_REF ($MERGE_BASE)"
RESULT=0

NON_ASCII_LINES=$(git diff -U0 --no-color "$MERGE_BASE" -- "${SOURCE_PATHSPEC[@]}" | grep '^+' | grep -v '^+++' | LC_ALL=C grep $'[^\t\r -~]' || true)
if [ -n "$NON_ASCII_LINES" ]; then
  echo "Found non-ASCII characters in the changed lines, only the C++ basic source character set is allowed (see CONTRIBUTING.md):"
  echo "$NON_ASCII_LINES" | cut -c2- | sed 's/^/  /'
  RESULT=1
fi

git diff -U0 --no-color "$MERGE_BASE" -- "${SOURCE_PATHSPEC[@]}" |
  "$CLANG_TIDY_DIFF" -p1 -path "$(native_path "$BUILD_DIR")" "${COMMON_ARGS[@]}" -only-check-in-db -hide-progress || RESULT=1

exit $RESULT
