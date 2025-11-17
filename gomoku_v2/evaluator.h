/*
 * Position Evaluation Engine
 */

#ifndef EVALUATOR_H
#define EVALUATOR_H

#include "types.h"
#include "game_state.h"

/* Initialize evaluation tables */
void eval_init(void);

/* Evaluate single position */
int32_t eval_position(GameState* gs, int16_t pos, uint8_t player);

/* Evaluate entire board */
int32_t eval_board(GameState* gs, uint8_t my_color, uint8_t opp_color);

/* Check for immediate threats */
int eval_has_threat(GameState* gs, int16_t pos, uint8_t player, int level);

#endif
