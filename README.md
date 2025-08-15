# hsh

My implementation of POSIX-like shell in C++26

## build

* configure
```
cmake -B build -G "Ninja Multi-Config"
```

* build
```
cmake --build build --config Debug
```

* run
```
# interactive
build/src/Debug/hsh

# batch mode (use single quotes to prevent system shell interfering)
build/src/Debug/hsh -c 'echo hi'
```

* run tests
```
# run all tests
ctest --test-dir build -C Debug --output-on-failure

# run with filter
ctest --test-dir build -C Debug --output-on-failure -R SCOPE
```

* formatting
```
just fmt
```

## TODO

* Functions
* Globbing
* Compound commands
* Full job comtrol
* Scripting
