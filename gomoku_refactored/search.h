/*
 * Search Engine Module
 * Uses iterative deepening and different control flow
 */

#ifndef SEARCH_H
#define SEARCH_H

#include "types.h"

/* Search context */
typedef struct {
    GameState* state;
    Timer* timer;
    int my_color;
    int opponent_color;
    SearchStats stats;
} SearchEngine;

/* Initialize search engine */
void search_init(SearchEngine* engine, GameState* gs, Timer* timer, int my_color);

/* Find best move - main entry point */
int search_best_move(SearchEngine* engine);

/* Minimax with alpha-beta pruning - different implementation */
int minimax_ab(SearchEngine* engine, int depth, int alpha, int beta, int maximizing);

/* Candidate generation - using incremental approach */
int generate_moves(SearchEngine* engine, Position* positions, int max_count);

/* Quick win detection */
int find_immediate_win(SearchEngine* engine, int player);

/* Depth calculation based on game phase */
int calculate_depth(GameState* gs);

/* Timer operations */
void timer_init(Timer* t);
void timer_start(Timer* t);
void timer_stop(Timer* t);
int timer_exceeded(Timer* t);
long long timer_elapsed_ms(Timer* t);

#endif
