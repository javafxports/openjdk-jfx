#!/bin/bash

echo "Enter hg"
cd hg

pushd openjfx/jfx-dev/rt/root
echo "Update openjfx/jfx-dev/rt -> (root)"
git hg fetch "http://hg.openjdk.java.net/openjfx/jfx-dev/rt"
git hg pull "http://hg.openjdk.java.net/openjfx/jfx-dev/rt"
popd

echo "Exit hg"
echo "Enter combined"

cd ../combined

echo "Check out master"
git checkout master

echo "Pull master (github)"
git pull github master

echo "Fetch (root)"
git fetch "imports/openjfx/jfx-dev/rt/root"

echo "Merge (root)"
git merge "imports/openjfx/jfx-dev/rt/root/master" -m "Merge from (root)" --no-ff

echo "Push master (github)"
git push github master --tags

echo "Check out develop"
git checkout develop

echo "Pull (github)"
git pull github develop

echo "Merge (master)"
git merge master -m "Merge from master" --no-ff

echo "Push develop (github)"
git push github develop --tags
