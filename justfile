alias b := build
alias c := clean
alias d := debug
alias r := run
alias t := test

export GTEST_COLOR := "1"
export CMAKE_BUILD_TYPE := "Debug"

configure:
  cmake -B build -G "Ninja Multi-Config"

build: configure
  cmake --build build --config "$CMAKE_BUILD_TYPE"

test *ARGS: build
  cmake --build build --target hsh_test
  ctest --test-dir build -C "$CMAKE_BUILD_TYPE" --output-on-failure {{ARGS}}

run *ARGS: build
  build/src/"$CMAKE_BUILD_TYPE"/hsh {{ARGS}}

debug *ARGS="": build
  #!/bin/bash
  gdb "build/src/Debug/hsh" -ex "b main" {{ARGS}}

fmt:
  find src -name '*.cpp*' -exec clang-format -i {} \;
  find test -name '*.cpp*' -exec clang-format -i {} \;

clean:
  rm -rf build .cache
