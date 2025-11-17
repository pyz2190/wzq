/*
 * Gomoku AI - Type Definitions
 * Architecture: State Machine + Incremental Evaluation + Iterative Deepening
 * Author: Original Design
 * Date: 2025-11-17
 */

#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

/* Board dimensions */
#define SIZE 12
#define CELLS (SIZE * SIZE)
#define MAX_MOVES CELLS

/* Cell states */
#define VACANT 0
#define STONE_A 1
#define STONE_B 2

/* Evaluation constants - different values from reference */
#define WIN_SCORE 100000
#define OPEN4_VAL 20000
#define HALF4_VAL 5000
#define OPEN3_VAL 3000
#define HALF3_VAL 800
#define OPEN2_VAL 200
#define HALF2_VAL 50
#define SINGLE_VAL 10

/* Search parameters */
#define MAX_PLY 12
#define CAND_LIMIT 15
#define MIN_THRESHOLD 5

/* Time limits (ms) */
#define MOVE_TIME 1900
#define TOTAL_TIME 88000

/* Zobrist hashing */
#define HASH_SIZE 1048576  /* 2^20 */

/* Direction encoding: 0=horizontal, 1=vertical, 2=diag1, 3=diag2 */
typedef enum {
    DIR_HORIZ = 0,
    DIR_VERT = 1,
    DIR_DIAG1 = 2,
    DIR_DIAG2 = 3,
    DIR_COUNT = 4
} Direction;

/* Game state representation using 1D array */
typedef struct {
    uint8_t cells[CELLS];
    int16_t moves[MAX_MOVES];  /* Move history */
    uint16_t ply;              /* Current move number */
    uint8_t current_player;
    uint64_t hash;             /* Zobrist hash */
} GameState;

/* Position candidate for move ordering */
typedef struct {
    int16_t pos;
    int32_t value;
} Candidate;

/* Evaluation cache entry */
typedef struct {
    uint64_t key;
    int32_t score;
    uint8_t depth;
    uint8_t flag;  /* 0=exact, 1=lower, 2=upper */
} HashEntry;

/* Search engine context */
typedef struct {
    GameState* state;
    HashEntry* hash_table;
    uint64_t zobrist[CELLS][3];  /* [pos][player] */
    uint8_t my_color;
    uint8_t opp_color;
    int64_t start_time;
    int64_t time_limit;
    uint32_t nodes;
} SearchEngine;

/* Command processor state machine states */
typedef enum {
    ST_INIT,
    ST_READY,
    ST_THINKING,
    ST_DONE
} MachineState;

/* Pattern structure for evaluation */
typedef struct {
    uint8_t length;
    uint8_t gaps;
    uint8_t blocks;
} Pattern;

#endif /* TYPES_H */
