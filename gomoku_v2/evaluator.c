/*
 * Evaluation Implementation
 * Uses precomputed pattern tables and unique control flow
 */

#include "evaluator.h"
#include <stdlib.h>

/* Pattern score lookup table [length][open_ends] */
static int32_t pattern_scores[6][3];
static int initialized = 0;

/* Initialize lookup tables */
void eval_init(void) {
    if (initialized) return;

    /* Clear table */
    uint8_t len = 0;
    do {
        uint8_t opens = 0;
        do {
            pattern_scores[len][opens] = 0;
            opens++;
        } while (opens < 3);
        len++;
    } while (len < 6);

    /* Populate pattern scores */
    pattern_scores[5][0] = WIN_SCORE;
    pattern_scores[5][1] = WIN_SCORE;
    pattern_scores[5][2] = WIN_SCORE;

    pattern_scores[4][2] = OPEN4_VAL;
    pattern_scores[4][1] = HALF4_VAL;
    pattern_scores[4][0] = 0;

    pattern_scores[3][2] = OPEN3_VAL;
    pattern_scores[3][1] = HALF3_VAL;
    pattern_scores[3][0] = 0;

    pattern_scores[2][2] = OPEN2_VAL;
    pattern_scores[2][1] = HALF2_VAL;
    pattern_scores[2][0] = 0;

    pattern_scores[1][1] = SINGLE_VAL;
    pattern_scores[1][2] = SINGLE_VAL;

    initialized = 1;
}

/* Scan pattern in one direction - using state machine approach */
typedef struct {
    int16_t position;
    int16_t delta;
    uint8_t player;
    GameState* gs;
    uint8_t count;
    uint8_t gaps;
    uint8_t blocked;
} ScanState;

static void scan_init(ScanState* ss, GameState* gs, int16_t pos, int16_t delta, uint8_t player) {
    ss->gs = gs;
    ss->position = pos;
    ss->delta = delta;
    ss->player = player;
    ss->count = 0;
    ss->gaps = 0;
    ss->blocked = 0;
}

static int scan_step(ScanState* ss) {
    int16_t p = ss->position + ss->delta;

    if (!gs_valid_pos(p)) {
        ss->blocked = 1;
        return 0;  /* Stop */
    }

    /* Check boundary wrapping */
    int r1 = gs_to_row(ss->position);
    int r2 = gs_to_row(p);

    /* Horizontal wrap check */
    if (ss->delta == 1 || ss->delta == -1) {
        if (r1 != r2) {
            ss->blocked = 1;
            return 0;
        }
    }

    uint8_t cell = gs_get(ss->gs, p);

    if (cell == ss->player) {
        ss->count++;
        ss->position = p;
        return 1;  /* Continue */
    } else if (cell == VACANT) {
        if (ss->count > 0 && ss->gaps == 0) {
            ss->gaps = 1;
            ss->position = p;
            return 1;
        } else {
            return 0;  /* Stop */
        }
    } else {
        ss->blocked = 1;
        return 0;  /* Enemy stone */
    }
}

static void scan_direction_full(GameState* gs, int16_t pos, int16_t delta, uint8_t player, uint8_t* total_count, uint8_t* open_ends) {
    ScanState forward, backward;

    /* Forward scan */
    scan_init(&forward, gs, pos, delta, player);

    scan_fw:
    if (!scan_step(&forward)) goto scan_fw_done;
    goto scan_fw;

    scan_fw_done:

    /* Backward scan */
    scan_init(&backward, gs, pos, -delta, player);

    scan_bw:
    if (!scan_step(&backward)) goto scan_bw_done;
    goto scan_bw;

    scan_bw_done:

    *total_count = 1 + forward.count + backward.count;
    *open_ends = 0;

    if (!forward.blocked) (*open_ends)++;
    if (!backward.blocked) (*open_ends)++;
}

/* Evaluate position for given player */
int32_t eval_position(GameState* gs, int16_t pos, uint8_t player) {
    if (!gs_valid_pos(pos)) return 0;
    if (gs->cells[pos] != VACANT) return 0;

    /* Direction deltas */
    int16_t deltas[4] = {1, SIZE, SIZE + 1, SIZE - 1};

    int32_t total_value = 0;
    uint8_t dir_idx = 0;

    /* Use tail recursion pattern with goto */
    eval_next_direction:
    if (dir_idx >= 4) goto eval_done;

    /* Temporarily place stone */
    gs->cells[pos] = player;

    uint8_t count, opens;
    scan_direction_full(gs, pos, deltas[dir_idx], player, &count, &opens);

    /* Remove stone */
    gs->cells[pos] = VACANT;

    /* Lookup score */
    uint8_t cnt_idx = (count > 5) ? 5 : count;
    uint8_t opn_idx = (opens > 2) ? 2 : opens;

    total_value += pattern_scores[cnt_idx][opn_idx];

    dir_idx++;
    goto eval_next_direction;

    eval_done:
    return total_value;
}

/* Board evaluation - different weighting scheme */
int32_t eval_board(GameState* gs, uint8_t my_color, uint8_t opp_color) {
    int32_t my_total = 0;
    int32_t opp_total = 0;

    int16_t pos = 0;

    /* Single pass evaluation */
    eval_loop:
    if (pos >= CELLS) goto eval_finish;

    uint8_t cell = gs->cells[pos];

    if (cell == my_color) {
        /* Temporarily remove to evaluate */
        gs->cells[pos] = VACANT;
        my_total += eval_position(gs, pos, my_color);
        gs->cells[pos] = my_color;
    } else if (cell == opp_color) {
        gs->cells[pos] = VACANT;
        opp_total += eval_position(gs, pos, opp_color);
        gs->cells[pos] = opp_color;
    }

    pos++;
    goto eval_loop;

    eval_finish:
    /* Asymmetric weighting: attack favored slightly */
    return my_total - (opp_total * 85 / 100);
}

/* Threat detection */
int eval_has_threat(GameState* gs, int16_t pos, uint8_t player, int level) {
    int32_t val = eval_position(gs, pos, player);

    switch (level) {
        case 2: return (val >= HALF4_VAL) ? 1 : 0;
        case 1: return (val >= HALF3_VAL) ? 1 : 0;
        default: return 0;
    }
}
