/*
 * Gomoku AI - Type Definitions
 * Refactored version with different architecture
 */

#ifndef TYPES_H
#define TYPES_H

#include <time.h>

/* Board configuration */
#define GRID_SIZE 12
#define TOTAL_CELLS (GRID_SIZE * GRID_SIZE)
#define CENTER_POS ((GRID_SIZE / 2) * GRID_SIZE + (GRID_SIZE / 2))

/* Cell states */
enum CellState {
    CELL_EMPTY = 0,
    CELL_BLACK = 1,
    CELL_WHITE = 2
};

/* Pattern values - different from original */
enum PatternValue {
    VAL_WIN = 100000,
    VAL_OPEN_FOUR = 20000,
    VAL_HALF_FOUR = 5000,
    VAL_OPEN_THREE = 3000,
    VAL_HALF_THREE = 800,
    VAL_OPEN_TWO = 200,
    VAL_HALF_TWO = 50,
    VAL_ONE = 10
};

/* Search configuration */
#define MAX_DEPTH 8
#define CANDIDATE_LIMIT 15
#define SCORE_THRESHOLD 5
#define TIME_LIMIT_TURN 1900
#define TIME_LIMIT_TOTAL 88000

/* Position representation - using 1D array instead of 2D */
typedef struct {
    int index;      /* 0-143 for 12x12 board */
    int score;      /* evaluation score */
} Position;

/* Game state - different structure */
typedef struct {
    int cells[TOTAL_CELLS];     /* 1D array representation */
    int move_history[TOTAL_CELLS];
    int move_number;
    int player_turn;
} GameState;

/* Search statistics */
typedef struct {
    long long nodes_searched;
    long long time_elapsed;
    int depth_reached;
} SearchStats;

/* Timer using standard C library */
typedef struct {
    clock_t start_time;
    clock_t total_used;
    int limit_turn;
    int limit_total;
} Timer;

/* Direction vectors - 8 directions instead of 4 */
static const int DIRECTION_OFFSETS[8] = {
    1,              /* East */
    GRID_SIZE,      /* South */
    GRID_SIZE + 1,  /* Southeast */
    GRID_SIZE - 1,  /* Southwest */
    -1,             /* West */
    -GRID_SIZE,     /* North */
    -GRID_SIZE - 1, /* Northwest */
    -GRID_SIZE + 1  /* Northeast */
};

/* Function pointer types for polymorphism */
typedef int (*EvalFunc)(GameState*, int);
typedef void (*MoveGenFunc)(GameState*, Position*, int*, int);

#endif
