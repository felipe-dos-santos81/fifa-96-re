#include <assert.h>
#include <stdio.h>
int main(void) {
  FILE *f = fopen("tests/golden/load_order.txt", "rb");
  assert(f && "load_order.txt manifest missing; run Task 2 probe");
  fclose(f);
  printf("load_order manifest present\n");
  return 0;
}
