# Diamond linking example

Revisiting the diamond linking problem and demonstrating the different "orders" that can occur.

`./compile.bsh` to build all the executables. Run `./main_*` to see all the test scenarios.

# Linking

On Linux, there are three times when symbols from multiple files will be linked together:

1. at link time - After compiling `.c` files into `.o` and `.a` files, `ld` links them together into an executable or a shared library (an `.so` file). However, not all symbols are resolved now; undefined symbols, as they are called, are not fully linked until load time (the next step).
2. at load time - When the executable is launched, undefined symbols whose linkage was deferred at link time are resolved now. They are resolved against the dynamically-linked dependencies (the `.so` files listed by `ldd`) by `ld.so`. This linking occurs before `int main` even starts.
3. during run time - `dlopen` can dynamically load shared libraries (`.so` files) during program execution. `ld.so` will link them (and their dependencies) into a local scope appended to the executable's global scope.

## `ld` - Link-time resolution

The rules are relatively simple here.

1. Search the `.o`'s first
2. Always search the `.so`'s
    - If `ld` locates the definition of a symbol within a dynamically-linked dependency (a `.so` file), it is added as an undefined symbol to the dynamic-symbols section of the executable (as printed by `nm -u`), and is not linked fully until load time (see Load-time resolution).
3. Search the `.a`'s

NOTE step 2 is always run, even if step 1 finds a match (more on that later in the  `_foo` examples)

### Duplicate symbols

- Duplicate symbols that happen during step 1 will result in a linker error: `multiple definition of 'foo_c'`. This can be demonstrated via: `./compile.bsh clash`
- Duplicate symbols that happen during steps 2 or 3 will be resolved with the first definition found winning. **No** warnings or errors are displayed.

## `ld.so` - Load-time resolution

When the executable is launched, its dynamically-linked dependencies (the `.so` files listed by `ldd`) are loaded and undefined symbols resolved (bound/linked to their definitions). This starts with the executable and proceeds to the dynamic libraries linked directly to the executable in a breadth-first manner according to the order in which they were specified on the link line. The first definition found wins. The dependencies of these dependent libraries are resolved in a similar manner, and so on.

Each undefined symbol is resolved according to the following rules, where the first definition found wins:

1. Search symbols in the executable, but only symbols that are in the dynamic-symbol list (step 2 from `ld` - Link time resolution).
2. Search the dynamically-linked libraries in a breadth-first manner.

## `dlopen` - Run-time resolution

Once the executable is fully loaded and running, `dlopen` may be called to dynamically load a shared library. Undefined symbols are again resolved like those during Load-time resolution, albeit with a slight deviation (by default): a dynamically-loaded library creates a **local** scope which is searched after the global scope of the executable and its dynamically-linked dependencies.

Much like link-time resolution, each undefined symbol from the dynamically-loaded shared library is resolved according to the following rules, where the first definition found wins:

1. Search symbols in the executable, but only symbols that are in the dynamic-symbol list (step 2 from `ld` - Link time resolution).
2. Search the dynamically-linked libraries in a breadth-first manner.
3. If the symbol being resolved is from dynamically loaded (aka Run-time) library (`dlopen`), also search symbols in its private link chain of dependencies in a breadth-first manner.

NOTE Although not recommended, if `RTLD_GLOBAL` is set (in contrast to `RTLD_LOCAL`, which is the default), `dlopen` will load symbols into the global namespace.

# Tests

## `main_1` and `main_2`

```
$ ./main_1
ok
foo_c version 1 100
foo_c version 1 200
$ ./main_2
ok
foo_c version 2 100
foo_c version 2 200
```

Explanation: This demonstrates `ld` linking rule 3: first (static) library on the link line wins.

## `main_3` and `main_4`

```
$ ./main_3
ok
foo_c version 1 100
foo_c version 1 200
$ ./main_4
ok
foo_c version 2 100
foo_c version 2 200
```

Explanation: This demonstrates load-time resolution rule 2: first (dynamic) library on the link line wins.

## `main_5` - `main_8`

```
$ ./main_5
ok
foo_c version 2 100
foo_c version 2 200
$ ./main_6
ok
foo_c version 1 100
foo_c version 1 200
$ ./main_7
ok
foo_c version 1 100
foo_c version 1 200
$ ./main_8
ok
foo_c version 2 100
foo_c version 2 200
```

Explanation: This demonstrates `ld` linking rule 2: dynamic library always wins over a static library.

## `main_foo_0` - `main_foo_8`

```
$ ./main_foo_*
ok
foo_c version 3 100
foo_c version 3 200
foo_c version 3 300
```

Explanation: This demonstrates `ld` linking rule 1.

Special note, `nm -D` on `main_foo_3` through `main_foo_8` show that `foo_c` is in the dynamic symbol table. This shows that `ld`'s rule 1 takes precedence over rule 3.

## `main_dyload_1`

```
$ ./main_dyload_1
dynamic ok
foo_c version 1 100
foo_c version 2 200
foo_c version 1 100
foo_c version 1 200
foo_c version 1 100
foo_c version 1 200
```

Explanation: This demonstrates run-time linking rule 3.

- The first two lines "work" as intended because `libmain.so` hasn't been loaded yet. Thus `liba.so` and `libb.so` are both loaded into separate private link chains.
- The second two lines do not work as intended because `libmain.so` is loaded into one private link chain, thus the second lookup for `foo_c` exercises rule 3 of run-time linking and uses the first `foo_c` found.
- The last two lines exercise a corner case where "`.so` files are not reloaded if they were loaded in another's link chain". So `libad.so` and `libbd.so` (in addition to `libc1d.so` and `libc2d.so`) are never reloaded, and use the version from `libmain.so`'s link chain.

## `main_dyload_2`

```
$ ./main_dyload_2
dynamic ok
foo_c version 4 100
foo_c version 4 200
foo_c version 4 100
foo_c version 4 200
foo_c version 4 100
foo_c version 4 200
```

Explanation: This demonstrates rule 2 of run-time linking.

## `main_foo_dyload_1`

```
$ ./main_foo_dyload_1
dynamic ok
foo_c version 1 100
foo_c version 2 200
foo_c version 3 111
foo_c version 1 100
foo_c version 1 200
foo_c version 3 222
foo_c version 1 100
foo_c version 1 200
foo_c version 3 333
```

Explanation: This demonstrates rule 1 and rule 3 of `ld` linking. The third version of `foo_c` is linked statically from the main `.o`. Consequentially, it is not a dynamic symbol (`nm -D main_foo_dyload_1` does not include `foo_c`) and, therefore, does not trigger rule 1 during load-time or run-time linking. As a result, `foo_c` version 1 and 2 are linked according to the rules for `main_foo_1`

## `main_foo_dyload_2`

```
$ ./main_foo_dyload_2
dynamic ok
foo_c version 3 100
foo_c version 3 200
foo_c version 3 111
foo_c version 3 100
foo_c version 3 200
foo_c version 3 222
foo_c version 3 100
foo_c version 3 200
foo_c version 3 333
```

Explanation: This is an interesting chain of events:

- During link time, according to rule 1: `foo_c` is linked statically from the main `.o`. But `foo_c` is also found in `libc4d.so`, so it is also added to the dynamic symbol list (`nm -D main_foo_dyload_2`).
- During runtime, rule 1 of load-time linking is always triggered and the static version is always found.

# Additional notes

- To use clang, `export GCC=clang` before running the compile script
- There is actually a connivent way to handle the diamond-linking problem: that is to pass the `--default-symver` flag to the linker. However this is believed to potentially contain bugs and might not be stable. `export CFLAGS=-Wl,--default-symver` and compile to test this out. `main_3` and `main_4` will now work perfectly