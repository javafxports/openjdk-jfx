#!/bin/bash

. ./env.sh

build_dir=/cygdrive/c/jfx
win_build_dir=`cygpath -w "$build_dir"` || exit 1
echo $build_dir
echo $win_build_dir


stat "$build_dir/letter_from_lady_x_re_am_Main.eml" || exit 1

gradle -stop
gradle -stop

rm -rf "$build_dir"/* || exit 1


cp ".travis.yml" "$build_dir" || exit 1
cp "../letter_from_lady_x_re_am_Main.eml" "$build_dir" || exit 1

cp * "$build_dir"
cp -r "dependencies" "$build_dir" &
cp -r "doc-files" "$build_dir" &
cp -r "gradle" "$build_dir" &
cp -r "apps" "$build_dir" &
cp -r "buildSrc" "$build_dir" &
cp -r "tests" "$build_dir" &
cp -r "netbeans" "$build_dir" &
cp -r "tools" "$build_dir"
ant -f "$win_build_dir"/tools/blastCopy/build.xml
ant -f "$win_build_dir"/tools/rebrand_javafx/build.xml
./blastcopy.sh -s "modules" -d "$win_build_dir\\modules" || exit 1

for job in `jobs -p`
do
    wait $job
done
rm -rf "$build_dir"/.gradle
rm -rf "$build_dir"/buildSrc/.gradle
cd "$build_dir"
java -jar "$win_build_dir"/tools/rebrand_javafx/dist/rebrand_javafx.jar "$win_build_dir" joslynfx aubree davis || exit 1

gradle all test
