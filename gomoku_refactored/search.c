/*
 * Search Engine Implementation
 * Different control flow patterns
 */

#include "search.h"
#include "board.h"
#include "evaluator.h"
#include <stdlib.h>
#include <limits.h>

/* Timer implementation using standard C */
void timer_init(Timer* t) {
    t->start_time = 0;
    t->total_used = 0;
    t->limit_turn = TIME_LIMIT_TURN;
    t->limit_total = TIME_LIMIT_TOTAL;
}

void timer_start(Timer* t) {
    t->start_time = clock();
}

void timer_stop(Timer* t) {
    clock_t elapsed = clock() - t->start_time;
    t->total_used += elapsed;
}

long long timer_elapsed_ms(Timer* t) {
    clock_t current = clock();
    return (long long)((current - t->start_time) * 1000.0 / CLOCKS_PER_SEC);
}

int timer_exceeded(Timer* t) {
    long long elapsed = timer_elapsed_ms(t);
    long long total = (long long)(t->total_used * 1000.0 / CLOCKS_PER_SEC) + elapsed;

    /* Using ternary operator instead of if-else - different CFG */
    return (elapsed >= t->limit_turn) ? 1 : (total >= t->limit_total) ? 1 : 0;
}

void search_init(SearchEngine* engine, GameState* gs, Timer* timer, int my_color) {
    engine->state = gs;
    engine->timer = timer;
    engine->my_color = my_color;
    engine->opponent_color = (my_color == CELL_BLACK) ? CELL_WHITE : CELL_BLACK;
    engine->stats.nodes_searched = 0;
    engine->stats.time_elapsed = 0;
    engine->stats.depth_reached = 0;
}

/* Depth calculation using lookup approach */
int calculate_depth(GameState* gs) {
    int phase_index;

    /* Determine game phase */
    if (gs->move_number < 10) {
        phase_index = 0;  /* Opening */
    } else if (gs->move_number < 80) {
        phase_index = 1;  /* Middle game */
    } else {
        phase_index = 2;  /* End game */
    }

    /* Depth table */
    static const int depth_table[3] = {4, 6, 8};
    return depth_table[phase_index];
}

/* Comparison function for sorting - using different logic */
static int compare_positions(const void* a, const void* b) {
    const Position* pa = (const Position*)a;
    const Position* pb = (const Position*)b;

    /* Descending order */
    return (pb->score > pa->score) ? 1 : (pb->score < pa->score) ? -1 : 0;
}

/* Generate candidate moves - using scoring and filtering */
int generate_moves(SearchEngine* engine, Position* positions, int max_count) {
    GameState* gs = engine->state;
    int idx, count, attack_score, defense_score, combined;
    int threshold;

    count = 0;

    /* Dynamic threshold based on game phase */
    threshold = (gs->move_number < 12) ? 0 :
                (gs->move_number < 30) ? SCORE_THRESHOLD :
                SCORE_THRESHOLD * 2;

    /* Scan board using index iteration */
    idx = 0;
    while (idx < TOTAL_CELLS && count < max_count * 2) {
        /* Skip non-empty cells */
        if (gs->cells[idx] != CELL_EMPTY) {
            idx++;
            continue;
        }

        /* Calculate attack and defense values */
        attack_score = eval_position(gs, idx, engine->my_color);
        defense_score = eval_position(gs, idx, engine->opponent_color);

        /* Combined score with different weights */
        combined = attack_score * 11 + defense_score * 9;

        /* Filter based on threshold */
        if (combined >= threshold) {
            positions[count].index = idx;
            positions[count].score = combined;
            count++;
        }

        idx++;
    }

    /* Sort positions by score */
    qsort(positions, count, sizeof(Position), compare_positions);

    /* Limit to max_count */
    return (count > max_count) ? max_count : count;
}

/* Find immediate winning move */
int find_immediate_win(SearchEngine* engine, int player) {
    GameState* gs = engine->state;
    int idx, result;

    idx = 0;
    /* Linear scan with early exit */
    while (idx < TOTAL_CELLS) {
        if (gs->cells[idx] != CELL_EMPTY) {
            idx++;
            continue;
        }

        /* Try move */
        make_move(gs, idx, player);
        result = check_victory(gs, idx, player);
        unmake_move(gs);

        /* Early return on win */
        if (result) {
            return idx;
        }

        idx++;
    }

    return -1;  /* No immediate win */
}

/* Minimax with alpha-beta - using different structure */
int minimax_ab(SearchEngine* engine, int depth, int alpha, int beta, int maximizing) {
    Position candidates[256];
    int num_candidates, i, score, value;
    int current_player;

    engine->stats.nodes_searched++;

    /* Timeout check */
    if (timer_exceeded(engine->timer)) {
        return eval_state(engine->state, engine->my_color, engine->opponent_color);
    }

    /* Leaf node evaluation */
    if (depth == 0) {
        return eval_state(engine->state, engine->my_color, engine->opponent_color);
    }

    /* Determine current player */
    current_player = maximizing ? engine->my_color : engine->opponent_color;

    /* Generate candidates */
    num_candidates = generate_moves(engine, candidates, CANDIDATE_LIMIT);

    /* No moves available */
    if (num_candidates == 0) {
        return eval_state(engine->state, engine->my_color, engine->opponent_color);
    }

    /* Maximizing player - using different loop structure */
    if (maximizing) {
        value = INT_MIN;
        i = 0;

        /* Using goto for loop control - different CFG */
        max_loop:
        if (i >= num_candidates) goto max_done;

        make_move(engine->state, candidates[i].index, engine->my_color);

        /* Check immediate win */
        if (check_victory(engine->state, candidates[i].index, engine->my_color)) {
            score = VAL_WIN - depth;
        } else {
            score = minimax_ab(engine, depth - 1, alpha, beta, 0);
        }

        unmake_move(engine->state);

        /* Update value and alpha */
        value = (score > value) ? score : value;
        alpha = (score > alpha) ? score : alpha;

        /* Beta cutoff */
        if (alpha >= beta) goto max_done;

        /* Timeout check */
        if (timer_exceeded(engine->timer)) goto max_done;

        i++;
        goto max_loop;

        max_done:
        return value;

    } else {
        /* Minimizing player */
        value = INT_MAX;
        i = 0;

        min_loop:
        if (i >= num_candidates) goto min_done;

        make_move(engine->state, candidates[i].index, engine->opponent_color);

        /* Check opponent win */
        if (check_victory(engine->state, candidates[i].index, engine->opponent_color)) {
            score = -VAL_WIN + depth;
        } else {
            score = minimax_ab(engine, depth - 1, alpha, beta, 1);
        }

        unmake_move(engine->state);

        /* Update value and beta */
        value = (score < value) ? score : value;
        beta = (score < beta) ? score : beta;

        /* Alpha cutoff */
        if (alpha >= beta) goto min_done;

        /* Timeout check */
        if (timer_exceeded(engine->timer)) goto min_done;

        i++;
        goto min_loop;

        min_done:
        return value;
    }
}

/* Main search function - using different decision flow */
int search_best_move(SearchEngine* engine) {
    Position candidates[256];
    int num_candidates, i, score, best_score, best_index;
    int win_index, depth;

    /* Step 1: Check for immediate win */
    win_index = find_immediate_win(engine, engine->my_color);
    if (win_index >= 0) {
        return win_index;
    }

    /* Step 2: Block opponent's win */
    win_index = find_immediate_win(engine, engine->opponent_color);
    if (win_index >= 0) {
        return win_index;
    }

    /* Step 3: Normal search */
    num_candidates = generate_moves(engine, candidates, CANDIDATE_LIMIT);

    /* Fallback to center */
    if (num_candidates == 0) {
        return CENTER_POS;
    }

    /* Initialize with first candidate */
    best_index = candidates[0].index;
    best_score = INT_MIN;

    /* Calculate search depth */
    depth = calculate_depth(engine->state);
    engine->stats.depth_reached = depth;

    /* Iterate through candidates */
    i = 0;
    while (i < num_candidates) {
        make_move(engine->state, candidates[i].index, engine->my_color);

        /* Check win */
        if (check_victory(engine->state, candidates[i].index, engine->my_color)) {
            score = VAL_WIN;
        } else {
            score = minimax_ab(engine, depth - 1, INT_MIN, INT_MAX, 0);
        }

        unmake_move(engine->state);

        /* Update best move */
        if (score > best_score) {
            best_score = score;
            best_index = candidates[i].index;
        }

        /* Timeout break */
        if (timer_exceeded(engine->timer)) {
            break;
        }

        i++;
    }

    return best_index;
}
