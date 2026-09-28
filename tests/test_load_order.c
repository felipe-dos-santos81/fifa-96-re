#include <assert.h>
#include <stdio.h>
int main(void) {
  FILE *f = fopen("tests/golden/load_order.txt", "rb");
  assert(f && "load_order.txt manifest missing; run Task 2 probe");
  long lines = 0;
  int c;
  while ((c = fgetc(f)) != EOF) if (c == '\n') lines++;
  assert(lines == 6);                              // full 6-file load order (Task 6 closeout)
  fclose(f);
  printf("load_order manifest present (6 lines)\n");
  return 0;
}
