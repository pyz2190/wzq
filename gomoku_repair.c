/*
 * Gomoku AI - Redesigned Architecture (Single File Version)
 *
 * Architecture: State Machine + Iterative Deepening + PVS
 * CFG Features: goto loops, table-driven dispatch, pattern lookup tables
 *
 * Author: Original Design
 * Date: 2025-11-17
 */

#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#ifdef _WIN32
#include <windows.h>
#endif

/* ==================== TYPE DEFINITIONS ==================== */

/* Board dimensions */
#define SIZE 12
#define CELLS (SIZE * SIZE)
#define MAX_MOVES CELLS

/* Cell states */
#define VACANT 0
#define STONE_A 1
#define STONE_B 2

/* Evaluation constants */
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

/* Hash table size */
#define HASH_SIZE 1048576

/* Direction encoding */
typedef enum {
    DIR_HORIZ = 0,
    DIR_VERT = 1,
    DIR_DIAG1 = 2,
    DIR_DIAG2 = 3,
    DIR_COUNT = 4
} Direction;

/* Game state representation */
typedef struct {
    uint8_t cells[CELLS];
    int16_t moves[MAX_MOVES];
    uint16_t ply;
    uint8_t current_player;
    uint64_t hash;
} GameState;

/* Position candidate */
typedef struct {
    int16_t pos;
    int32_t value;
} Candidate;

/* Hash entry */
typedef struct {
    uint64_t key;
    int32_t score;
    uint8_t depth;
    uint8_t flag;
} HashEntry;

/* Search engine */
typedef struct {
    GameState* state;
    HashEntry* hash_table;
    uint64_t zobrist[CELLS][3];
    uint8_t my_color;
    uint8_t opp_color;
    int64_t start_time;
    int64_t time_limit;
    uint32_t nodes;
} SearchEngine;

/* State machine states */
typedef enum {
    ST_INIT,
    ST_READY,
    ST_THINKING,
    ST_DONE
} MachineState;

/* Pattern structure */
typedef struct {
    uint8_t length;
    uint8_t gaps;
    uint8_t blocks;
} Pattern;

/* ==================== GAME STATE MODULE ==================== */

/* Position utilities */
static inline int gs_to_row(int16_t pos) {
    return pos / SIZE;
}

static inline int gs_to_col(int16_t pos) {
    return pos % SIZE;
}

static inline int16_t gs_to_pos(int row, int col) {
    return row * SIZE + col;
}

static inline int gs_valid_pos(int16_t pos) {
    return (pos >= 0 && pos < CELLS) ? 1 : 0;
}

static inline uint8_t gs_get(GameState* gs, int16_t pos) {
    return gs_valid_pos(pos) ? gs->cells[pos] : STONE_B + 1;
}

/* Initialize game state */
static void gs_init(GameState* gs) {
    uint16_t i = 0;

    do {
        gs->cells[i] = VACANT;
        gs->moves[i] = -1;
        i++;
    } while (i < CELLS);

    gs->ply = 0;
    gs->current_player = STONE_A;
    gs->hash = 0;
}

/* Setup initial 4 stones */
static void gs_setup_initial(GameState* gs) {
    int16_t positions[4] = {
        gs_to_pos(5, 5),
        gs_to_pos(6, 6),
        gs_to_pos(5, 6),
        gs_to_pos(6, 5)
    };

    uint8_t players[4] = {STONE_B, STONE_B, STONE_A, STONE_A};

    uint8_t idx = 0;

    place_loop:
    if (idx >= 4) goto place_done;

    gs->cells[positions[idx]] = players[idx];
    gs->moves[gs->ply++] = positions[idx];

    idx++;
    goto place_loop;

    place_done:
    gs->current_player = STONE_A;
}

/* Make a move */
static void gs_do_move(GameState* gs, int16_t pos, uint8_t player) {
    gs->cells[pos] = player;
    gs->moves[gs->ply] = pos;
    gs->ply++;
    gs->current_player = (player == STONE_A) ? STONE_B : STONE_A;
}

/* Undo last move */
static void gs_undo_move(GameState* gs) {
    if (gs->ply == 0) return;

    gs->ply--;
    int16_t pos = gs->moves[gs->ply];
    gs->cells[pos] = VACANT;

    uint8_t prev = gs->current_player;
    gs->current_player = (prev == STONE_A) ? STONE_B : STONE_A;
}

/* Check direction for consecutive stones */
static int check_direction(GameState* gs, int16_t pos, uint8_t player, int16_t delta) {
    int16_t count = 1;
    int16_t p;

    /* Forward scan */
    p = pos + delta;
    scan_forward:
    if (!gs_valid_pos(p)) goto scan_backward;

    {
        int row1 = gs_to_row(pos);
        int row2 = gs_to_row(p);

        if (delta == 1 || delta == -1) {
            if (row1 != row2) goto scan_backward;
        }
    }

    if (gs->cells[p] != player) goto scan_backward;

    count++;
    p += delta;
    goto scan_forward;

    scan_backward:
    p = pos - delta;

    scan_back_loop:
    if (!gs_valid_pos(p)) goto check_count;

    {
        int row1 = gs_to_row(pos);
        int row2 = gs_to_row(p);

        if (delta == 1 || delta == -1) {
            if (row1 != row2) goto check_count;
        }
    }

    if (gs->cells[p] != player) goto check_count;

    count++;
    p -= delta;
    goto scan_back_loop;

    check_count:
    return count;
}

/* Check for victory */
static int gs_check_win(GameState* gs, int16_t pos, uint8_t player) {
    if (!gs_valid_pos(pos)) return 0;
    if (gs->cells[pos] != player) return 0;

    int16_t deltas[4] = {1, SIZE, SIZE + 1, SIZE - 1};

    uint8_t dir = 0;

    check_next_dir:
    if (dir >= 4) return 0;

    int cnt = check_direction(gs, pos, player, deltas[dir]);
    if (cnt >= 5) return 1;

    dir++;
    goto check_next_dir;
}

/* ==================== EVALUATOR MODULE ==================== */

static int32_t pattern_scores[6][3];
static int eval_initialized = 0;

/* Initialize lookup tables */
static void eval_init(void) {
    if (eval_initialized) return;

    uint8_t len = 0;
    do {
        uint8_t opens = 0;
        do {
            pattern_scores[len][opens] = 0;
            opens++;
        } while (opens < 3);
        len++;
    } while (len < 6);

    pattern_scores[5][0] = WIN_SCORE;
    pattern_scores[5][1] = WIN_SCORE;
    pattern_scores[5][2] = WIN_SCORE;

    pattern_scores[4][2] = OPEN4_VAL;
    pattern_scores[4][1] = HALF4_VAL;
    pattern_scores[4][0] = 0;

    pattern_scores[3][2] = OPEN3_VAL;
    pattern_scores[3][1] = HALF3_VAL;
    pattern_scores[3][0] = 0;

    pattern_scores[2][2] = OPEN2_VAL;
    pattern_scores[2][1] = HALF2_VAL;
    pattern_scores[2][0] = 0;

    pattern_scores[1][1] = SINGLE_VAL;
    pattern_scores[1][2] = SINGLE_VAL;

    eval_initialized = 1;
}

/* Pattern scanner state */
typedef struct {
    int16_t position;
    int16_t delta;
    uint8_t player;
    GameState* gs;
    uint8_t count;
    uint8_t gaps;
    uint8_t blocked;
} ScanState;

static void scan_init(ScanState* ss, GameState* gs, int16_t pos, int16_t delta, uint8_t player) {
    ss->gs = gs;
    ss->position = pos;
    ss->delta = delta;
    ss->player = player;
    ss->count = 0;
    ss->gaps = 0;
    ss->blocked = 0;
}

static int scan_step(ScanState* ss) {
    int16_t p = ss->position + ss->delta;

    if (!gs_valid_pos(p)) {
        ss->blocked = 1;
        return 0;
    }

    int r1 = gs_to_row(ss->position);
    int r2 = gs_to_row(p);

    if (ss->delta == 1 || ss->delta == -1) {
        if (r1 != r2) {
            ss->blocked = 1;
            return 0;
        }
    }

    uint8_t cell = gs_get(ss->gs, p);

    if (cell == ss->player) {
        ss->count++;
        ss->position = p;
        return 1;
    } else if (cell == VACANT) {
        if (ss->count > 0 && ss->gaps == 0) {
            ss->gaps = 1;
            ss->position = p;
            return 1;
        } else {
            return 0;
        }
    } else {
        ss->blocked = 1;
        return 0;
    }
}

static void scan_direction_full(GameState* gs, int16_t pos, int16_t delta, uint8_t player, uint8_t* total_count, uint8_t* open_ends) {
    ScanState forward, backward;

    scan_init(&forward, gs, pos, delta, player);

    scan_fw:
    if (!scan_step(&forward)) goto scan_fw_done;
    goto scan_fw;

    scan_fw_done:

    scan_init(&backward, gs, pos, -delta, player);

    scan_bw:
    if (!scan_step(&backward)) goto scan_bw_done;
    goto scan_bw;

    scan_bw_done:

    *total_count = 1 + forward.count + backward.count;
    *open_ends = 0;

    if (!forward.blocked) (*open_ends)++;
    if (!backward.blocked) (*open_ends)++;
}

/* Evaluate position */
static int32_t eval_position(GameState* gs, int16_t pos, uint8_t player) {
    if (!gs_valid_pos(pos)) return 0;
    if (gs->cells[pos] != VACANT) return 0;

    int16_t deltas[4] = {1, SIZE, SIZE + 1, SIZE - 1};

    int32_t total_value = 0;
    uint8_t dir_idx = 0;

    eval_next_direction:
    if (dir_idx >= 4) goto eval_done;

    gs->cells[pos] = player;

    uint8_t count, opens;
    scan_direction_full(gs, pos, deltas[dir_idx], player, &count, &opens);

    gs->cells[pos] = VACANT;

    uint8_t cnt_idx = (count > 5) ? 5 : count;
    uint8_t opn_idx = (opens > 2) ? 2 : opens;

    total_value += pattern_scores[cnt_idx][opn_idx];

    dir_idx++;
    goto eval_next_direction;

    eval_done:
    return total_value;
}

/* Board evaluation */
static int32_t eval_board(GameState* gs, uint8_t my_color, uint8_t opp_color) {
    int32_t my_total = 0;
    int32_t opp_total = 0;

    int16_t pos = 0;

    eval_loop:
    if (pos >= CELLS) goto eval_finish;

    uint8_t cell = gs->cells[pos];

    if (cell == my_color) {
        gs->cells[pos] = VACANT;
        my_total += eval_position(gs, pos, my_color);
        gs->cells[pos] = my_color;
    } else if (cell == opp_color) {
        gs->cells[pos] = VACANT;
        opp_total += eval_position(gs, pos, opp_color);
        gs->cells[pos] = opp_color;
    }

    pos++;
    goto eval_loop;

    eval_finish:
    return my_total - (opp_total * 85 / 100);
}

/* ==================== SEARCH MODULE ==================== */

/* Time management */
static int64_t get_time_ms(void) {
#if defined(_WIN32) || defined(WIN32)
    return (int64_t)GetTickCount();
#elif defined(__linux__) || defined(__unix__)
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
#else
    return (int64_t)(clock() * 1000.0 / CLOCKS_PER_SEC);
#endif
}

/* Zobrist initialization */
static void init_zobrist(SearchEngine* engine) {
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

static void search_init(SearchEngine* engine, GameState* gs, uint8_t my_color) {
    engine->state = gs;
    engine->my_color = my_color;
    engine->opp_color = (my_color == STONE_A) ? STONE_B : STONE_A;
    engine->nodes = 0;

    engine->hash_table = (HashEntry*)calloc(HASH_SIZE, sizeof(HashEntry));

    init_zobrist(engine);
}

static void search_cleanup(SearchEngine* engine) {
    if (engine->hash_table) {
        free(engine->hash_table);
        engine->hash_table = NULL;
    }
}

/* Generate candidate moves */
static int generate_moves(SearchEngine* engine, Candidate* cands, uint8_t for_player) {
    GameState* gs = engine->state;
    uint8_t enemy = (for_player == STONE_A) ? STONE_B : STONE_A;

    int count = 0;
    int16_t pos = 0;

    int threshold = MIN_THRESHOLD;
    if (gs->ply < 12) {
        threshold = 0;
    } else if (gs->ply >= 30) {
        threshold = MIN_THRESHOLD * 2;
    }

    gen_loop:
    if (pos >= CELLS) goto gen_sort;
    if (count >= CELLS) goto gen_sort;

    if (gs->cells[pos] != VACANT) {
        pos++;
        goto gen_loop;
    }

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

/* Find immediate win/block */
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

/* PVS search */
static int32_t pvs_search(SearchEngine* engine, int depth, int32_t alpha, int32_t beta, int is_pv);

static int32_t pvs_search(SearchEngine* engine, int depth, int32_t alpha, int32_t beta, int is_pv) {
    if ((engine->nodes & 1023) == 0) {
        if (get_time_ms() >= engine->time_limit) {
            return eval_board(engine->state, engine->my_color, engine->opp_color);
        }
    }

    engine->nodes++;

    if (depth <= 0) {
        return eval_board(engine->state, engine->my_color, engine->opp_color);
    }

    Candidate candidates[CELLS];
    uint8_t current = engine->state->current_player;
    int num_moves = generate_moves(engine, candidates, current);

    if (num_moves == 0) {
        return eval_board(engine->state, engine->my_color, engine->opp_color);
    }

    int maximizing = (current == engine->my_color) ? 1 : 0;

    if (maximizing) {
        int32_t best = INT_MIN;
        int move_idx = 0;
        int is_first = 1;

        max_loop:
        if (move_idx >= num_moves) goto max_done;

        int16_t pos = candidates[move_idx].pos;

        gs_do_move(engine->state, pos, current);

        if (gs_check_win(engine->state, pos, current)) {
            gs_undo_move(engine->state);
            return WIN_SCORE - depth;
        }

        int32_t score;

        if (is_first) {
            score = pvs_search(engine, depth - 1, alpha, beta, is_pv);
            is_first = 0;
        } else {
            score = pvs_search(engine, depth - 1, alpha, alpha + 1, 0);

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
static int16_t search_best_move(SearchEngine* engine, int time_limit_ms) {
    engine->start_time = get_time_ms();
    engine->time_limit = engine->start_time + time_limit_ms;
    engine->nodes = 0;

    int16_t win_move = find_critical_move(engine, engine->my_color);
    if (win_move >= 0) return win_move;

    int16_t block_move = find_critical_move(engine, engine->opp_color);
    if (block_move >= 0) return block_move;

    Candidate candidates[CELLS];
    int num_moves = generate_moves(engine, candidates, engine->my_color);

    if (num_moves == 0) return SIZE / 2 * SIZE + SIZE / 2;

    int16_t best_move = candidates[0].pos;

    int max_depth = 6;
    if (engine->state->ply < 10) {
        max_depth = 4;
    } else if (engine->state->ply >= 80) {
        max_depth = 8;
    }

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
        return pos;
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

/* ==================== MAIN DRIVER ==================== */

typedef struct {
    GameState game;
    SearchEngine engine;
    MachineState state;
    uint8_t my_player;
    uint8_t opp_player;
    int64_t total_time_used;
} Context;

static Context ctx;

/* Command handlers */
typedef void (*CmdHandler)(const char* line);

static void cmd_start(const char* line);
static void cmd_place(const char* line);
static void cmd_turn(const char* line);
static void cmd_end(const char* line);

typedef struct {
    const char* prefix;
    int prefix_len;
    CmdHandler handler;
} CmdEntry;

static const CmdEntry cmd_table[] = {
    {"START", 5, cmd_start},
    {"PLACE", 5, cmd_place},
    {"TURN", 4, cmd_turn},
    {"END", 3, cmd_end},
    {NULL, 0, NULL}
};

/* State machine dispatcher */
static void dispatch_command(const char* line) {
    const CmdEntry* entry = cmd_table;

    lookup:
    if (entry->prefix == NULL) goto unknown_cmd;

    if (strncmp(line, entry->prefix, entry->prefix_len) == 0) {
        entry->handler(line);
        return;
    }

    entry++;
    goto lookup;

    unknown_cmd:
    return;
}

/* START command */
static void cmd_start(const char* line) {
    int field;
    sscanf(line, "START %d", &field);

    ctx.my_player = (field == 1) ? STONE_A : STONE_B;
    ctx.opp_player = (field == 1) ? STONE_B : STONE_A;

    gs_init(&ctx.game);
    gs_setup_initial(&ctx.game);

    eval_init();
    search_init(&ctx.engine, &ctx.game, ctx.my_player);

    ctx.state = ST_READY;
    ctx.total_time_used = 0;

    printf("OK\n");
    fflush(stdout);
}

/* PLACE command */
static void cmd_place(const char* line) {
    int row, col;
    sscanf(line, "PLACE %d %d", &row, &col);

    int16_t pos = gs_to_pos(row, col);
    gs_do_move(&ctx.game, pos, ctx.opp_player);
}

/* TURN command */
static void cmd_turn(const char* line) {
    (void)line;
    ctx.state = ST_THINKING;

    int16_t best_pos = search_best_move(&ctx.engine, MOVE_TIME);

    gs_do_move(&ctx.game, best_pos, ctx.my_player);

    int row = gs_to_row(best_pos);
    int col = gs_to_col(best_pos);

    printf("%d %d\n", row, col);
    fflush(stdout);

    ctx.state = ST_READY;
}

/* END command */
static void cmd_end(const char* line) {
    (void)line;
    ctx.state = ST_DONE;
    search_cleanup(&ctx.engine);
}

/* Main loop */
int main(void) {
    char buffer[256];

    ctx.state = ST_INIT;

    event_loop:
    if (ctx.state == ST_DONE) goto cleanup;

    if (!fgets(buffer, sizeof(buffer), stdin)) goto cleanup;

    char* newline = strchr(buffer, '\n');
    if (newline) *newline = '\0';

    dispatch_command(buffer);

    goto event_loop;

    cleanup:
    return 0;
}
