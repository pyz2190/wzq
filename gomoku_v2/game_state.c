/*
 * Game State Implementation
 * Uses 1D array and unique control flow patterns
 */

#include "game_state.h"
#include <string.h>

/* Initialize to empty state */
void gs_init(GameState* gs) {
    uint16_t i = 0;

    /* Use do-while instead of for */
    do {
        gs->cells[i] = VACANT;
        gs->moves[i] = -1;
        i++;
    } while (i < CELLS);

    gs->ply = 0;
    gs->current_player = STONE_A;
    gs->hash = 0;
}

/* Setup initial 4 stones */
void gs_setup_initial(GameState* gs) {
    /* Different layout than reference */
    int16_t positions[4] = {
        gs_to_pos(5, 5),
        gs_to_pos(6, 6),
        gs_to_pos(5, 6),
        gs_to_pos(6, 5)
    };

    uint8_t players[4] = {STONE_B, STONE_B, STONE_A, STONE_A};

    uint8_t idx = 0;

    /* Use goto-based loop */
    place_loop:
    if (idx >= 4) goto place_done;

    gs->cells[positions[idx]] = players[idx];
    gs->moves[gs->ply++] = positions[idx];

    idx++;
    goto place_loop;

    place_done:
    gs->current_player = STONE_A;
}

/* Make a move */
void gs_do_move(GameState* gs, int16_t pos, uint8_t player) {
    gs->cells[pos] = player;
    gs->moves[gs->ply] = pos;
    gs->ply++;
    gs->current_player = (player == STONE_A) ? STONE_B : STONE_A;
}

/* Undo last move */
void gs_undo_move(GameState* gs) {
    if (gs->ply == 0) return;

    gs->ply--;
    int16_t pos = gs->moves[gs->ply];
    gs->cells[pos] = VACANT;

    uint8_t prev = gs->current_player;
    gs->current_player = (prev == STONE_A) ? STONE_B : STONE_A;
}

/* Position conversion */
int gs_to_row(int16_t pos) {
    return pos / SIZE;
}

int gs_to_col(int16_t pos) {
    return pos % SIZE;
}

int16_t gs_to_pos(int row, int col) {
    return row * SIZE + col;
}

int gs_valid_pos(int16_t pos) {
    return (pos >= 0 && pos < CELLS) ? 1 : 0;
}

uint8_t gs_get(GameState* gs, int16_t pos) {
    return gs_valid_pos(pos) ? gs->cells[pos] : STONE_B + 1;
}

/* Victory check - using function pointer array */
typedef int (*DirectionChecker)(GameState*, int16_t, uint8_t, int16_t);

static int check_direction(GameState* gs, int16_t pos, uint8_t player, int16_t delta) {
    int16_t count = 1;
    int16_t p;

    /* Forward scan */
    p = pos + delta;
    scan_forward:
    if (!gs_valid_pos(p)) goto scan_backward;

    /* Boundary wrap detection */
    {
        int row1 = gs_to_row(pos);
        int row2 = gs_to_row(p);

        /* Check for wrap-around */
        if (delta == 1 || delta == -1) {
            if (row1 != row2) goto scan_backward;
        }
    }

    if (gs->cells[p] != player) goto scan_backward;

    count++;
    p += delta;
    goto scan_forward;

    scan_backward:
    p = pos - delta;

    scan_back_loop:
    if (!gs_valid_pos(p)) goto check_count;

    {
        int row1 = gs_to_row(pos);
        int row2 = gs_to_row(p);

        if (delta == 1 || delta == -1) {
            if (row1 != row2) goto check_count;
        }
    }

    if (gs->cells[p] != player) goto check_count;

    count++;
    p -= delta;
    goto scan_back_loop;

    check_count:
    return count;
}

int gs_check_win(GameState* gs, int16_t pos, uint8_t player) {
    if (!gs_valid_pos(pos)) return 0;
    if (gs->cells[pos] != player) return 0;

    /* Direction deltas: horizontal, vertical, diag1, diag2 */
    int16_t deltas[4] = {1, SIZE, SIZE + 1, SIZE - 1};

    uint8_t dir = 0;

    /* Use recursive-style check */
    check_next_dir:
    if (dir >= 4) return 0;

    int cnt = check_direction(gs, pos, player, deltas[dir]);
    if (cnt >= 5) return 1;

    dir++;
    goto check_next_dir;
}
