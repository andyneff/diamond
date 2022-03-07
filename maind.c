#include <dlfcn.h>
#include <stdio.h>

#ifdef MAIN_FOO_C
  void foo_c(int x)
  {
    printf("foo_c version 3 %d\n", x);
  }
#endif
void foo_a();

#ifndef MYDLFLAG
  #define MYDLFLAG RTLD_NOW
#endif

int main()
{
  printf("dynamic ok\n");
  foo_a();
  void (*my_funca)();
  void (*my_funcb)();
  void (*my_funcac)();
  void (*my_funcbc)();
  void (*my_func2)(int);

  void *handle_main;
  void *handlea;
  void *handleb;

  handlea = dlopen("./libad.so", MYDLFLAG);
  *(void**)(&my_funca) = dlsym(handlea, "foo_a");

  handleb = dlopen("./libbd.so", MYDLFLAG);
  *(void**)(&my_funcb) = dlsym(handleb, "foo_b");

  my_funca();
  my_funcb();
  #ifdef MAIN_FOO_C
    foo_c(111);
  #endif

  dlclose(handlea);
  dlclose(handleb);

  // Broken examples

  handle_main = dlopen("./libmain.so", MYDLFLAG);
  *(void**)(&my_funca) = dlsym(handle_main, "foo_a");
  *(void**)(&my_funcb) = dlsym(handle_main, "foo_b");
  my_funca();
  my_funcb();
  #ifdef MAIN_FOO_C
    foo_c(222);
  #endif
  // dlclose(handle_main); // this will unbreak a and b below

  handlea = dlopen("./libad.so", MYDLFLAG);
  *(void**)(&my_funca) = dlsym(handlea, "foo_a");

  handleb = dlopen("./libbd.so", MYDLFLAG);
  *(void**)(&my_funcb) = dlsym(handleb, "foo_b");

  my_funca();
  my_funcb();
  #ifdef MAIN_FOO_C
    foo_c(333);
  #endif

  dlclose(handlea);
  dlclose(handleb);
  dlclose(handle_main);
}