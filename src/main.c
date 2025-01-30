#include <ncurses.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "tetris.h"

void initialize_colors() {
  start_color();
  use_default_colors();
  init_pair(1, COLOR_CYAN, COLOR_CYAN);        // I
  init_pair(2, COLOR_YELLOW, COLOR_YELLOW);    // O
  init_pair(3, COLOR_MAGENTA, COLOR_MAGENTA);  // T
  init_pair(4, COLOR_WHITE, COLOR_WHITE);      // L
  init_pair(5, COLOR_BLUE, COLOR_BLUE);        // J
  init_pair(6, COLOR_GREEN, COLOR_GREEN);      // S
  init_pair(7, COLOR_RED, COLOR_RED);          // Z
  init_pair(8, COLOR_WHITE, COLOR_BLACK);      // Shadow
  init_pair(9, COLOR_CYAN, COLOR_BLACK);       // UI text
  bkgd(COLOR_PAIR(0));
}

void draw_block(int y, int x, int color) {
  if (has_colors()) {
    attron(COLOR_PAIR(color));
    mvaddch(y + 1, x * 2 + 1, ACS_CKBOARD);
    mvaddch(y + 1, x * 2 + 2, ACS_CKBOARD);
    attroff(COLOR_PAIR(color));
  } else {
    mvaddch(y + 1, x * 2 + 1, '#');
    mvaddch(y + 1, x * 2 + 2, '#');
  }
}

void draw_borders() {
  // Game borders
  attron(COLOR_PAIR(8));
  for (int y = 0; y < FIELD_HEIGHT + 2; y++) {
    mvaddch(y, 0, ACS_VLINE);
    mvaddch(y, FIELD_WIDTH * 2 + 1, ACS_VLINE);
  }
  for (int x = 0; x < FIELD_WIDTH * 2 + 2; x++) {
    mvaddch(0, x, ACS_HLINE);
    mvaddch(FIELD_HEIGHT + 1, x, ACS_HLINE);
  }
  mvaddch(0, 0, ACS_ULCORNER);
  mvaddch(0, FIELD_WIDTH * 2 + 1, ACS_URCORNER);
  mvaddch(FIELD_HEIGHT + 1, 0, ACS_LLCORNER);
  mvaddch(FIELD_HEIGHT + 1, FIELD_WIDTH * 2 + 1, ACS_LRCORNER);

  // Sidebar borders
  int sidebar_x = FIELD_WIDTH * 2 + 4;
  mvaddch(0, sidebar_x - 2, ACS_ULCORNER);
  mvaddch(0, sidebar_x + 11, ACS_URCORNER);
  mvaddch(7, sidebar_x - 2, ACS_LLCORNER);
  mvaddch(7, sidebar_x + 11, ACS_LRCORNER);
  for (int x = sidebar_x - 1; x <= sidebar_x + 10; x++) {
    mvaddch(0, x, ACS_HLINE);
    mvaddch(7, x, ACS_HLINE);
  }
  for (int y = 1; y < 7; y++) {
    mvaddch(y, sidebar_x - 2, ACS_VLINE);
    mvaddch(y, sidebar_x + 11, ACS_VLINE);
  }
  attroff(COLOR_PAIR(8));
}

void draw_sidebar(const game_info_t *game_state) {
  attron(COLOR_PAIR(9));
  mvprintw(1, FIELD_WIDTH * 2 + 7, "NEXT");
  attroff(COLOR_PAIR(9));

  int start_y = 2;
  int start_x = FIELD_WIDTH + 3;
  // Next tetromino piece
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (game_state->next.shape[i][j]) {
        draw_block(start_y + i, start_x + j - 1, game_state->next.color_code);
      }
    }
  }

  attron(COLOR_PAIR(9));
  mvprintw(9, FIELD_WIDTH * 2 + 3, "SCORE: %d", game_state->score);
  mvprintw(11, FIELD_WIDTH * 2 + 3, "HIGH: %d", game_state->high_score);
  mvprintw(13, FIELD_WIDTH * 2 + 3, "LEVEL: %d", game_state->level);
  if (game_state->pause) {
    attron(A_BOLD);
    mvprintw(16, FIELD_WIDTH * 2 + 3, "PAUSED");
    attroff(A_BOLD);
  }
  attroff(COLOR_PAIR(9));
}

void draw_piece(int y, int x, const game_info_t *game_state) {
  if (game_state->current.shape[y][x]) {
    const int i = game_state->current_y + y;
    const int j = game_state->current_x + x;
    if (i >= 0 && i < FIELD_HEIGHT && j >= 0 && j < FIELD_WIDTH) {
      draw_block(i, j, game_state->current.color_code);
    }
  }
}

void draw_shadow(const game_info_t *game_state) {
  game_info_t shadow = *game_state;
  while (!check_collision(&shadow)) shadow.current_y++;
  shadow.current_y--;

  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (shadow.current.shape[i][j]) {
        int y = shadow.current_y + i;
        int x = shadow.current_x + j;
        draw_block(y, x, 8);
      }
    }
  }
}

user_action_t get_key_action(int ch) {
  user_action_t action = USER_ACTION_NONE;
  static const struct {
    int key;
    user_action_t action;
  } map[] = {{'q', USER_ACTION_EXIT},        {KEY_LEFT, USER_ACTION_LEFT},
             {KEY_RIGHT, USER_ACTION_RIGHT}, {KEY_DOWN, USER_ACTION_DOWN},
             {KEY_UP, USER_ACTION_ROTATE},   {'p', USER_ACTION_PAUSE},
             {' ', USER_ACTION_DROP}};
  for (size_t i = 0; i < sizeof(map) / sizeof(map[0]); i++)
    if (ch == map[i].key) action = map[i].action;
  return action;
}

int main() {
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  nodelay(stdscr, TRUE);
  curs_set(0);

  if (has_colors()) initialize_colors();

  game_info_t game_state;
  game_timing_t timing;
  initialize_game(&game_state, &timing);

  while (!game_state.is_game_over) {
    user_action_t action = get_key_action(getch());
    handle_input(&game_state, action);
    update_game_state(&game_state, &timing);

    erase();

    draw_borders();
    draw_shadow(&game_state);
    // Current tetromino piece
    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 4; j++) {
        draw_piece(i, j, &game_state);
      }
    }

    // Leftover tetromino blocks
    for (int i = 0; i < FIELD_HEIGHT; i++) {
      for (int j = 0; j < FIELD_WIDTH; j++) {
        if (game_state.field[i][j].filled)
          draw_block(i, j, game_state.field[i][j].color);
      }
    }

    draw_sidebar(&game_state);
    refresh();
    napms(16);
  }

  endwin();
  printf("Game Over!\nFinal Score: %d\nHigh Score: %d\n", game_state.score,
         game_state.high_score);
  return 0;
}
