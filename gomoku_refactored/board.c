/*
 * Board Management Implementation
 */

#include "board.h"
#include <string.h>

void state_init(GameState* gs) {
    int i;
    i = 0;
    /* Using while loop instead of for - different CFG */
    while (i < TOTAL_CELLS) {
        gs->cells[i] = CELL_EMPTY;
        i++;
    }
    gs->move_number = 0;
    gs->player_turn = CELL_BLACK;
}

void state_setup_initial(GameState* gs) {
    /* Initial 4 stones in cross pattern */
    int center = CENTER_POS;
    set_cell(gs, center - GRID_SIZE - 1, CELL_WHITE);
    set_cell(gs, center, CELL_WHITE);
    set_cell(gs, center - GRID_SIZE, CELL_BLACK);
    set_cell(gs, center - 1, CELL_BLACK);
    gs->move_number = 4;
}

int coord_to_index(int row, int col) {
    return row * GRID_SIZE + col;
}

void index_to_coord(int index, int* row, int* col) {
    *row = index / GRID_SIZE;
    *col = index % GRID_SIZE;
}

int is_valid_index(int index) {
    /* Simplified boundary check */
    return (index >= 0 && index < TOTAL_CELLS);
}

int get_cell(GameState* gs, int index) {
    return is_valid_index(index) ? gs->cells[index] : -1;
}

void set_cell(GameState* gs, int index, int value) {
    if (is_valid_index(index)) {
        gs->cells[index] = value;
    }
}

void clear_cell(GameState* gs, int index) {
    if (is_valid_index(index)) {
        gs->cells[index] = CELL_EMPTY;
    }
}

void make_move(GameState* gs, int index, int player) {
    if (!is_valid_index(index)) return;
    if (gs->cells[index] != CELL_EMPTY) return;

    gs->cells[index] = player;
    gs->move_history[gs->move_number] = index;
    gs->move_number++;
    gs->player_turn = (player == CELL_BLACK) ? CELL_WHITE : CELL_BLACK;
}

void unmake_move(GameState* gs) {
    if (gs->move_number <= 0) return;

    gs->move_number--;
    int index = gs->move_history[gs->move_number];
    int player = gs->cells[index];
    gs->cells[index] = CELL_EMPTY;
    gs->player_turn = player;
}

/* Victory check using state machine approach - different CFG */
int check_victory(GameState* gs, int index, int player) {
    int dir_idx, pos, count, offset;
    int row, col, r, c;

    if (get_cell(gs, index) != player) return 0;

    index_to_coord(index, &row, &col);

    /* Check 4 direction pairs (not 8 directions) */
    dir_idx = 0;
    while (dir_idx < 4) {
        count = 1;
        offset = DIRECTION_OFFSETS[dir_idx];

        /* Forward scan */
        pos = index + offset;
        while (1) {
            if (!is_valid_index(pos)) break;
            index_to_coord(pos, &r, &c);

            /* Check if crossed boundary */
            if (dir_idx == 0 && c < col) break;  /* East */
            if (dir_idx == 4 && c > col) break;  /* West */

            if (gs->cells[pos] == player) {
                count++;
                pos += offset;
            } else {
                break;
            }
        }

        /* Backward scan */
        offset = DIRECTION_OFFSETS[dir_idx + 4];
        pos = index + offset;
        while (1) {
            if (!is_valid_index(pos)) break;
            index_to_coord(pos, &r, &c);

            /* Check if crossed boundary */
            if (dir_idx == 0 && c > col) break;
            if (dir_idx == 4 && c < col) break;

            if (gs->cells[pos] == player) {
                count++;
                pos += offset;
            } else {
                break;
            }
        }

        if (count >= 5) return 1;

        dir_idx++;
    }

    return 0;
}

/* Detect pattern in one direction - returns count and open ends */
int detect_pattern_dir(GameState* gs, int index, int direction, int player, int* open_ends) {
    int count, pos, offset, row, col, r, c;
    int left_open, right_open;

    count = 1;
    left_open = 0;
    right_open = 0;

    index_to_coord(index, &row, &col);
    offset = DIRECTION_OFFSETS[direction];

    /* Scan forward */
    pos = index + offset;
    while (1) {
        if (!is_valid_index(pos)) break;
        index_to_coord(pos, &r, &c);

        /* Boundary wrap check */
        if (direction == 0 && c < col) break;
        if (direction == 1 && r < row) break;

        if (gs->cells[pos] == player) {
            count++;
            pos += offset;
            if (count >= 6) break;  /* No need to count beyond 6 */
        } else if (gs->cells[pos] == CELL_EMPTY) {
            right_open = 1;
            break;
        } else {
            break;
        }
    }

    /* Scan backward */
    offset = DIRECTION_OFFSETS[direction + 4];
    pos = index + offset;
    while (1) {
        if (!is_valid_index(pos)) break;
        index_to_coord(pos, &r, &c);

        /* Boundary wrap check */
        if (direction == 0 && c > col) break;
        if (direction == 1 && r > row) break;

        if (gs->cells[pos] == player) {
            count++;
            pos += offset;
            if (count >= 6) break;
        } else if (gs->cells[pos] == CELL_EMPTY) {
            left_open = 1;
            break;
        } else {
            break;
        }
    }

    *open_ends = left_open + right_open;
    return count;
}
