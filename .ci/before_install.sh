#!/usr/bin/env bash
set -euo pipefail

if [[ "${TRAVIS_OS_NAME}" == osx ]]; then
  brew update
  brew install findutils
  brew cask reinstall java
  brew outdated gradle || brew upgrade gradle
  brew unlink python # fixes 'run_one_line' is not defined error in backtrace
fi
