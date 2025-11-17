/*
 * Gomoku AI Main Driver
 * Uses state machine pattern for command processing
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "game_state.h"
#include "evaluator.h"
#include "search.h"

/* Global context */
typedef struct {
    GameState game;
    SearchEngine engine;
    MachineState state;
    uint8_t my_player;
    uint8_t opp_player;
    int64_t total_time_used;
} Context;

static Context ctx;

/* Command handlers - function pointer approach */
typedef void (*CmdHandler)(const char* line);

/* Forward declarations */
static void cmd_start(const char* line);
static void cmd_place(const char* line);
static void cmd_turn(const char* line);
static void cmd_end(const char* line);

/* Command dispatch table */
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

/* State machine command dispatcher */
static void dispatch_command(const char* line) {
    const CmdEntry* entry = cmd_table;

    /* Table lookup loop using goto */
    lookup:
    if (entry->prefix == NULL) goto unknown_cmd;

    if (strncmp(line, entry->prefix, entry->prefix_len) == 0) {
        entry->handler(line);
        return;
    }

    entry++;
    goto lookup;

    unknown_cmd:
    return;  /* Ignore unknown commands */
}

/* START command handler */
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

/* PLACE command handler */
static void cmd_place(const char* line) {
    int row, col;
    sscanf(line, "PLACE %d %d", &row, &col);

    int16_t pos = gs_to_pos(row, col);
    gs_do_move(&ctx.game, pos, ctx.opp_player);
}

/* TURN command handler */
static void cmd_turn(const char* line) {
    (void)line;  /* Unused */
    ctx.state = ST_THINKING;

    /* Search for best move */
    int16_t best_pos = search_best_move(&ctx.engine, MOVE_TIME);

    /* Make the move */
    gs_do_move(&ctx.game, best_pos, ctx.my_player);

    /* Output */
    int row = gs_to_row(best_pos);
    int col = gs_to_col(best_pos);

    printf("%d %d\n", row, col);
    fflush(stdout);

    ctx.state = ST_READY;
}

/* END command handler */
static void cmd_end(const char* line) {
    (void)line;  /* Unused */
    ctx.state = ST_DONE;
    search_cleanup(&ctx.engine);
}

/* Main loop - event-driven architecture */
int main(void) {
    char buffer[256];

    ctx.state = ST_INIT;

    /* Event loop using goto */
    event_loop:
    if (ctx.state == ST_DONE) goto cleanup;

    if (!fgets(buffer, sizeof(buffer), stdin)) goto cleanup;

    /* Remove newline */
    char* newline = strchr(buffer, '\n');
    if (newline) *newline = '\0';

    /* Dispatch command */
    dispatch_command(buffer);

    goto event_loop;

    cleanup:
    return 0;
}
