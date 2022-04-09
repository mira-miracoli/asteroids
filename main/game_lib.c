#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>

#include "../tui/tui.h"

#include "./game_lib.h"

void draw_info_bar(GameState *gs) {
  char buf[255];
  sprintf(buf, "LIFES: %d    POINTS: %d    DISTANCE: %d    POWERUP: %d",
          gs->ship.health, gs->points, gs->time_step, gs->ship.powerup_time);
  tui_set_str_at(0, gs->term_size.y - 1, buf, FG_WHITE, BG_BLACK);
}

void draw_frame(GameState *gs) {
  Cell c = (Cell){
      .content = ' ', .text_color = FG_WHITE, .background_color = BG_WHITE};
  Int2 frame_begin = {gs->field_begin.x - 1, gs->field_begin.y - 1};
  Int2 frame_end = {gs->field_end.x + 1, gs->field_end.y + 1};
  for (size_t x = frame_begin.x; x < frame_end.x; ++x) {
    *tui_cell_at(x, frame_begin.y) = c;
    *tui_cell_at(x, frame_end.y - 1) = c;
  }
  for (size_t y = frame_begin.y; y < frame_end.y; ++y) {
    *tui_cell_at(frame_begin.x, y) = c;
    *tui_cell_at(frame_end.x - 1, y) = c;
  }
}

bool is_field_coordinate(GameState *gs, int x, int y) {
  Int2 size = gs->field_size;
  bool x_is_valid = 0 <= x && x < size.x;
  bool y_is_valid = 0 <= y && y < size.x;
  return x_is_valid && y_is_valid;
}

Cell *field_cell_at(GameState *gs, int x, int y) {
  /* Same as `assert(is_field_coordinate(gs, x, y))` but prints a stack trace if
   * used with the address sanitizer.
   *
   * The stack trace allows you to find out which function call caused the
   * invalid coordinates, which greatly simplifies debugging.
   * (You will probably run into this.)
   *
   * To get the stack trace, we use an ugly hack: if the assertion fails, we
   * simply cause a segmentation fault by writing to the NULL-Pointer, which the
   * address sanitizer then detects and spits out a stack trace for us :3
   */
  if (!is_field_coordinate(gs, x, y)) {
    tui_shutdown();
    printf(FG_RED "ASSERTION FAILED: Coordinate (%d, %d) is not a valid game "
                  "field coordinate.\n\n" COLOR_RESET,
           x, y);
    fflush(stdout);
    int *null = NULL;
    *null = 42;
  }

  return tui_cell_at(x + gs->field_begin.x, y + gs->field_begin.y);
}

void draw_ship(GameState *gs) {
  Cell beam = (Cell){
      .content = '-', .text_color = FG_YELLOW, .background_color = BG_BLACK};
  Cell engine = (Cell){
      .content = '=', .text_color = FG_YELLOW, .background_color = BG_BLACK};
  Cell body = (Cell){
      .content = ' ', .text_color = FG_GREEN, .background_color = BG_GREEN};
  Cell top = (Cell){
      .content = '>', .text_color = FG_YELLOW, .background_color = BG_BLACK};
  Int2 ship_begin = {gs->ship.pos.x, gs->ship.pos.y - 1};
  for (int i = ship_begin.y; i < ship_begin.y + 3; i++) {
    *field_cell_at(gs, ship_begin.x, i) = beam;
  }
  for (int i = ship_begin.y; i < ship_begin.y + 3; i++) {
    *field_cell_at(gs, ship_begin.x + 1, i) = engine;
  }
  for (int i = ship_begin.y; i < ship_begin.y + 3; i++) {
    *field_cell_at(gs, ship_begin.x + 2, i) = body;
    if (i == ship_begin.y + 1) {
      *field_cell_at(gs, ship_begin.x + 2, i) = body;
      *field_cell_at(gs, ship_begin.x + 3, i) = body;
      *field_cell_at(gs, ship_begin.x + 4, i) = top;
    }
  }
  if (gs->ship.powerup_time > 0) {
    *field_cell_at(gs, ship_begin.x + 3, ship_begin.y) = top;
    *field_cell_at(gs, ship_begin.x + 3, ship_begin.y + 2) = top;
  }
}

void draw_projectiles(GameState *gs) {
  Cell p = (Cell){
      .content = '>', .text_color = FG_HI_RED, .background_color = BG_BLACK};
  Int2 *ppos = NULL;
  for (size_t i = 0; i < vec_length(gs->projectiles); i++) {
    ppos = *vec_at(gs->projectiles, i);
    *field_cell_at(gs, ppos->x, ppos->y) = p;
  }
}

void draw_asteroids(GameState *gs) {
  Cell a = (Cell){
      .content = ' ', .text_color = FG_WHITE, .background_color = BG_WHITE};
  Int2 *ppos = NULL;
  for (size_t i = 0; i < vec_length(gs->asteroids); i++) {
    ppos = *vec_at(gs->asteroids, i);
    if (is_field_coordinate(gs, ppos->x, ppos->y)) {
      *field_cell_at(gs, ppos->x, ppos->y) = a;
    }
  }
}

void draw_powerups(GameState *gs) {
  Cell p = (Cell){
      .content = '@', .text_color = FG_GREEN, .background_color = BG_BLACK};
  Int2 *pos = NULL;
  for (size_t i = 0; i < vec_length(gs->powerups); i++) {
    pos = *vec_at(gs->powerups, i);
    if (is_field_coordinate(gs, pos->x, pos->y)) {
      *field_cell_at(gs, pos->x, pos->y) = p;
    }
  }
}

void draw_mines(GameState *gs) {
  Cell p = (Cell){.content = 'X',
                  .text_color = FG_HI_MAGENTA,
                  .background_color = BG_BLACK};
  Int2 *pos = NULL;
  for (size_t i = 0; i < vec_length(gs->mines); i++) {
    pos = *vec_at(gs->mines, i);
    if (is_field_coordinate(gs, pos->x, pos->y)) {
      *field_cell_at(gs, pos->x, pos->y) = p;
    }
  }
}

void draw_explosions(GameState *gs) {
  Cell exp = (Cell){
      .content = '#', .text_color = FG_YELLOW, .background_color = BG_HI_RED};
  Explosion *e = NULL;
  size_t x = 0;
  size_t y = 0;
  for (size_t i = 0; i < vec_length(gs->explosions); i++) {
    e = *vec_at(gs->explosions, i);
    x = e->pos.x;
    y = e->pos.y;
    if (e->age == 0) {
      *field_cell_at(gs, x, y) = exp;
    } else {
      if (is_field_coordinate(gs, x + (e->age * 2), y)) {
        *field_cell_at(gs, x + (e->age * 2), y) = exp;
      }
      if (is_field_coordinate(gs, x - (e->age * 2), y)) {
        *field_cell_at(gs, x - (e->age * 2), y) = exp;
      }
      if (is_field_coordinate(gs, x, y + e->age)) {
        *field_cell_at(gs, x, y + e->age) = exp;
      }
      if (is_field_coordinate(gs, x, y - e->age)) {
        *field_cell_at(gs, x, y - e->age) = exp;
      }
    }
  }
}

bool handle_input(GameState *gs, char c) {
  if (c == 'w') {
    if (gs->ship.pos.y >= 3) {
      gs->ship.pos.y -= 1;
    }
  }
  if (c == 'a') {
    if (gs->ship.pos.x >= 2) {
      gs->ship.pos.x -= 1;
    }
  }
  if (c == 's') {
    if (gs->ship.pos.y <= gs->field_size.y - 4) {
      gs->ship.pos.y += 1;
    }
  }
  if (c == 'd') {
    if (gs->ship.pos.x <= gs->field_size.x - 7) {
      gs->ship.pos.x += 1;
    }
  }
  if (c == ' ') {
    Int2 *p = malloc(sizeof(Int2));
    p->x = gs->ship.pos.x + 5;
    p->y = gs->ship.pos.y;
    if (p == NULL) {
      exit(1);
    } else {
      vec_push(gs->projectiles, p);
      if (gs->ship.powerup_time > 0) {
        Int2 *p1 = malloc(sizeof(Int2));
        p1->x = gs->ship.pos.x + 3;
        p1->y = gs->ship.pos.y - 1;
        vec_push(gs->projectiles, p1);
        Int2 *p2 = malloc(sizeof(Int2));
        p2->y = gs->ship.pos.y + 1;
        p2->x = gs->ship.pos.x + 3;
        vec_push(gs->projectiles, p2);
      }
    }
  }
  if (c == 'q') {
    return true;
  }
  return false;
}

bool collides_with_ship(Int2 ship_pos, Int2 pos) {
  for (size_t i = ship_pos.x + 2; i < ship_pos.x + 5; i++) {
    if (i == pos.x && ship_pos.y == pos.y) {
      return true;
    }
  }
  ship_pos.x += 2;
  ship_pos.y--;
  for (size_t i = ship_pos.y; i < ship_pos.y + 3; i++) {
    if (ship_pos.x == pos.x && i == pos.y) {
      return true;
    }
  }
  return false;
}

void move_projectiles(GameState *gs) {
  Int2 *ppos = NULL;
  for (size_t i = 0; i < vec_length(gs->projectiles); i++) {
    ppos = *vec_at(gs->projectiles, i);
    ppos->x += 1;
    if (!is_field_coordinate(gs, ppos->x, ppos->y)) {
      vec_remove(gs->projectiles, i);
    }
  }
}

void move_asteroids(GameState *gs) {
  Int2 *ppos = NULL;
  int s = round(gs->asteroid_speed);
  if (gs->time_step % (5 - s) == 0) {
    for (size_t i = 0; i < vec_length(gs->asteroids); i++) {
      ppos = *vec_at(gs->asteroids, i);
      ppos->x -= 1;
      if (!is_field_coordinate(gs, ppos->x, ppos->y)) {
        vec_remove(gs->asteroids, i);
      }
    }
  }
}

void move_powerups(GameState *gs) {
  Int2 *ppos = NULL;
  for (size_t i = 0; i < vec_length(gs->powerups); i++) {
    ppos = *vec_at(gs->powerups, i);
    ppos->x -= 1;
    if (!is_field_coordinate(gs, ppos->x, ppos->y)) {
      vec_remove(gs->powerups, i);
    }
  }
}

void move_mines(GameState *gs) {
  Int2 *ppos = NULL;
  for (size_t i = 0; i < vec_length(gs->mines); i++) {
    ppos = *vec_at(gs->mines, i);
    ppos->x -= 1;
    if (!is_field_coordinate(gs, ppos->x, ppos->y)) {
      vec_remove(gs->mines, i);
    }
  }
}

void move_explosions(GameState *gs) {
  Explosion *e = NULL;
  for (size_t i = 0; i < vec_length(gs->explosions); i++) {
    e = *vec_at(gs->explosions, i);
    e->age += 1;
    if (e->age > 5) {
      vec_remove(gs->explosions, i);
    }
  }
}

void spawn_asteroids(GameState *gs) {
  if (gs->time_step % 5 == 0) {
    for (size_t i = 0; i < gs->field_size.y; i++) {
      int x = rand() % 49;
      if (x == 0) {
        Int2 *ast = malloc(sizeof(Int2));
        if (ast == NULL) {
          exit(1);
        } else {
          ast->x = gs->field_size.x - 1;
          ast->y = i;
          vec_push(gs->asteroids, ast);
        }
      }
    }
  }
}

void spawn_powerups(GameState *gs) {
  int x = rand() % 199;
  if (x == 0) {
    int x = rand() % gs->field_size.y - 1;
    Int2 *pu = malloc(sizeof(Int2));
    if (pu == NULL) {
      exit(1);
    } else {
      pu->x = gs->field_size.x - 1;
      pu->y = x;
      vec_push(gs->powerups, pu);
    }
  }
}

void spawn_mines(GameState *gs) {
  if (gs->time_step % 5 == 0) {
    for (size_t i = 0; i < gs->field_size.y; i++) {
      int x = rand() % 999;
      if (x == 0) {
        Int2 *pu = malloc(sizeof(Int2));
        if (pu == NULL) {
          exit(1);
        } else {
          pu->x = gs->field_size.x - 1;
          pu->y = i;
          vec_push(gs->mines, pu);
        }
      }
    }
  }
}

void handle_projectile_asteroid_collisions(GameState *gs) {
  Int2 *ppos = NULL;
  Int2 *apos = NULL;
  for (size_t i = 0; i < vec_length(gs->asteroids); i++) {
    apos = *vec_at(gs->asteroids, i);
    for (size_t j = 0; j < vec_length(gs->projectiles); j++) {
      ppos = *vec_at(gs->projectiles, j);
      if (apos->x == ppos->x && apos->y == ppos->y) {
        gs->points += 5;
        Explosion *exp = malloc(sizeof(Explosion));
        if (exp == NULL) {
          exit(1);
        } else {
          exp->pos.x = ppos->x;
          exp->pos.y = ppos->y;
          exp->age = 0;
          vec_push(gs->explosions, exp);
        }
        vec_remove(gs->asteroids, i);
        vec_remove(gs->projectiles, j);
        gs->points++;
        apos = *vec_at(gs->projectiles, j);
      }
    }
  }
}

void handle_powerup_ship_collisions(GameState *gs) {
  Int2 *apos = NULL;
  for (size_t i = 0; i < vec_length(gs->powerups); i++) {
    apos = *vec_at(gs->powerups, i);
    if (collides_with_ship(gs->ship.pos, *apos)) {
      gs->points += 50;
      vec_remove(gs->powerups, i);
      gs->ship.powerup_time = 1000;
    }
  }
}

void handle_mines_ship_collisions(GameState *gs) {
  Int2 *mine = NULL;
  for (size_t i = 0; i < vec_length(gs->mines); i++) {
    mine = *vec_at(gs->mines, i);
    if (collides_with_ship(gs->ship.pos, *mine)) {
      gs->points -= 100;
      vec_remove(gs->mines, i);
      gs->asteroid_speed = (gs->asteroid_speed == 4) ? gs->asteroid_speed : 4;
    }
  }
}

void handle_asteroid_ship_collisions(GameState *gs) {
  Int2 *apos = NULL;
  for (size_t i = 0; i < vec_length(gs->asteroids); i++) {
    apos = *vec_at(gs->asteroids, i);
    if (collides_with_ship(gs->ship.pos, *apos)) {
      vec_remove(gs->asteroids, i);
      gs->ship.health -= 1;
    }
  }
}
