#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "tetris.h"

static const piece_t all_pieces[NUM_PIECES] = {
    {1, {{0, 0, 0, 0}, {1, 1, 1, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}}},
    {2, {{0, 0, 0, 0}, {0, 1, 1, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}}},
    {3, {{0, 0, 0, 0}, {0, 1, 0, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}}},
    {4, {{0, 0, 0, 0}, {0, 0, 1, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}}},
    {5, {{0, 0, 0, 0}, {1, 0, 0, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}}},
    {6, {{0, 0, 0, 0}, {0, 1, 1, 0}, {1, 1, 0, 0}, {0, 0, 0, 0}}},
    {7, {{0, 0, 0, 0}, {1, 1, 0, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}}}};

START_TEST(test_load_save_high_score) {
  game_info_t game_state;
  memset(&game_state, 0, sizeof(game_info_t));

  game_state.high_score = 500;
  save_high_score(&game_state);

  load_high_score(&game_state);
  ck_assert_int_eq(game_state.high_score, 500);

  remove("highscore.txt");
}
END_TEST

START_TEST(test_spawn_new_piece) {
  game_info_t game_state;
  game_timing_t timing;
  memset(&game_state, 0, sizeof(game_info_t));
  initialize_game(&game_state, &timing);

  piece_t initial_next = game_state.next;

  spawn_new_piece(&game_state);

  ck_assert_int_eq(game_state.current.color_code, initial_next.color_code);
  ck_assert_int_eq(game_state.current_x, FIELD_WIDTH / 2 - 2);
  ck_assert_int_eq(game_state.current_y, 0);
}
END_TEST

START_TEST(test_check_collision) {
  game_info_t game_state;
  memset(&game_state, 0, sizeof(game_info_t));
  initialize_game(&game_state, (game_timing_t[]){0});

  ck_assert(!check_collision(&game_state));

  game_state.current_x = -1;
  ck_assert(check_collision(&game_state));
}
END_TEST

START_TEST(test_lock_piece_and_clear_lines) {
  game_info_t game_state;
  memset(&game_state, 0, sizeof(game_info_t));
  initialize_game(&game_state, (game_timing_t[]){0});

  // Lock an I-piece at the bottom (occupying columns 3-6 in row 19)
  game_state.current = all_pieces[0];
  game_state.current_x = FIELD_WIDTH / 2 - 2;  // x = 3
  game_state.current_y = FIELD_HEIGHT - 2;     // y = 18
  lock_piece(&game_state);

  // Manually fill the remaining columns in row 19 to complete the line
  for (int col = 0; col < FIELD_WIDTH; col++) {
    if (col < 3 || col > 6) {
      game_state.field[FIELD_HEIGHT - 1][col].filled = true;
    }
  }

  // Verify the line is cleared
  int lines_cleared = clear_completed_lines(&game_state);
  update_score(&game_state, lines_cleared);

  ck_assert_int_eq(lines_cleared, 1);
  ck_assert_int_eq(game_state.score, 100);
}
END_TEST

START_TEST(test_handle_action_left_right) {
  game_info_t game_state;
  memset(&game_state, 0, sizeof(game_info_t));
  initialize_game(&game_state, (game_timing_t[]){0});
  // game_state->current_x = FIELD_WIDTH / 2 - 2;
  // game_state->current_y = 0;
  game_state.current = all_pieces[1];  // O

  for (int current_x = FIELD_WIDTH / 2 - 2; current_x > -1; current_x--) {
    handle_input_action(&game_state, USER_ACTION_LEFT);
    ck_assert_int_eq(game_state.current_x, current_x - 1);
  }
  handle_input_action(&game_state, USER_ACTION_LEFT);
  ck_assert_int_eq(game_state.current_x, -1);
  handle_input_action(&game_state, USER_ACTION_LEFT);
  ck_assert_int_eq(game_state.current_x, -1);

  for (int current_x = -1; current_x < FIELD_WIDTH - 3; current_x++) {
    handle_input_action(&game_state, USER_ACTION_RIGHT);
    ck_assert_int_eq(game_state.current_x, current_x + 1);
  }
  handle_input_action(&game_state, USER_ACTION_RIGHT);
  ck_assert_int_eq(game_state.current_x, FIELD_WIDTH - 3);
  handle_input_action(&game_state, USER_ACTION_RIGHT);
  ck_assert_int_eq(game_state.current_x, FIELD_WIDTH - 3);
}
END_TEST

START_TEST(test_handle_movement) {
  game_info_t game_state;
  memset(&game_state, 0, sizeof(game_info_t));
  initialize_game(&game_state, (game_timing_t[]){0});

  handle_input_action(&game_state, USER_ACTION_NONE);
  ck_assert(!game_state.is_speeding);

  handle_movement(&game_state);
  ck_assert_int_eq(game_state.speed, BASE_FALL_INTERVAL / game_state.level);

  handle_input_action(&game_state, USER_ACTION_DOWN);
  ck_assert_int_eq(game_state.speed,
                   BASE_FALL_INTERVAL / (game_state.level * SPEED_MULTIPLIER));
}
END_TEST

START_TEST(test_handle_action_rotate) {
  game_info_t game_state;
  memset(&game_state, 0, sizeof(game_info_t));
  initialize_game(&game_state, (game_timing_t[]){0});
  game_state.current = all_pieces[0];  // I-piece
  game_state.current_x = FIELD_WIDTH / 2 - 2;
  game_state.current_y = 0;

  for (int i = 0; i < 4; i++) {
    ck_assert(game_state.current.shape[1][i]);
  }
  handle_input_action(&game_state, USER_ACTION_ROTATE);

  for (int i = 0; i < 4; i++) {
    ck_assert(game_state.current.shape[i][2]);
  }
}
END_TEST

START_TEST(test_handle_action_destroy) {
  game_info_t game_state;
  memset(&game_state, 0, sizeof(game_info_t));
  initialize_game(&game_state, (game_timing_t[]){0});

  handle_input(&game_state, USER_ACTION_PAUSE);
  ck_assert(game_state.pause);
  handle_input(&game_state, USER_ACTION_PAUSE);
  ck_assert(!game_state.pause);

  handle_input(&game_state, USER_ACTION_EXIT);
  ck_assert(game_state.is_game_over);
  handle_input(&game_state, USER_ACTION_EXIT);
  ck_assert(game_state.is_game_over);
}
END_TEST

START_TEST(test_handle_input_drop) {
  game_info_t game_state;
  memset(&game_state, 0, sizeof(game_info_t));
  initialize_game(&game_state, (game_timing_t[]){0});

  // Force the current piece to be an O-block (2x2)
  game_state.current = all_pieces[1];          // O-piece
  game_state.current_x = FIELD_WIDTH / 2 - 2;  // x = 3
  game_state.current_y = 0;

  // Perform hard drop
  handle_input_action(&game_state, USER_ACTION_DROP);

  // Verify the O-piece is locked at the bottom (rows 18-19, columns 4-5)
  for (int i = 18; i < 20; i++) {
    for (int j = 4; j < 6; j++) {
      ck_assert(game_state.field[i][j].filled);
    }
  }
}
END_TEST

START_TEST(test_update_state) {
  game_info_t game_state;
  game_timing_t timing;
  memset(&game_state, 0, sizeof(game_info_t));
  initialize_game(&game_state, &timing);

  timing.state = GAME_STATE_START;
  update_game_state(&game_state, &timing);
  ck_assert(game_state.current.color_code);
  ck_assert(game_state.next.color_code);

  game_state.current = all_pieces[0];
  game_state.current_x = FIELD_WIDTH / 2 - 2;  // x = 3
  game_state.current_y = FIELD_HEIGHT - 2;     // y = 18

  // GAME_STATE_MOVING after GAME_STATE_START
  for (int current_y = game_state.current_y; current_y < FIELD_HEIGHT - 3;
       current_y++) {
    timing.last_update += 1001;
    update_game_state(&game_state, &timing);
    ck_assert_int_eq(game_state.current_y, current_y + 1);
  }
  timing.last_update += 1001;
  update_game_state(&game_state, &timing);
  ck_assert_int_eq(game_state.current_y, FIELD_HEIGHT - 2);

  // GAME_STATE_ATTACHING after GAME_STATE_MOVING
  timing.last_update += 1001;
  for (int col = 0; col < FIELD_WIDTH; col++) {
    if (col < 3 || col > 6) {
      game_state.field[FIELD_HEIGHT - 1][col].filled = true;
    }
  }
  update_game_state(&game_state, &timing);
  ck_assert_int_eq(game_state.score, 100);

  timing.last_update += 1001;
  timing.state = GAME_STATE_GAME_OVER;
  game_state.high_score = 0;
  update_game_state(&game_state, &timing);
  ck_assert(game_state.is_game_over);
  remove("highscore.txt");
}
END_TEST

Suite *tetris_suite(void) {
  Suite *suite = suite_create("Tetris");
  TCase *tc_core = tcase_create("Core");
  tcase_add_test(tc_core, test_load_save_high_score);
  tcase_add_test(tc_core, test_spawn_new_piece);
  tcase_add_test(tc_core, test_check_collision);
  tcase_add_test(tc_core, test_lock_piece_and_clear_lines);
  tcase_add_test(tc_core, test_handle_action_left_right);
  tcase_add_test(tc_core, test_handle_movement);
  tcase_add_test(tc_core, test_handle_action_destroy);
  tcase_add_test(tc_core, test_handle_action_rotate);
  tcase_add_test(tc_core, test_handle_input_drop);
  tcase_add_test(tc_core, test_update_state);
  suite_add_tcase(suite, tc_core);

  return suite;
}

int main(void) {
  int num_failed;
  Suite *suite = tetris_suite();
  SRunner *suite_runner = srunner_create(suite);

  srunner_set_fork_status(suite_runner, CK_NOFORK);
  srunner_run_all(suite_runner, CK_NORMAL);
  num_failed = srunner_ntests_failed(suite_runner);
  srunner_free(suite_runner);

  return (num_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
