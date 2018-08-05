#!/bin/bash

. ./env.sh

build_dir=/cygdrive/c/jfx
win_build_dir=`cygpath -w "$build_dir"` || exit 1
echo $build_dir
echo $win_build_dir


stat "$build_dir/letter_from_lady_x_re_am_Main.eml" || exit 1

gradle -stop

rm -rf "$build_dir"/* || exit 1
cp ".travis.yml" "$build_dir" || exit 1
cp "../letter_from_lady_x_re_am_Main.eml" "$build_dir" || exit 1

cp * "$build_dir"
cp -r "buildSrc" "$build_dir" &
cp -r "dependencies" "$build_dir" &
cp -r "doc-files" "$build_dir" &
cp -r "gradle" "$build_dir" &
cp -r "apps" "$build_dir" &
mkdir "$build_dir/modules"
cp -r "modules/javafx.base" "$build_dir/modules" &
cp -r "modules/javafx.controls" "$build_dir/modules" &
cp -r "modules/javafx.fxml" "$build_dir/modules" &
cp -r "modules/javafx.graphics" "$build_dir/modules" &
cp -r "modules/javafx.media" "$build_dir/modules" &
cp -r "modules/javafx.swing" "$build_dir/modules" &
cp -r "modules/javafx.swt" "$build_dir/modules" &
cp -r "modules/javafx.web" "$build_dir/modules" &
cp -r "netbeans" "$build_dir" &
cp -r "tests" "$build_dir" &
cp -r "tools" "$build_dir" &

for job in `jobs -p`
do
echo $job
    wait $job
done

cd "$build_dir"
pushd .
cd "apps/rebrand_javafx" || exit 1
ant jar || exit 1
#java -jar dist/rebrand_javafx.jar "$win_build_dir" joslynfx aubree davis || exit 1
popd
gradle all
