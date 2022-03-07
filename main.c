#include <stdio.h>

void foo_a();
void foo_b();

#ifdef MAIN_FOO_C
  void foo_c(int x)
  {
    printf("foo_c version 3 %d\n", x);
  }
  void foo_d(int x)
  {
    printf("foo_d version fu %d\n", x);
  }
#endif

int main()
{
  printf("ok\n");
  foo_a();
  foo_b();
  #ifdef MAIN_FOO_C
    foo_c(300);
  #endif
  return 0;
}