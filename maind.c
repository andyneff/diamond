#include <dlfcn.h>
#include <stdio.h>

int main()
{
  printf("dynamic ok\n");
  void (*my_funca)();
  void (*my_funcb)();
  void (*my_func2)(int);

  void *handle_main = dlopen("./libmain.so", RTLD_LAZY);
  *(void**)(&my_funca) = dlsym(handle_main, "foo_a");
  *(void**)(&my_funcb) = dlsym(handle_main, "foo_b");
  my_funca();
  my_funcb();
  dlclose(handle_main);

  void *handlea = dlopen("./libad.so", RTLD_LAZY||RTLD_LOCAL);
  *(void**)(&my_funca) = dlsym(handlea, "foo_a");

  void *handleb = dlopen("./libbd.so", RTLD_LAZY||RTLD_LOCAL);
  *(void**)(&my_funcb) = dlsym(handleb, "foo_b");

  my_funca(); // crashes here?
  my_funcb();

  dlclose(handlea);
  dlclose(handleb);

  dlclose(handle_main);
}