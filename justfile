alias b := build
alias c := clean
alias d := debug
alias r := run
alias t := test

configure:
  cmake -B build -G "Ninja Multi-Config"

build MODE="Debug": configure
  cmake --build build --config {{MODE}}

test MODE="Debug" *ARGS="": build
  cmake --build build --target test_hsh
  GTEST_COLOR=1 ctest --test-dir build -C {{MODE}} --output-on-failure {{ARGS}}

run MODE="Debug": build
  build/src/{{MODE}}/hsh

debug *ARGS="": build
  #!/bin/bash
  gdb "build/src/Debug/hsh" -ex "b main"

fmt:
  find src -name '*.cpp' -exec clang-format -i {} \;
  find include -name '*.hpp' -exec clang-format -i {} \;
  find test -name '*.cpp' -exec clang-format -i {} \;

clean:
  rm -rf build .cache
