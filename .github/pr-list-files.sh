#! /bin/bash

if [ -z "$TRAVIS_BRANCH" ]; then
    echo "*** TRAVIS_BRANCH is undefined" >&2
    exit 1
fi

# Produce a list of added and modified files by a GitHub pull request
git diff --name-only --diff-filter=AM $(git merge-base HEAD $TRAVIS_BRANCH)..HEAD
