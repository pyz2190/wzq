/*
 * Game State Management
 */

#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "types.h"

/* Initialize game state */
void gs_init(GameState* gs);

/* Place initial stones */
void gs_setup_initial(GameState* gs);

/* Make/unmake moves */
void gs_do_move(GameState* gs, int16_t pos, uint8_t player);
void gs_undo_move(GameState* gs);

/* Position utilities */
int gs_to_row(int16_t pos);
int gs_to_col(int16_t pos);
int16_t gs_to_pos(int row, int col);
int gs_valid_pos(int16_t pos);

/* Victory check */
int gs_check_win(GameState* gs, int16_t pos, uint8_t player);

/* Get cell */
uint8_t gs_get(GameState* gs, int16_t pos);

#endif
