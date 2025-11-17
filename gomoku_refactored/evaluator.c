/*
 * Evaluation Implementation
 */

#include "evaluator.h"
#include "board.h"

/* Lookup table for pattern scores - different approach than switch/if-else */
static int pattern_table[6][3];  /* [count][open_ends] */

void eval_init_tables(void) {
    int i, j;

    /* Initialize all to 0 */
    for (i = 0; i < 6; i++) {
        for (j = 0; j < 3; j++) {
            pattern_table[i][j] = 0;
        }
    }

    /* Five or more */
    pattern_table[5][0] = VAL_WIN;
    pattern_table[5][1] = VAL_WIN;
    pattern_table[5][2] = VAL_WIN;

    /* Four */
    pattern_table[4][2] = VAL_OPEN_FOUR;
    pattern_table[4][1] = VAL_HALF_FOUR;

    /* Three */
    pattern_table[3][2] = VAL_OPEN_THREE;
    pattern_table[3][1] = VAL_HALF_THREE;

    /* Two */
    pattern_table[2][2] = VAL_OPEN_TWO;
    pattern_table[2][1] = VAL_HALF_TWO;

    /* One */
    pattern_table[1][0] = VAL_ONE;
    pattern_table[1][1] = VAL_ONE;
    pattern_table[1][2] = VAL_ONE;
}

int lookup_pattern_score(int count, int open_count) {
    /* Bounds checking */
    if (count < 0 || count > 5) count = 5;
    if (open_count < 0 || open_count > 2) open_count = 2;

    return pattern_table[count][open_count];
}

/* Evaluate position using table-driven approach */
int eval_position(GameState* gs, int index, int player) {
    int total_score, dir, count, opens;

    if (get_cell(gs, index) != CELL_EMPTY) return 0;

    total_score = 0;

    /* Check 4 primary directions using iteration */
    dir = 0;
    do {
        count = detect_pattern_dir(gs, index, dir, player, &opens);
        total_score += lookup_pattern_score(count, opens);
        dir++;
    } while (dir < 4);

    return total_score;
}

/* Threat level detection - using bitmask approach */
int detect_threat_level(GameState* gs, int index, int player) {
    int dir, count, opens, threat;

    threat = 0;

    /* Iterate through directions */
    for (dir = 0; dir < 4; dir++) {
        count = detect_pattern_dir(gs, index, dir, player, &opens);

        /* Use bitwise operations to encode threat level */
        if (count >= 5) {
            threat |= 0x80;  /* Immediate win */
        } else if (count == 4 && opens == 2) {
            threat |= 0x40;  /* Open four */
        } else if (count == 4 && opens == 1) {
            threat |= 0x20;  /* Half four */
        } else if (count == 3 && opens == 2) {
            threat |= 0x10;  /* Open three */
        }
    }

    return threat;
}

/* Board evaluation using accumulation pattern - different from original */
int eval_state(GameState* gs, int my_player, int opp_player) {
    int my_total, opp_total, idx;
    int orig_value, my_val, opp_val;

    my_total = 0;
    opp_total = 0;

    /* Scan all cells using different iteration pattern */
    idx = 0;
    while (idx < TOTAL_CELLS) {
        orig_value = gs->cells[idx];

        /* Evaluate my pieces */
        if (orig_value == my_player) {
            gs->cells[idx] = CELL_EMPTY;
            my_val = eval_position(gs, idx, my_player);
            gs->cells[idx] = orig_value;
            my_total += my_val;
        }

        /* Evaluate opponent pieces */
        if (orig_value == opp_player) {
            gs->cells[idx] = CELL_EMPTY;
            opp_val = eval_position(gs, idx, opp_player);
            gs->cells[idx] = orig_value;
            opp_total += opp_val;
        }

        idx++;
    }

    /* Different weighting formula - attack vs defense */
    return (my_total * 100 - opp_total * 85) / 100;
}
