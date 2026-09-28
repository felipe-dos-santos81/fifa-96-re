#include <assert.h>
#include <stdio.h>
#include <string.h>

static const char *expect[6] = {
  "tests/golden/fnames.dat",
  "tests/golden/lengths.dat",
  "tests/golden/pcindex.pog",
  "tests/golden/fw1.qfs",
  "tests/golden/sfx_game.bnk",
  "tests/golden/vid_game.tgv",
};

int main(void) {
  FILE *f = fopen("tests/golden/load_order.txt", "rb");
  assert(f && "load_order.txt manifest missing; run Task 2 probe");
  char data[4096];
  size_t len = fread(data, 1, sizeof data - 1, f);
  fclose(f);
  assert(len < sizeof data);
  data[len] = '\0';
  int count = 0;
  for (char *p = data; ; ) {
    char *nl = strchr(p, '\n');
    size_t ll = nl ? (size_t)(nl - p) : strlen(p);
    if (ll == 0 && !nl) break;                      // trailing newline artifact, not a line
    assert(count < 6);
    char line[512];
    assert(ll < sizeof line);
    memcpy(line, p, ll);
    line[ll] = '\0';
    assert(strcmp(line, expect[count]) == 0);
    count++;
    if (!nl) break;
    p = nl + 1;
  }
  assert(count == 6);                              // exactly 6 lines, in order
  printf("load_order manifest content and order verified (6 lines)\n");
  return 0;
}
