#include "tetris.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const piece_t all_pieces[NUM_PIECES] = {
    {1, {{0, 0, 0, 0}, {1, 1, 1, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}}},   // I
    {2, {{0, 0, 0, 0}, {0, 1, 1, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}}},   // O
    {3, {{0, 0, 0, 0}, {0, 1, 0, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}}},   // T
    {4, {{0, 0, 0, 0}, {0, 0, 1, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}}},   // L
    {5, {{0, 0, 0, 0}, {1, 0, 0, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}}},   // J
    {6, {{0, 0, 0, 0}, {0, 1, 1, 0}, {1, 1, 0, 0}, {0, 0, 0, 0}}},   // S
    {7, {{0, 0, 0, 0}, {1, 1, 0, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}}}};  // Z

void load_high_score(game_info_t *game_state) {
  FILE *file = fopen(HIGHSCORE_TXT, "r");
  if (file) {
    if (fscanf(file, "%d", &game_state->high_score) == 0) {
      game_state->high_score = 0;
    }

    fclose(file);
  }
}

void save_high_score(const game_info_t *game_state) {
  FILE *file = fopen(HIGHSCORE_TXT, "w");
  if (file) {
    fprintf(file, "%d", game_state->high_score);
    fclose(file);
  }
}

void compute_shadow_position(game_info_t *game_state) {
  game_info_t shadow = *game_state;
  while (1) {
    shadow.current_y++;
    if (check_collision(&shadow)) {
      shadow.current_y--;
      break;
    }
  }
  game_state->shadow_x = shadow.current_x;
  game_state->shadow_y = shadow.current_y;
}

void spawn_new_piece(game_info_t *game_state) {
  game_state->current = game_state->next;
  game_state->next = all_pieces[rand() % NUM_PIECES];
  game_state->current_x = FIELD_WIDTH / 2 - 2;
  game_state->current_y = 0;
  compute_shadow_position(game_state);
}

bool check_collision(const game_info_t *game_state) {
  bool flag = false;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (!game_state->current.shape[i][j]) continue;

      const int x = game_state->current_x + j;
      const int y = game_state->current_y + i;
      const bool out_of_bounds = x < 0 || x >= FIELD_WIDTH || y >= FIELD_HEIGHT;
      const bool collision = y >= 0 && game_state->field[y][x].filled;

      if (out_of_bounds || collision) {
        flag = true;
        break;
      }
    }
  }
  return flag;
}

void lock_piece(game_info_t *game_state) {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (!game_state->current.shape[i][j]) continue;

      const int x = game_state->current_x + j;
      const int y = game_state->current_y + i;
      if (y < 0 || y >= FIELD_HEIGHT) continue;

      game_state->field[y][x].filled = true;
      game_state->field[y][x].color = game_state->current.color_code;
    }
  }
}

int clear_completed_lines(game_info_t *game_state) {
  int lines_cleared = 0;

  for (int row = FIELD_HEIGHT - 1; row >= 0; row--) {
    bool line_full = true;

    for (int col = 0; col < FIELD_WIDTH; col++) {
      if (!game_state->field[row][col].filled) {
        line_full = false;
        break;
      }
    }

    if (line_full) {
      lines_cleared++;

      // Move down tetromino blocks
      for (int i = row; i > 0; i--) {
        memcpy(&game_state->field[i], &game_state->field[i - 1],
               sizeof(cell_t) * FIELD_WIDTH);
      }
      memset(&game_state->field[0], 0, sizeof(cell_t) * FIELD_WIDTH);
      row++;
    }
  }

  return lines_cleared;
}

void update_score(game_info_t *game_state, int lines_cleared) {
  const int points[] = {0, 100, 300, 700, 1500};
  game_state->score += points[lines_cleared] * game_state->level;
  game_state->level = (game_state->score / 600) + 1;
  game_state->level = game_state->level > 10 ? 10 : game_state->level;
}

void handle_movement(game_info_t *game_state) {
  const int speed = game_state->is_speeding ? SPEED_MULTIPLIER : 1;
  game_state->speed = BASE_FALL_INTERVAL / (game_state->level * speed);
}

void handle_action_left(game_info_t *game_state) {
  game_state->current_x--;
  if (check_collision(game_state)) game_state->current_x++;
  compute_shadow_position(game_state);
}

void handle_action_right(game_info_t *game_state) {
  game_state->current_x++;
  if (check_collision(game_state)) game_state->current_x--;
  compute_shadow_position(game_state);
}

void handle_action_down(game_info_t *game_state) {
  game_state->is_speeding = true;
  handle_movement(game_state);
}

void handle_action_rotate(game_info_t *game_state) {
  piece_t rotated = game_state->current;
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      rotated.shape[j][3 - i] = game_state->current.shape[i][j];

  const piece_t original = game_state->current;
  game_state->current = rotated;
  if (check_collision(game_state)) game_state->current = original;
  compute_shadow_position(game_state);
}

void handle_action_pause(game_info_t *game_state) {
  game_state->pause = !game_state->pause;
}

void handle_action_exit(game_info_t *game_state) {
  game_state->is_game_over = true;
}

void handle_action_drop(game_info_t *game_state) {
  while (1) {
    game_state->current_y++;
    if (check_collision(game_state)) {
      game_state->current_y--;
      break;
    }
  }
  lock_piece(game_state);
  int lines_cleared = clear_completed_lines(game_state);
  if (lines_cleared > 0) update_score(game_state, lines_cleared);

  spawn_new_piece(game_state);
  if (check_collision(game_state)) game_state->is_game_over = true;
}

void handle_action_none(game_info_t *game_state) {
  game_state->is_speeding = false;
  handle_movement(game_state);
}

void handle_input_action(game_info_t *game_state, user_action_t action) {
  static const struct {
    user_action_t action;
    action_handler handler;
  } dispatch[] = {{USER_ACTION_LEFT, handle_action_left},
                  {USER_ACTION_RIGHT, handle_action_right},
                  {USER_ACTION_DOWN, handle_action_down},
                  {USER_ACTION_ROTATE, handle_action_rotate},
                  {USER_ACTION_PAUSE, handle_action_pause},
                  {USER_ACTION_EXIT, handle_action_exit},
                  {USER_ACTION_DROP, handle_action_drop}};

  action_handler handler = handle_action_none;

  for (size_t i = 0; i < sizeof(dispatch) / sizeof(dispatch[0]); i++) {
    if (dispatch[i].action == action) {
      handler = dispatch[i].handler;
      break;
    }
  }

  handler(game_state);
}

void handle_input(game_info_t *game_state, user_action_t action) {
  if (!game_state->pause || action == USER_ACTION_PAUSE) {
    handle_input_action(game_state, action);
  }
}

void handle_state_moving(game_info_t *game_state, game_timing_t *timing) {
  game_state->current_y++;
  if (check_collision(game_state)) {
    game_state->current_y--;
    timing->state = GAME_STATE_ATTACHING;
  }
  compute_shadow_position(game_state);
}

void handle_state_attaching(game_info_t *game_state, game_timing_t *timing) {
  lock_piece(game_state);
  const int lines = clear_completed_lines(game_state);
  if (lines > 0) update_score(game_state, lines);

  // Immediate piece spawn and collision check
  spawn_new_piece(game_state);
  timing->state =
      check_collision(game_state) ? GAME_STATE_GAME_OVER : GAME_STATE_MOVING;
}

void handle_state_game_over(game_info_t *game_state, game_timing_t *timing) {
  (void)timing;
  if (game_state->score > game_state->high_score) {
    game_state->high_score = game_state->score;
    save_high_score(game_state);
  }
  game_state->is_game_over = true;
}

void handle_state_start(game_info_t *game_state, game_timing_t *timing) {
  (void)game_state;
  (void)timing;
}

void execute_state(game_info_t *game_state, game_timing_t *timing) {
  static const struct {
    game_state_t state;
    state_handler handler;
  } dispatch[] = {{GAME_STATE_START, handle_state_start},
                  {GAME_STATE_MOVING, handle_state_moving},
                  {GAME_STATE_ATTACHING, handle_state_attaching},
                  {GAME_STATE_GAME_OVER, handle_state_game_over}};

  state_handler handler = handle_state_start;

  for (size_t i = 0; i < sizeof(dispatch) / sizeof(dispatch[0]); i++) {
    if (dispatch[i].state == timing->state) {
      handler = dispatch[i].handler;
      break;
    }
  }

  handler(game_state, timing);
}

game_info_t update_game_state(game_info_t *game_state, game_timing_t *timing) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  const unsigned long current_time = tv.tv_sec * 1000 + tv.tv_usec / 1000;

  if (timing->state == GAME_STATE_START) {
    load_high_score(game_state);
    srand(time(NULL));
    game_state->next = all_pieces[rand() % NUM_PIECES];
    spawn_new_piece(game_state);
    timing->state = GAME_STATE_MOVING;
    timing->last_update = current_time;
  }

  bool is_stopped = game_state->pause || game_state->is_game_over;
  bool less_than_interval =
      current_time - timing->last_update <= (unsigned long)game_state->speed;

  if (!is_stopped && !less_than_interval) {
    execute_state(game_state, timing);
    timing->last_update = current_time;
  }

  return *game_state;
}

void initialize_game(game_info_t *game_state, game_timing_t *timing) {
  memset(game_state, 0, sizeof(game_info_t));
  memset(timing, 0, sizeof(game_timing_t));

  game_state->next = all_pieces[0];
  spawn_new_piece(game_state);

  timing->state = GAME_STATE_START;
  game_state->level = 1;
  game_state->speed = BASE_FALL_INTERVAL;
}
