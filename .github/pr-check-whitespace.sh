#! /bin/bash

if [ -z "$TRAVIS_BRANCH" ]; then
    echo "TRAVIS_BRANCH is undefined, skipping white-space check"
    exit 0
fi

ROOTDIR=$(git rev-parse --show-toplevel)
cd $ROOTDIR

echo ""
echo "--- List of added or modified files"
echo ""

# List all files that are part of this pull request
bash .github/pr-list-files.sh

echo ""
echo "--- Checking for white space errors"
echo ""

# Check all files that are part of this pull request
bash .github/pr-list-files.sh | bash tools/scripts/checkWhiteSpace -S -x
