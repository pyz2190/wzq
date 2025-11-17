/*
 * Search Engine
 */

#ifndef SEARCH_H
#define SEARCH_H

#include "types.h"
#include "game_state.h"

/* Initialize search engine */
void search_init(SearchEngine* engine, GameState* gs, uint8_t my_color);

/* Find best move using iterative deepening */
int16_t search_best_move(SearchEngine* engine, int time_limit_ms);

/* Cleanup */
void search_cleanup(SearchEngine* engine);

#endif
