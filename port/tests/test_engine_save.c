#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fifa96_engine/fifa96_competition.h"
#include "fifa96_engine/fifa96_save.h"

static void make_comp(struct fifa96_competition *c) {
  const uint8_t pool[8] = {0, 1, 2, 3, 4, 5, 6, 7};
  fifa96_competition_init(c);
  assert(fifa96_competition_start(c, FIFA96_COMPETITION_LEAGUE, 0, pool, 8,
                                  0x12345678u) == 0);
  assert(c->state == FIFA96_COMPETITION_ACTIVE);
}

int main(void) {
  char dir[] = "/tmp/f96saveXXXXXX";
  assert(mkdtemp(dir) != NULL);
  struct fifa96_competition c;
  struct fifa96_competition loaded;
  struct fifa96_settings s;
  struct fifa96_settings sl;
  struct fifa96_save_info info;
  char path[FIFA96_SAVE_PATH_MAX];
  char label[FIFA96_SAVE_SUMMARY_MAX];
  make_comp(&c);
  fifa96_settings_init(&s);
  fifa96_settings_defaults(&s);
  s.value[0x0E] = 5;
  fifa96_settings_init(&sl);

  assert(fifa96_save_path(path, sizeof path, dir, 0) == 0);
  assert(strstr(path, "F96SAVE00.DAT") != NULL);
  assert(fifa96_save_path(path, sizeof path, dir, 10) != 0);
  assert(fifa96_save_slot_label(dir, 0, label, sizeof label) == 0);
  assert(strcmp(label, "Empty") == 0);
  assert(fifa96_save_inspect(dir, 0, &info) != 0);
  assert(info.valid == 0);

  assert(fifa96_save_write(dir, 0, &c, &s) == 0);
  assert(fifa96_save_inspect(dir, 0, &info) == 0);
  assert(info.valid == 1);
  assert(info.kind == FIFA96_COMPETITION_LEAGUE);
  assert(info.state == FIFA96_COMPETITION_ACTIVE);
  assert(fifa96_save_slot_label(dir, 0, label, sizeof label) == 0);
  assert(label[0] != '\0' && strchr(label, '\n') == NULL);

  assert(fifa96_save_read(dir, 0, &loaded, &sl) == 0);
  assert(loaded.kind == c.kind && loaded.state == c.state);
  assert(loaded.team_count == c.team_count);
  assert(loaded.user_team == c.user_team);
  assert(loaded.round == c.round);
  assert(loaded.current == c.current);
  assert(loaded.fixture_count == c.fixture_count);
  assert(memcmp(loaded.teams, c.teams, sizeof c.teams) == 0);
  assert(memcmp(loaded.fixtures, c.fixtures, sizeof c.fixtures) == 0);
  assert(sl.value[0x0E] == 5);

  assert(fifa96_save_write(dir, 3, &c, &s) == 0);
  assert(fifa96_save_read(dir, 3, &loaded, &sl) == 0);
  assert(fifa96_save_read(dir, 4, &loaded, &sl) != 0);

  /* Truncate and corrupt the slot; both must be rejected and leave outputs
   * untouched. */
  assert(fifa96_save_path(path, sizeof path, dir, 0) == 0);
  FILE *f = fopen(path, "wb");
  assert(f != NULL);
  assert(fwrite("F96SAVE1", 1, 8, f) == 8);
  assert(fclose(f) == 0);
  memset(&loaded, 0xAA, sizeof loaded);
  memset(&sl, 0xAA, sizeof sl);
  assert(fifa96_save_read(dir, 0, &loaded, &sl) != 0);
  assert(fifa96_save_slot_label(dir, 0, label, sizeof label) == 0);
  assert(strcmp(label, "Empty") == 0);

  assert(fifa96_save_path(path, sizeof path, dir, 0) == 0);
  f = fopen(path, "wb");
  assert(f != NULL);
  assert(fwrite("not a save", 1, 10, f) == 10);
  assert(fclose(f) == 0);
  assert(fifa96_save_read(dir, 0, &loaded, &sl) != 0);

  char p1[FIFA96_SAVE_PATH_MAX], p2[FIFA96_SAVE_PATH_MAX], p3[FIFA96_SAVE_PATH_MAX];
  assert(fifa96_save_path(p1, sizeof p1, dir, 0) == 0);
  assert(fifa96_save_path(p2, sizeof p2, dir, 3) == 0);
  assert(fifa96_save_path(p3, sizeof p3, dir, 4) == 0);
  remove(p1);
  remove(p2);
  remove(p3);
  assert(rmdir(dir) == 0);
  puts("test_engine_save: ok");
  return 0;
}
