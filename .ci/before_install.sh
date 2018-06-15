#!/usr/bin/env bash
set -euo pipefail

if [[ "${TRAVIS_OS_NAME}" == osx ]]; then
  brew update
  brew install ant
  brew install findutils
  brew unlink python # fixes 'run_one_line' is not defined error in backtrace
fi

