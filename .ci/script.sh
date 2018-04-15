#!/usr/bin/env bash
set -uo pipefail

BOOT_JDK10_VER="42"
BOOT_JDK10="jdk-10-ea+${BOOT_JDK10_VER}"

echo "which java: $(which java)"
ulimit -c unlimited -S

sh ./gradlew build -x :web:test --no-daemon --stacktrace --info

# Print core dumps when JVM crashes.
RESULT=$?

if [[ ${RESULT} -ne 0 ]]; then
  if [ ! -z ${PRINT_CRASH_LOGS+x} ]; then
    if [[ "${TRAVIS_OS_NAME}" == osx ]]; then FIND="gfind"; else FIND="find"; fi
    ${FIND} . -name "hs_err_pid*.log" -type f -printf '\n====== JVM CRASH LOG ======\n%p\n' -exec cat {} \;
    CORES=''
    if [[ "${TRAVIS_OS_NAME}" == osx ]]; then
      CORES="$(find /cores -type f -regex '*core.[0-9]{4}' -print)"
    else
      CORES="$(find . -type f -regex '*core.[0-9]{6}' -print)"
    fi

    if [ -n "${CORES}" ]; then
      for core in ${CORES}; do
        printf '\n\n======= NATIVE BACK TRACE (%s) =======\n' "$core"
        if [[ "${TRAVIS_OS_NAME}" == osx ]]; then
          lldb --batch -o "thread backtrace all" -f "$(which java)" -c "${core}"
        else
          gdb -n -batch -ex "thread apply all bt" "$(which java)" -c "${core}"
        fi
      done
    fi
  fi
  exit ${RESULT}
fi

# Download JDK 10 to use for boot JDK for building JDK 11.
cd "$TRAVIS_BUILD_DIR"
if [[ "${TRAVIS_OS_NAME}" == osx ]]; then
  BOOT_JDK_MAC="${BOOT_JDK10}_osx-x64_bin.dmg"
  wget https://download.java.net/java/jdk10/archive/"$BOOT_JDK10_VER"/BCL/"$BOOT_JDK_MAC"
  hdiutil attach "$BOOT_JDK_MAC" -mountpoint ~/_mount
  # The following dance is done to avoid calling "installer" which requires root.
  pkgutil --expand ~/_mount/JDK\ 10.pkg ~/jdk-10-compressed
  mkdir -p ~/jdk-10
  tar xvf ~/jdk-10-compressed/jdk10.pkg/Payload -C ~/jdk-10 --strip-components 3 # Strip Contents/Home/
else
  BOOT_JDK_LINUX="${BOOT_JDK10}_linux-x64_bin.tar.gz"
  wget https://download.java.net/java/jdk10/archive/"$BOOT_JDK10_VER"/BCL/"$BOOT_JDK_LINUX"
  tar xvf "$BOOT_JDK_LINUX"
fi


# Now that OpenJFX has been built, we will build OpenJDK and configure
# it to use our newly built OpenJFX
git clone --depth=1 -b dev https://github.com/adoptopenjdk/openjdk-jdk10.git jdk
cd jdk/
chmod +x configure
unset _JAVA_OPTIONS
if [[ "${TRAVIS_OS_NAME}" == osx ]]; then BOOT_JDK="~/jdk-10"; else BOOT_JDK="$TRAVIS_BUILD_DIR/jdk-10"; fi
bash configure --with-import-modules="$TRAVIS_BUILD_DIR"/build/modular-sdk --with-boot-jdk="$BOOT_JDK"

# Actually building the JDK exceeds the time limit (and maybe even the memory limit)
# of Travis, for the free/open-source tier. Keep this in case we get the proper infrastructure
# in place to build the JDK with Travis.
if false ; then
  make images
  ./build/*/images/jdk/bin/java -version
  make run-test-tier1
fi

# vim :set ts=2 sw=2 sts=2 et:
