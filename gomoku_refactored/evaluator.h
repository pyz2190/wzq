/*
 * Position Evaluation Module
 * Uses lookup tables and different evaluation strategy
 */

#ifndef EVALUATOR_H
#define EVALUATOR_H

#include "types.h"

/* Initialize evaluation tables */
void eval_init_tables(void);

/* Evaluate single position for a player */
int eval_position(GameState* gs, int index, int player);

/* Evaluate entire board state */
int eval_state(GameState* gs, int my_player, int opp_player);

/* Quick threat detection */
int detect_threat_level(GameState* gs, int index, int player);

/* Pattern score lookup */
int lookup_pattern_score(int count, int open_count);

#endif
