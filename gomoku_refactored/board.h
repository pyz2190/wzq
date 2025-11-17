/*
 * Board Management Module
 * Uses 1D array and index-based operations
 */

#ifndef BOARD_H
#define BOARD_H

#include "types.h"

/* Initialize game state */
void state_init(GameState* gs);

/* Setup initial 4 stones */
void state_setup_initial(GameState* gs);

/* Convert between 1D index and 2D coordinates */
int coord_to_index(int row, int col);
void index_to_coord(int index, int* row, int* col);

/* Boundary checking */
int is_valid_index(int index);

/* Cell operations */
int get_cell(GameState* gs, int index);
void set_cell(GameState* gs, int index, int value);
void clear_cell(GameState* gs, int index);

/* Move operations - different interface */
void make_move(GameState* gs, int index, int player);
void unmake_move(GameState* gs);

/* Victory check - using different algorithm */
int check_victory(GameState* gs, int index, int player);

/* Pattern detection at position */
int detect_pattern_dir(GameState* gs, int index, int direction, int player, int* open_ends);

#endif
