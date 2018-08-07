#!/bin/bash
build_dir=/cygdrive/c/jfx
win_build_dir=`cygpath -w "$build_dir"` || exit 1

java -jar "$win_build_dir"/tools/blastCopy/dist/blastCopy.jar $@

