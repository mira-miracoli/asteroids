#include "../unity/unity.h"

#include "./game_lib.h"

void setUp(void) {}

void test_collision_with_ship(void) {
  Int2 s = {.x = 0, .y = 2};
  Int2 a1 = {.x = 0, .y = 0};
  for (int i = 0; i < 6; i++) {
    for (int j = 0; j < 5; j++) {
      a1.x = i;
      a1.y = j;
      if ((i == 2 && j == 1) || (i == 2 && j == 2) || (i == 2 && j == 3) ||
          (i == 3 && j == 2) || (i == 4 && j == 2)) {
        TEST_ASSERT(collides_with_ship(s, a1));
        printf("collision found\n");
      } else {
        TEST_ASSERT(!collides_with_ship(s, a1));
      }
      printf("test passed for %d, %d\n", i, j);
    }
  }
}
void tearDown(void) {}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_collision_with_ship);
  return UNITY_END();
}
