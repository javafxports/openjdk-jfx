#!/usr/bin/env bash
set -euo pipefail

rm -f /tmp/openjdk12.html
rm -rf $HOME/bootjdk
wget http://jdk.java.net/12/ -O /tmp/openjdk12.html

url=$(grep "href.*https://download.java.net.*jdk12.*${TRAVIS_OS_NAME}-x64.*tar.gz\"" /tmp/openjdk12.html | sed -e 's,^.*https:,https:,' -e 's,\.tar.gz.*$,.tar.gz,')
echo Download boot JDK from: $url
targzfile=$(basename $url)

wget $url -O /tmp/$targzfile

mkdir $HOME/bootjdk
tar zxf /tmp/$targzfile -C $HOME/bootjdk
if [[ "${TRAVIS_OS_NAME}" == osx ]]; then
    mv $HOME/bootjdk/jdk-12.jdk/Contents/Home $HOME/bootjdk/jdk-12
fi
export JAVA_HOME=$HOME/bootjdk/jdk-12
export PATH=$JAVA_HOME/bin:$PATH
java -version

if [[ "${TRAVIS_OS_NAME}" == osx ]]; then
  brew update
  brew install ant
  brew install findutils
  brew unlink python # fixes 'run_one_line' is not defined error in backtrace
fi

