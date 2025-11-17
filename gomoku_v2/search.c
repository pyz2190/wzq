/*
 * Search Implementation
 * Uses Iterative Deepening + PVS (Principal Variation Search)
 */

#define _POSIX_C_SOURCE 199309L

#include "search.h"
#include "evaluator.h"
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

/* Time management */
static int64_t get_time_ms(void) {
#if defined(_WIN32) || defined(WIN32)
    return (int64_t)GetTickCount();
#elif defined(__linux__) || defined(__unix__)
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
#else
    /* Fallback for other systems */
    return (int64_t)(clock() * 1000.0 / CLOCKS_PER_SEC);
#endif
}

/* Zobrist initialization */
static void init_zobrist(SearchEngine* engine) {
    /* Simple PRNG for zobrist values */
    uint64_t seed = 0x123456789ABCDEFULL;

    uint16_t pos = 0;
    do {
        uint8_t player = 0;
        do {
            seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
            engine->zobrist[pos][player] = seed;
            player++;
        } while (player < 3);
        pos++;
    } while (pos < CELLS);
}

void search_init(SearchEngine* engine, GameState* gs, uint8_t my_color) {
    engine->state = gs;
    engine->my_color = my_color;
    engine->opp_color = (my_color == STONE_A) ? STONE_B : STONE_A;
    engine->nodes = 0;

    /* Allocate hash table */
    engine->hash_table = (HashEntry*)calloc(HASH_SIZE, sizeof(HashEntry));

    init_zobrist(engine);
}

void search_cleanup(SearchEngine* engine) {
    if (engine->hash_table) {
        free(engine->hash_table);
        engine->hash_table = NULL;
    }
}

/* Generate candidate moves using different ordering */
static int generate_moves(SearchEngine* engine, Candidate* cands, uint8_t for_player) {
    GameState* gs = engine->state;
    uint8_t enemy = (for_player == STONE_A) ? STONE_B : STONE_A;

    int count = 0;
    int16_t pos = 0;

    /* Dynamic threshold based on game phase */
    int threshold = MIN_THRESHOLD;
    if (gs->ply < 12) {
        threshold = 0;
    } else if (gs->ply >= 30) {
        threshold = MIN_THRESHOLD * 2;
    }

    /* Single-pass generation with goto loop */
    gen_loop:
    if (pos >= CELLS) goto gen_sort;
    if (count >= CELLS) goto gen_sort;

    if (gs->cells[pos] != VACANT) {
        pos++;
        goto gen_loop;
    }

    /* Evaluate attack and defense */
    int32_t atk = eval_position(gs, pos, for_player);
    int32_t def = eval_position(gs, pos, enemy);

    int32_t combined = atk * 11 + def * 9;

    if (combined < threshold) {
        pos++;
        goto gen_loop;
    }

    cands[count].pos = pos;
    cands[count].value = combined;
    count++;

    pos++;
    goto gen_loop;

    gen_sort:
    /* Simple bubble sort with goto - different from qsort */
    if (count <= 1) goto gen_done;

    int changed = 1;
    bubble_outer:
    if (!changed) goto gen_done;

    changed = 0;
    int i = 0;

    bubble_inner:
    if (i >= count - 1) goto bubble_outer;

    if (cands[i].value < cands[i + 1].value) {
        Candidate temp = cands[i];
        cands[i] = cands[i + 1];
        cands[i + 1] = temp;
        changed = 1;
    }

    i++;
    goto bubble_inner;

    gen_done:
    return (count > CAND_LIMIT) ? CAND_LIMIT : count;
}

/* Check for immediate win/block */
static int16_t find_critical_move(SearchEngine* engine, uint8_t player) {
    GameState* gs = engine->state;
    int16_t pos = 0;

    scan_positions:
    if (pos >= CELLS) return -1;

    if (gs->cells[pos] == VACANT) {
        gs_do_move(gs, pos, player);
        int win = gs_check_win(gs, pos, player);
        gs_undo_move(gs);

        if (win) return pos;
    }

    pos++;
    goto scan_positions;
}

/* Forward declarations */
static int32_t pvs_search(SearchEngine* engine, int depth, int32_t alpha, int32_t beta, int is_pv);

/* PVS (Principal Variation Search) implementation */
static int32_t pvs_search(SearchEngine* engine, int depth, int32_t alpha, int32_t beta, int is_pv) {
    /* Time check */
    if ((engine->nodes & 1023) == 0) {
        if (get_time_ms() >= engine->time_limit) {
            return eval_board(engine->state, engine->my_color, engine->opp_color);
        }
    }

    engine->nodes++;

    /* Leaf node */
    if (depth <= 0) {
        return eval_board(engine->state, engine->my_color, engine->opp_color);
    }

    Candidate candidates[CELLS];
    uint8_t current = engine->state->current_player;
    int num_moves = generate_moves(engine, candidates, current);

    if (num_moves == 0) {
        return eval_board(engine->state, engine->my_color, engine->opp_color);
    }

    /* Determine if maximizing */
    int maximizing = (current == engine->my_color) ? 1 : 0;

    if (maximizing) {
        int32_t best = INT_MIN;
        int move_idx = 0;
        int is_first = 1;

        max_loop:
        if (move_idx >= num_moves) goto max_done;

        int16_t pos = candidates[move_idx].pos;

        gs_do_move(engine->state, pos, current);

        /* Check immediate win */
        if (gs_check_win(engine->state, pos, current)) {
            gs_undo_move(engine->state);
            return WIN_SCORE - depth;
        }

        int32_t score;

        /* PVS: full window for PV, null window for others */
        if (is_first) {
            score = pvs_search(engine, depth - 1, alpha, beta, is_pv);
            is_first = 0;
        } else {
            /* Null window search */
            score = pvs_search(engine, depth - 1, alpha, alpha + 1, 0);

            /* Re-search if necessary */
            if (score > alpha && score < beta && is_pv) {
                score = pvs_search(engine, depth - 1, alpha, beta, 1);
            }
        }

        gs_undo_move(engine->state);

        best = (score > best) ? score : best;
        alpha = (score > alpha) ? score : alpha;

        if (alpha >= beta) goto max_done;

        move_idx++;
        goto max_loop;

        max_done:
        return best;
    } else {
        int32_t best = INT_MAX;
        int move_idx = 0;
        int is_first = 1;

        min_loop:
        if (move_idx >= num_moves) goto min_done;

        int16_t pos = candidates[move_idx].pos;

        gs_do_move(engine->state, pos, current);

        if (gs_check_win(engine->state, pos, current)) {
            gs_undo_move(engine->state);
            return -WIN_SCORE + depth;
        }

        int32_t score;

        if (is_first) {
            score = pvs_search(engine, depth - 1, alpha, beta, is_pv);
            is_first = 0;
        } else {
            score = pvs_search(engine, depth - 1, beta - 1, beta, 0);

            if (score > alpha && score < beta && is_pv) {
                score = pvs_search(engine, depth - 1, alpha, beta, 1);
            }
        }

        gs_undo_move(engine->state);

        best = (score < best) ? score : best;
        beta = (score < beta) ? score : beta;

        if (alpha >= beta) goto min_done;

        move_idx++;
        goto min_loop;

        min_done:
        return best;
    }
}

/* Iterative deepening search */
int16_t search_best_move(SearchEngine* engine, int time_limit_ms) {
    engine->start_time = get_time_ms();
    engine->time_limit = engine->start_time + time_limit_ms;
    engine->nodes = 0;

    /* Check critical moves first */
    int16_t win_move = find_critical_move(engine, engine->my_color);
    if (win_move >= 0) return win_move;

    int16_t block_move = find_critical_move(engine, engine->opp_color);
    if (block_move >= 0) return block_move;

    /* Generate candidates */
    Candidate candidates[CELLS];
    int num_moves = generate_moves(engine, candidates, engine->my_color);

    if (num_moves == 0) return SIZE / 2 * SIZE + SIZE / 2;  /* Center fallback */

    int16_t best_move = candidates[0].pos;

    /* Adaptive max depth based on game phase */
    int max_depth = 6;  /* Default */
    if (engine->state->ply < 10) {
        max_depth = 4;
    } else if (engine->state->ply >= 80) {
        max_depth = 8;
    }

    /* Iterative deepening loop */
    int depth = 1;

    id_loop:
    if (depth > max_depth) goto id_done;
    if (get_time_ms() >= engine->time_limit) goto id_done;

    int32_t best_score = INT_MIN;
    int32_t alpha = INT_MIN;
    int32_t beta = INT_MAX;
    int move_idx = 0;

    search_moves:
    if (move_idx >= num_moves) goto next_depth;

    int16_t pos = candidates[move_idx].pos;

    gs_do_move(engine->state, pos, engine->my_color);

    if (gs_check_win(engine->state, pos, engine->my_color)) {
        gs_undo_move(engine->state);
        return pos;  /* Immediate win */
    }

    int32_t score = pvs_search(engine, depth - 1, alpha, beta, 1);

    gs_undo_move(engine->state);

    if (score > best_score) {
        best_score = score;
        best_move = pos;
    }

    alpha = (score > alpha) ? score : alpha;

    if (get_time_ms() >= engine->time_limit) goto id_done;

    move_idx++;
    goto search_moves;

    next_depth:
    depth++;
    goto id_loop;

    id_done:
    return best_move;
}
