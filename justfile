alias b := build
alias c := clean
alias d := debug
alias r := run
alias t := test

configure:
  cmake -B build -G Ninja

build: configure
  cmake --build build

test *ARGS: build
  cmake --build build --target hsh_test
  ctest --test-dir build --output-on-failure {{ARGS}}

run *ARGS: build
  build/src/hsh {{ARGS}}

debug *ARGS="": build
  #!/bin/bash
  gdb "build/src/hsh" -ex "b main" {{ARGS}}

fmt:
  find src -name '*.cpp*' -exec clang-format -i {} \;
  find test -name '*.cpp*' -exec clang-format -i {} \;

release:
  cmake -B cmake-build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
  cmake --build cmake-build-release

clean:
  rm -rf build .cache
