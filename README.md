# Diamond linking example

Just revisiting the diamond linking problem, and demonstrating the different "orders" that can occur.

`./compile.bsh` to build all the executable`
Run `./main_*` to see all the test scenarios.

# Linking

There are three places that Linux will link symbols from multiple files together:

1. `ld` "link time", this happens after compiling `.c` files into `.o` files.
2. Dynamic link librarys are load time. These are dynamic files and symbols that were determined at link time being loaded as soon the executable loads, before `int main` even starts
3. `dlopen` "run time". There are loaded dynamically as the program is executed, and can either be loaded to a locally `RTLD_LOCAL` or globally `RTLD_GLOBAL`

## ld Link time

The rules are relatively simple here.

1. Search the `.o`s first
2. Always search the `.so`s and add to dynamic symbols list (left most argument wins)
3. Search the `.a`s (left most argument wins) if 1 and 2 did not find a match

With the caveat that step 2 is always run, even if step 1 finds a match (more on that later in the  `_foo` examples)

### ld Link Time Duplicates

- Duplicate symbols that happen during step 1, will result in a linker error: `multiple definition of `foo_c'`. This can be demonstrated via: `./compile.bsh double`
- Duplicate symbols that happen during steps 2 or 3 will be resolved with the left most argument wins. No warnings or errors are displayed.

## Load time and runtime loading

After the executable is loaded all the dynamic libraries are loaded. During runtime, when `dlopen` is called, that library is also loaded. In both cases, the dependencies of those libraries are loaded first, before the actualy library is loaded.

As each symbol needs to be resolved, it appears to be resolved in the following order

1. Search the static symbols, but only symbols that are in the dynamic symbol list (step 2 from ld Link Time)
2. Search the Load time Dynamic libraries
3. (`dlopen` Runtime only) Search the global link chain loaded by `dlopen` with `RTLD_GLOBAL`
4. (`dlopen` Runtime only) Search the private link chain loaded by `dlopen` with `RTLD_LOCAL`

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

Explanation: This demonstrates ld linking rule 3. Left most wins.

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

Explanation: This demonstrates ld linking rule 2. Left most wins.

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

Explanation: This demonstrates ld linking rule 2. Dynamic always wins.

## `main_foo_0` - `main_foo_8`

```
$ ./main_foo_*
ok
foo_c version 3 100
foo_c version 3 200
foo_c version 3 300
```

Explanation: This demonstrates ld linking rule 1.

Special note, `nm -D` on `main_foo_3` through `main_foo_8` show that `foo_c` is in the dynamic symbol table. This that the ld linker does behave differently between rule 1 and 3

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

Explanation: This demonstrates run time linking rule 4.

- The first two lines "work" as intended, becuase `libmain.so` hasn't been loaded yes. Thus `liba.so` and `libb.so` are both loaded into separate private link chains.
- The second two lines do not work as intended, because `libmain.so` is loaded into one private link chain, thus the second lookup for `foo_c` excercises rull 4 of run time linking, and uses the first `foo_c` loaded.
- The last two lines excercise a corner condition that "so files are not reloaded, if they were loaded in an other's link chain". So `libad.so` and `libbd.so` (in addition to libc1d.so and `libc2d.so`) are never reloaded, and use the version from `libmain.so`'s link chain.

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

Explanation: This demonstrates rule 2 of run time linking. `foo_c` is never queried during link time.

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

Explanation: This demonstrates rule one of ld linking, and since `foo_c` is not a dynamic symbol in `main_foo_dyload_1`, it does not excercise rule 1 of run time or load time linking

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

- During ld link time, according to rule 1: `foo_c` is link in from the main `.o`. But `foo_c` is also found in `libc4d.so`, so it also added to the dynamic symbol list.
- During run time, rule 1 of load time is always excercised, and the static version is always found.

# Additional notes

- To use clang, `export GCC=clang` before running the compile script
- There is actually a convient way to handle the diamon linking problem, and that is to pass the `--default-symver` flag to the linker. However this is believed to potentially contain bugs and might not be stable. `export CFLAGS=-Wl,--default-symver` and compile. To test this out. `main_3` and `main_4` will now work perfectly