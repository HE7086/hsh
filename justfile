alias b := build
alias c := clean
alias d := debug
alias r := run
alias t := test

configure:
  cmake -B build -G "Ninja Multi-Config"

build: configure
  cmake --build build --config Debug

test *ARGS: build
  cmake --build build --target hsh_test
  ctest --test-dir build -C Debug --output-on-failure {{ARGS}}

run *ARGS: build
  build/src/Debug/hsh {{ARGS}}

debug *ARGS="": build
  #!/bin/bash
  gdb "build/src/Debug/hsh" -ex "b main" {{ARGS}}

fmt:
  find src -name '*.cpp*' -exec clang-format -i {} \;
  find test -name '*.cpp*' -exec clang-format -i {} \;

clean:
  rm -rf build .cache
