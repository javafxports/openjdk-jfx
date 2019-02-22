#!/usr/bin/env bash
set -uo pipefail

echo "which java: $(which java)"
ulimit -c unlimited -S

echo "==============================="
if [ -n "$ANT_HOME" ]; then
    echo "ANT_HOME=$ANT_HOME"
fi
echo "which ant"
which ant
echo "ant -version"
ant -version
echo "==============================="

sh ./gradlew tasks

# Print core dumps when JVM crashes.
RESULT=$?

if [[ ${RESULT} -ne 0 ]]; then
  if [ ! -z ${PRINT_CRASH_LOGS+x} ]; then
    if [[ "${TRAVIS_OS_NAME}" == osx ]]; then FIND="gfind"; else FIND="find"; fi
    ${FIND} . -name "hs_err_pid*.log" -type f -printf '\n====== JVM CRASH LOG ======\n%p\n' -exec cat {} \;
    CORES=''
    if [[ "${TRAVIS_OS_NAME}" == osx ]]; then
      CORES="$(find /cores -type f -regex '*core.[0-9]{4}' -print)"
    else
      CORES="$(find . -type f -regex '*core.[0-9]{6}' -print)"
    fi

    if [ -n "${CORES}" ]; then
      for core in ${CORES}; do
        printf '\n\n======= NATIVE BACK TRACE (%s) =======\n' "$core"
        if [[ "${TRAVIS_OS_NAME}" == osx ]]; then
          lldb --batch -o "thread backtrace all" -f "$(which java)" -c "${core}"
        else
          gdb -n -batch -ex "thread apply all bt" "$(which java)" -c "${core}"
        fi
      done
    fi
  fi
  exit ${RESULT}
fi

bash .github/pr-check-whitespace.sh
RESULT=$?
if [[ ${RESULT} -ne 0 ]]; then
    echo "Error in white space check"
    exit ${RESULT}
fi

# vim :set ts=2 sw=2 sts=2 et:
