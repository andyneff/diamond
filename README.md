# Diamond linking example

Revisiting the diamond linking problem and demonstrating the different "orders" that can occur.

`./compile.bsh` to build all the executables. Run `./main_*` to see all the test scenarios.

# Linking

On Linux, there are three times when symbols from multiple files will be linked together:

1. at link time - After compiling `.c` files into `.o` and `.a` files, `ld` links references to a symbol together with their definitions into an executable or a shared library (an `.so` file). However, not all symbols are resolved now; undefined symbols, as they are called, are not fully linked until load time (the next step).
2. at load time - When the executable is launched, undefined symbols whose linkage was deferred at link time are resolved now. They are resolved against the dynamically-linked dependencies (the `.so` files listed by `ldd`) by `ld.so`. This linking occurs before `int main` even starts.
3. during run time - `dlopen` can dynamically load shared libraries (`.so` files) during program execution. `ld.so` will link them (and their dependencies) into a local scope appended to the executable's global scope.

## `ld` - Link-time resolution

When linking (building) an executable or shared library (`.so` file), the rules are as follows:

1. Search the `.o`'s first
2. Always search the dynamic symbol table of the `.so`'s (the dynamically-linked dependencies)
    - The dynamic symbol table (.dynsym) includes those symbols with definitions (`nm -D --defined`) that can be used by the linker to resolve a symbol.
    - If `ld` locates the definition of a needed symbol within a `.so` file, a corresponding undefined symbol is added to the dynamic symbol table (`nm -u`) of the target---executable or shared library---being built. As a result, those symbols are not linked fully until load time (see Load-time resolution).
3. Search the `.a`'s

NOTE When an executable is built, all symbols that **it** defined are not, by default, added to the dynamic symbol table; i.e., they have local, not external, visibility, and hence, are not eligible to resolve undefined symbols. Conversely, symbols in a shared library are, by default, dynamic. This can, however, be fine-tuned with the `--export-dynamic` family of flags, the `--dynamic-list` flag or the `--version-script` flag in `ld`. [1]

NOTE step 2 is always run, even if step 1 finds a match. As a consequence of this, there is an odd corner case where a symbol can (accidentally) become dynamic when building an executable without the use of the export-control flags mentioned in the previous note; see the  `_foo` examples below.

### Duplicate symbols

- Duplicate symbols that happen during step 1 will result in a linker error: `multiple definition of 'foo_c'`. This can be demonstrated via: `./compile.bsh clash`
- Duplicate symbols that happen during steps 2 or 3 will be resolved with the first definition found winning. **No** warnings or errors are displayed.

## `ld.so` - Load-time resolution

When the executable is launched, its dynamically-linked dependencies (the `.so` files listed by `ldd`) are loaded and undefined symbols resolved/linked (i.e., their symbol definitions relocated). This starts with the executable and proceeds to the dynamic libraries linked directly to the executable in a breadth-first manner according to the order in which they were specified on the link line. The first definition found wins. The dependencies of these dependent libraries are resolved in a similar manner, and so on.

Specifically, each undefined symbol (the result of step 2 from `ld` - Link-time resolution) is resolved according to the following rules, where the first definition found in the dynamic symbol table wins:

1. Search symbols in the executable. (However, as noted above, defined symbols are not, by default, found in an executable's dynamic-symbol table.)
2. Search the dynamically-linked libraries in a breadth-first manner.

## `dlopen` - Run-time resolution

Once the executable is fully loaded and running, `dlopen` may be called to dynamically load a shared library. Undefined symbols are again resolved like those during Load-time resolution, albeit with a slight deviation (by default): a dynamically-loaded library creates a **local** scope which is searched after the global scope of the executable and its dynamically-linked dependencies.

Much like link-time resolution, each undefined symbol from the dynamically-loaded shared library (step 2 from `ld` - Link time resolution) is resolved according to the following rules, where the first definition found in the dynamic symbol table wins:

1. Search symbols in the executable.
2. Search the dynamically-linked dependencies of the executable in a breadth-first manner.
3. Search the dynamically-linked dependencies of the dlopen'd shared library in a breadth-first manner. These symbols belong to a private link chain and cannot be used to bind symbols in other dlopen's.

NOTE Although not recommended, if `RTLD_GLOBAL` is set (in contrast to `RTLD_LOCAL`, which is the default), `dlopen` will load symbols into the global namespace. There are other `ld` flags as well that can affect linking like `RTLD_DEEPBIND`, but also flags that can be set in the shared object when linking it together like `DF_SYMBOLIC`. These flags are also not recommended. [2]

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

Special note: `nm -D` on `main_foo_3` through `main_foo_8` show that `foo_c` is in the dynamic symbol table and is defined (`T`). This shows that `ld`'s rule 1 takes precedence over rule 3.

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

Explanation: This demonstrates rule 2 of run-time linking. That is, `foo_c` is not in the dynamic symbol table of the main executable (`main_dyload_2`). `foo_c` exists in the dynamically-linked dependency, libc4d.so, of `main_dyload_2`, as well as the dynamically-linked dependency of the dlopen'd shared libraries libad.so and libbd.so. However, the symbol in libc4d.so is found first.

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

- During link time, according to rule 1: `foo_c` is linked statically from the main `.o`. But `foo_c` is also found in `libc4d.so`, so it is also added to the dynamic symbol list (`nm -D --defined main_foo_dyload_2`).
- During run time, rule 1 of load-time linking is always triggered and the static version is always found.

# Additional notes

- To use clang, `export GCC=clang` before running the compile script
- There is actually a connivent way to handle the diamond-linking problem: that is to pass the `--default-symver` flag to the linker. However this is believed to potentially contain bugs and might not be stable. `export CFLAGS=-Wl,--default-symver` and compile to test this out. `main_3` and `main_4` will now work perfectly

# References

[1] https://stackoverflow.com/questions/26294841/having-object-file-symbols-become-dynamic-symbols-in-executable
[2] Ulrich Drepper, How To Write Shared Libraries. December 10, 2011. Available: https://www.akkadia.org/drepper/dsohowto.pdf
