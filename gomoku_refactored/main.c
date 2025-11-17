/*
 * Gomoku AI - Main Program
 * Refactored version with different architecture
 * Command protocol handler
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "types.h"
#include "board.h"
#include "evaluator.h"
#include "search.h"

/* Global game state */
static GameState game;
static Timer game_timer;
static SearchEngine engine;
static int ai_color = 0;
static int enemy_color = 0;

/* Command handlers - using function pointer table */
typedef void (*CommandHandler)(const char*);

/* Forward declarations */
void cmd_start(const char* line);
void cmd_place(const char* line);
void cmd_turn(const char* line);
void cmd_end(const char* line);

/* Command dispatch table - different from if-else chain */
typedef struct {
    const char* name;
    int name_len;
    CommandHandler handler;
} CommandEntry;

static const CommandEntry command_table[] = {
    {"START", 5, cmd_start},
    {"PLACE", 5, cmd_place},
    {"TURN", 4, cmd_turn},
    {"END", 3, cmd_end},
    {NULL, 0, NULL}
};

/* Initialize game */
void game_init(void) {
    state_init(&game);
    timer_init(&game_timer);
    eval_init_tables();
}

/* Setup initial position */
void setup_initial_position(void) {
    state_setup_initial(&game);
}

/* START command handler */
void cmd_start(const char* line) {
    int player_id;

    /* Parse player color */
    sscanf(line, "START %d", &player_id);

    ai_color = player_id;
    enemy_color = (player_id == 1) ? 2 : 1;

    /* Reset game state */
    game_init();
    setup_initial_position();

    /* Send response */
    printf("OK\n");
    fflush(stdout);
}

/* PLACE command handler */
void cmd_place(const char* line) {
    int row, col, index;

    /* Parse coordinates */
    sscanf(line, "PLACE %d %d", &row, &col);

    /* Convert to index and make move */
    index = coord_to_index(row, col);
    make_move(&game, index, enemy_color);
}

/* TURN command handler */
void cmd_turn(const char* line) {
    int best_index, row, col;

    /* Start timer */
    timer_start(&game_timer);

    /* Initialize search engine */
    search_init(&engine, &game, &game_timer, ai_color);

    /* Search for best move */
    best_index = search_best_move(&engine);

    /* Make move */
    make_move(&game, best_index, ai_color);

    /* Convert index to coordinates */
    index_to_coord(best_index, &row, &col);

    /* Output move */
    printf("%d %d\n", row, col);
    fflush(stdout);

    /* Stop timer */
    timer_stop(&game_timer);
}

/* END command handler */
void cmd_end(const char* line) {
    /* Game over - cleanup if needed */
}

/* Dispatch command using table lookup - different from if-else */
void dispatch_command(const char* line) {
    const CommandEntry* entry;
    int matched;

    entry = command_table;

    /* Table-driven dispatch loop */
    while (entry->name != NULL) {
        matched = (strncmp(line, entry->name, entry->name_len) == 0);

        if (matched) {
            entry->handler(line);
            return;
        }

        entry++;
    }
}

/* Main loop using different pattern */
int main(void) {
    char buffer[256];
    char* result;
    int running;

    /* Initialize */
    game_init();

    running = 1;

    /* Main loop using while with flag */
    while (running) {
        /* Read input */
        result = fgets(buffer, sizeof(buffer), stdin);

        /* Check EOF */
        if (result == NULL) {
            running = 0;
            continue;
        }

        /* Remove newline */
        buffer[strcspn(buffer, "\n")] = '\0';

        /* Check for END command */
        if (strncmp(buffer, "END", 3) == 0) {
            cmd_end(buffer);
            running = 0;
            continue;
        }

        /* Dispatch command */
        dispatch_command(buffer);
    }

    return 0;
}
