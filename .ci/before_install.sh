#!/usr/bin/env bash
set -euo pipefail

if [[ "${TRAVIS_OS_NAME}" == osx ]]; then
  openjdk='openjdk-10.0.1_osx-x64_bin.tar.gz'
  wget -nv https://download.java.net/java/GA/jdk10/10.0.1/fb4372174a714e6b8c52526dc134031e/10/$openjdk
  mkdir -p ~/jdk10
  tar -zxf "$openjdk" -C ~/jdk10 --strip-components 4 # Strip Contents/Home/
  ls -la ~/jdk10
  brew update
  brew install ant
  brew install findutils
  brew unlink python # fixes 'run_one_line' is not defined error in backtrace
fi

if [[ "${TRAVIS_OS_NAME}" == linux ]]; then
  wget https://raw.githubusercontent.com/sormuras/bach/master/install-jdk.sh
fi
