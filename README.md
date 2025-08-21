# hsh

My implementation of POSIX-like shell in C++26 (WIP)

## build

* configure
```
cmake -B build -G Ninja
```

* build
```
cmake --build build
```

* run
```bash
# interactive
build/src/hsh

# batch mode
build/src/hsh -c 'echo hello'
```

* run tests
```bash
# build tests
cmake --build build --target hsh_test

# run all tests
ctest --test-dir build --output-on-failure

# run with filter
ctest --test-dir build --output-on-failure -R SCOPE
```

* formatting
```
just fmt
```

## current state of the project
Note: although most basic functionalities are working, they are not well tested for a complex case. There are a lot of edge cases where tests did not cover.

### Done
* basic shell syntax parsing
* pipelines `|`
* background jobs `&`
* redirections `>`
* control statement `if` `for` `while` `until` `case`
* logical expression `&&`
* tilde expansion `~`
* variable expansion `$VAR`
* pathname expansion `*.txt`
* brace expansion `{a..z}`
* arithmetic expansion `$((1+1))`
* special parameters `$@`
* builtin commands:
  * cd echo exit export jobs fg bg pwd
* basic prompt `[user@host pwd]$`
* repl
* command line arguments `hsh --help`
* batch mode `hsh -c 'echo hello'`
* subshells `(...)`

### TODO
* complex parameter expansion `${VAR#PATTERN}`
* command substitution `$(...)`
* process substitution `<(...)`
* here-doc, here-string `<<EOF`
* command grouping `{... ; ...}`
* functions and scripts `function f() {...}`
* more builtin commands
* command history
* tab completion
* advanced prompt
* and more...
