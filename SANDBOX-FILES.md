# Introduction

This file contains a list of the few "sandbox-only" files in this repo. The
files in this list only exist in the `javafxports/openjdk-jfx` sandbox
and are not in the upstream HG repo. This is a very minimal set of files
limited primarily to "README" files that are unique to the sandbox and
Continuous Integration (CI) test scripts. Over time we expect this set
of files to shrink.

As a reminder, the general policy for making updates is:

1. A PR that touches "sandbox-only" files must _only_ modify "sandbox-only"
files. You can't mix "sandbox-only" files and upstream managed files in
the same PR.

2. After a PR that touches "sandbox-only" files is merged into `develop`,
it will need to be manually integrated into `master` as well by one of
the sandbox maintainers.

# List of sandbox-only files

```
.ci/before_install.sh
.ci/script.sh
.github/CONTRIBUTING.md
.github/README.md
.github/pr-check-whitespace.sh
.github/pr-list-files.sh
.github/sync_upstream.sh
.travis.yml
SANDBOX-FILES.md
appveyor.yml
```
