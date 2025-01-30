#ifndef TETRIS_H
#define TETRIS_H

#include <stdbool.h>
#include <sys/time.h>

#define FIELD_WIDTH 10
#define FIELD_HEIGHT 20
#define NUM_PIECES 7
#define BASE_FALL_INTERVAL 1000
#define SPEED_MULTIPLIER 50

#define HIGHSCORE_TXT "highscore.txt"

typedef enum {
  GAME_STATE_START,
  GAME_STATE_MOVING,
  GAME_STATE_ATTACHING,
  GAME_STATE_GAME_OVER
} game_state_t;

typedef enum {
  USER_ACTION_DOWN,
  USER_ACTION_LEFT,
  USER_ACTION_RIGHT,
  USER_ACTION_ROTATE,
  USER_ACTION_PAUSE,
  USER_ACTION_EXIT,
  USER_ACTION_DROP,
  USER_ACTION_NONE
} user_action_t;

typedef struct {
  int color_code;
  int shape[4][4];
} piece_t;

typedef struct {
  int color;
  bool filled;
} cell_t;

typedef struct {
  cell_t field[FIELD_HEIGHT][FIELD_WIDTH];
  piece_t next;
  piece_t current;
  int current_x;
  int current_y;
  int shadow_x;
  int shadow_y;
  int score;
  int high_score;
  int level;
  int speed;
  bool pause;
  bool is_game_over;
  bool is_speeding;
} game_info_t;

typedef struct {
  game_state_t state;
  unsigned long last_update;
} game_timing_t;

void handle_input(game_info_t *game_state, user_action_t action);
game_info_t update_game_state(game_info_t *game_state, game_timing_t *timing);
void initialize_game(game_info_t *game_state, game_timing_t *timing);

#endif
