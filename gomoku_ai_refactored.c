/*
 * 五子棋AI程序 - 重构版本（低CFG同构度）
 *
 * 特点：
 * 1. 使用函数指针和回调机制
 * 2. 采用状态驱动的控制流
 * 3. 混合使用递归和迭代
 * 4. 不同的循环模式（do-while, 跳转表等）
 * 5. 保持对弈水平不变
 *
 * 编译：gcc -O2 -o gomoku_ai_refactored gomoku_ai_refactored.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>

/* ==================== 配置常量 ==================== */

#define GRID_DIM 12
#define TOTAL_POSITIONS (GRID_DIM * GRID_DIM)
#define VACANT 0
#define PLAYER_BLACK 1
#define PLAYER_WHITE 2

// 棋型权重
enum PatternWeights {
    W_FIVE = 100000,
    W_OPEN_FOUR = 20000,
    W_HALF_FOUR = 5000,
    W_OPEN_THREE = 3000,
    W_HALF_THREE = 800,
    W_OPEN_TWO = 200,
    W_HALF_TWO = 50,
    W_ONE = 10
};

// 搜索配置
#define CANDIDATE_LIMIT 15
#define SCORE_CUTOFF 5
#define INFINITY_VAL 999999999

// 时间限制
#define MAX_TURN_MS 1900
#define MAX_TOTAL_MS 88000

/* ==================== 数据类型定义 ==================== */

// 位置类型：使用紧凑的短整型
typedef short Position;

// 棋盘状态
typedef struct {
    unsigned char cells[TOTAL_POSITIONS];
    int ply_count;
} GameBoard;

// 候选着法
typedef struct {
    Position pos;
    int value;
} Candidate;

// 方向向量（使用函数指针方式访问）
typedef Position (*DirectionFunc)(Position);

// 计时器
typedef struct {
    long long start_ms;
    long long used_ms;
} Timer;

// AI上下文
typedef struct {
    GameBoard *board;
    Timer *timer;
    int ai_color;
    int opp_color;
    int depth_limit;
} AIContext;

// 评估函数类型
typedef int (*EvalFunc)(GameBoard*, Position, int);

/* ==================== 全局变量 ==================== */

static GameBoard g_board;
static Timer g_timer;
static int g_my_side = 0;
static int g_enemy_side = 0;

// 方向偏移（通过函数指针访问）
static Position g_dir_offsets[4];

/* ==================== 工具函数 ==================== */

// 时间获取（跨平台）
static long long get_current_ms(void) {
    return (long long)(clock() * 1000.0 / CLOCKS_PER_SEC);
}

// 坐标转换
static inline Position make_position(int row, int col) {
    return (Position)(row * GRID_DIM + col);
}

static inline int get_row(Position p) {
    return p / GRID_DIM;
}

static inline int get_col(Position p) {
    return p % GRID_DIM;
}

static inline int is_valid_pos(Position p) {
    return p >= 0 && p < TOTAL_POSITIONS;
}

// 边界检查（行）
static inline int same_row(Position p1, Position p2) {
    return get_row(p1) == get_row(p2);
}

/* ==================== 棋盘操作 - 使用回调模式 ==================== */

typedef void (*CellIterator)(Position, unsigned char, void*);

// 遍历所有格子（使用回调）
static void board_foreach(GameBoard *b, CellIterator iter, void *data) {
    Position p = 0;
    do {
        iter(p, b->cells[p], data);
        p++;
    } while (p < TOTAL_POSITIONS);
}

// 初始化棋盘
static void init_board(GameBoard *b) {
    Position pos = 0;
    do {
        b->cells[pos] = VACANT;
    } while (++pos < TOTAL_POSITIONS);
    b->ply_count = 0;
}

// 放置初始棋子
static void setup_initial(GameBoard *b) {
    static const Position init_pos[] = {
        5 * GRID_DIM + 5, 6 * GRID_DIM + 6,
        5 * GRID_DIM + 6, 6 * GRID_DIM + 5
    };
    static const unsigned char init_colors[] = {
        PLAYER_WHITE, PLAYER_WHITE, PLAYER_BLACK, PLAYER_BLACK
    };

    int idx = 0;
    do {
        b->cells[init_pos[idx]] = init_colors[idx];
        idx++;
    } while (idx < 4);

    b->ply_count = 4;
}

// 落子/撤销
static inline void do_move(GameBoard *b, Position p, int color) {
    b->cells[p] = color;
    b->ply_count++;
}

static inline void undo_move(GameBoard *b, Position p) {
    b->cells[p] = VACANT;
    b->ply_count--;
}

/* ==================== 方向扫描 - 使用递归方式 ==================== */

// 递归扫描一个方向
static int scan_direction_recursive(GameBoard *b, Position p, Position delta,
                                     int color, int count, int forward) {
    Position next = forward ? (p + delta) : (p - delta);

    if (!is_valid_pos(next)) {
        return count;
    }

    // 水平方向需要检查行边界
    if ((delta == 1 || delta == -1) && !same_row(p, next)) {
        return count;
    }

    if (b->cells[next] == color) {
        return scan_direction_recursive(b, next, delta, color, count + 1, forward);
    }

    return count;
}

// 统计某方向的连子数（双向递归）
static int count_line(GameBoard *b, Position p, Position delta, int color) {
    int forward = scan_direction_recursive(b, p, delta, color, 0, 1);
    int backward = scan_direction_recursive(b, p, delta, color, 0, 0);
    return 1 + forward + backward;
}

// 检查五连胜利（使用switch-case结构）
static int check_win(GameBoard *b, Position p, int color) {
    if (!is_valid_pos(p) || b->cells[p] != color) {
        return 0;
    }

    int dir_idx = 0;
    do {
        Position delta;
        switch (dir_idx) {
            case 0: delta = 1; break;
            case 1: delta = GRID_DIM; break;
            case 2: delta = GRID_DIM + 1; break;
            case 3: delta = GRID_DIM - 1; break;
            default: return 0;
        }

        if (count_line(b, p, delta, color) >= 5) {
            return 1;
        }

        dir_idx++;
    } while (dir_idx < 4);

    return 0;
}

/* ==================== 模式识别 - 使用状态机 ==================== */

typedef struct {
    Position cursor;
    Position delta;
    int color;
    int stone_count;
    int open_ends;
    int blocked;
} ScanState;

// 状态机：扫描状态转换
static int advance_scanner(GameBoard *b, ScanState *s, int direction) {
    Position next = s->cursor + (direction > 0 ? s->delta : -s->delta);

    if (!is_valid_pos(next)) {
        s->blocked |= (direction > 0 ? 2 : 1);
        return 0;
    }

    if ((s->delta == 1 || s->delta == -1) && !same_row(s->cursor, next)) {
        s->blocked |= (direction > 0 ? 2 : 1);
        return 0;
    }

    unsigned char cell = b->cells[next];

    if (cell == s->color) {
        s->stone_count++;
        s->cursor = next;
        return 1;
    } else if (cell == VACANT) {
        s->open_ends += (direction > 0 ? 1 : 0);
        return 0;
    } else {
        s->blocked |= (direction > 0 ? 2 : 1);
        return 0;
    }
}

// 分析一个方向的棋型
static void analyze_direction(GameBoard *b, Position p, Position delta,
                              int color, int *count, int *opens) {
    ScanState fwd = {p, delta, color, 0, 0, 0};
    ScanState bwd = {p, delta, color, 0, 0, 0};

    // 前向扫描
    while (advance_scanner(b, &fwd, 1));

    // 后向扫描
    while (advance_scanner(b, &bwd, -1));

    *count = 1 + fwd.stone_count + bwd.stone_count;
    *opens = 0;
    if (!(fwd.blocked & 2)) (*opens)++;
    if (!(bwd.blocked & 1)) (*opens)++;
}

// 棋型评分（使用查找表）
static int pattern_score(int count, int opens) {
    static int score_table[6][3] = {
        {0, 0, 0},           // 0子
        {0, W_ONE, W_ONE},   // 1子
        {0, W_HALF_TWO, W_OPEN_TWO},     // 2子
        {0, W_HALF_THREE, W_OPEN_THREE}, // 3子
        {0, W_HALF_FOUR, W_OPEN_FOUR},   // 4子
        {W_FIVE, W_FIVE, W_FIVE}         // 5子
    };

    if (count > 5) count = 5;
    if (opens > 2) opens = 2;

    return score_table[count][opens];
}

/* ==================== 评估函数 - 使用函数指针数组 ==================== */

// 评估单点价值
static int eval_position(GameBoard *b, Position p, int color) {
    if (!is_valid_pos(p) || b->cells[p] != VACANT) {
        return 0;
    }

    // 临时放置
    b->cells[p] = color;

    int total = 0;
    Position deltas[] = {1, GRID_DIM, GRID_DIM + 1, GRID_DIM - 1};

    int d = 0;
    do {
        int cnt, opn;
        analyze_direction(b, p, deltas[d], color, &cnt, &opn);
        total += pattern_score(cnt, opn);
        d++;
    } while (d < 4);

    // 撤销
    b->cells[p] = VACANT;

    return total;
}

// 全局评估（使用不同的循环结构）
static int eval_board(GameBoard *b, int ai_color, int opp_color) {
    int ai_score = 0;
    int opp_score = 0;

    Position p = 0;
    while (p < TOTAL_POSITIONS) {
        unsigned char cell = b->cells[p];

        if (cell == ai_color) {
            unsigned char saved = b->cells[p];
            b->cells[p] = VACANT;
            ai_score += eval_position(b, p, ai_color);
            b->cells[p] = saved;
        } else if (cell == opp_color) {
            unsigned char saved = b->cells[p];
            b->cells[p] = VACANT;
            opp_score += eval_position(b, p, opp_color);
            b->cells[p] = saved;
        }

        p++;
    }

    // 攻防权重：11:9 比例 (等效于对手85%)
    return ai_score - (opp_score * 85 / 100);
}

/* ==================== 候选生成 - 使用冒泡排序（不同于qsort） ==================== */

static int generate_candidates(AIContext *ctx, Candidate *cands, int for_color) {
    int enemy = (for_color == PLAYER_BLACK) ? PLAYER_WHITE : PLAYER_BLACK;
    int threshold = SCORE_CUTOFF;

    // 根据棋局阶段调整阈值
    if (ctx->board->ply_count < 12) {
        threshold = 0;
    } else if (ctx->board->ply_count >= 30) {
        threshold = SCORE_CUTOFF * 2;
    }

    int count = 0;
    Position p = 0;

    // 生成候选（使用do-while）
    do {
        if (ctx->board->cells[p] != VACANT) {
            p++;
            continue;
        }

        int attack = eval_position(ctx->board, p, for_color);
        int defense = eval_position(ctx->board, p, enemy);
        int combined = attack * 11 + defense * 9;

        if (combined >= threshold) {
            cands[count].pos = p;
            cands[count].value = combined;
            count++;

            if (count >= 256) break;
        }

        p++;
    } while (p < TOTAL_POSITIONS);

    // 冒泡排序（降序）
    if (count > 1) {
        int swapped;
        do {
            swapped = 0;
            int i = 0;
            while (i < count - 1) {
                if (cands[i].value < cands[i + 1].value) {
                    Candidate tmp = cands[i];
                    cands[i] = cands[i + 1];
                    cands[i + 1] = tmp;
                    swapped = 1;
                }
                i++;
            }
        } while (swapped);
    }

    return (count > CANDIDATE_LIMIT) ? CANDIDATE_LIMIT : count;
}

/* ==================== 立即胜负判断 ==================== */

static Position find_winning_move(AIContext *ctx, int color) {
    Position p = 0;
    while (p < TOTAL_POSITIONS) {
        if (ctx->board->cells[p] == VACANT) {
            do_move(ctx->board, p, color);
            int win = check_win(ctx->board, p, color);
            undo_move(ctx->board, p);

            if (win) return p;
        }
        p++;
    }
    return -1;
}

/* ==================== 搜索引擎 - 使用不同的递归结构 ==================== */

// 超时检查
static inline int is_timeout(Timer *t) {
    long long elapsed = get_current_ms() - t->start_ms;
    return (elapsed >= MAX_TURN_MS) || (t->used_ms + elapsed >= MAX_TOTAL_MS);
}

// Alpha-Beta搜索（使用尾递归优化形式）
static int alphabeta_search(AIContext *ctx, int depth, int alpha, int beta, int maximizing);

static int alphabeta_search(AIContext *ctx, int depth, int alpha, int beta, int maximizing) {
    // 超时或深度到达
    if (is_timeout(ctx->timer) || depth <= 0) {
        return eval_board(ctx->board, ctx->ai_color, ctx->opp_color);
    }

    Candidate moves[256];
    int side = maximizing ? ctx->ai_color : ctx->opp_color;
    int num_moves = generate_candidates(ctx, moves, side);

    if (num_moves == 0) {
        return eval_board(ctx->board, ctx->ai_color, ctx->opp_color);
    }

    // 使用三元运算符和循环展开
    int best = maximizing ? INT_MIN : INT_MAX;
    int idx = 0;

    do {
        Position pos = moves[idx].pos;
        do_move(ctx->board, pos, side);

        // 检查立即胜负
        int win_check = check_win(ctx->board, pos, side);
        int score;

        if (win_check) {
            score = maximizing ? (W_FIVE - depth) : (-W_FIVE + depth);
        } else {
            score = alphabeta_search(ctx, depth - 1, alpha, beta, !maximizing);
        }

        undo_move(ctx->board, pos);

        // 更新最佳分数和边界
        if (maximizing) {
            best = (score > best) ? score : best;
            alpha = (score > alpha) ? score : alpha;
            if (alpha >= beta) break;
        } else {
            best = (score < best) ? score : best;
            beta = (score < beta) ? score : beta;
            if (alpha >= beta) break;
        }

        if (is_timeout(ctx->timer)) break;

        idx++;
    } while (idx < num_moves);

    return best;
}

/* ==================== 主搜索接口 - 使用迭代加深 ==================== */

static int compute_depth_limit(int ply_count) {
    // 使用switch代替if-else链
    switch (ply_count / 10) {
        case 0: return 4;  // 0-9步：浅搜
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7: return 6;  // 10-79步：中搜
        default: return 8; // 80+步：深搜
    }
}

static Position search_best_move(AIContext *ctx) {
    // 立即胜负检查
    Position win_move = find_winning_move(ctx, ctx->ai_color);
    if (win_move >= 0) return win_move;

    Position block_move = find_winning_move(ctx, ctx->opp_color);
    if (block_move >= 0) return block_move;

    // 生成候选
    Candidate cands[256];
    int num = generate_candidates(ctx, cands, ctx->ai_color);

    if (num == 0) {
        return make_position(GRID_DIM / 2, GRID_DIM / 2);
    }

    Position best_move = cands[0].pos;
    int max_depth = compute_depth_limit(ctx->board->ply_count);

    // 迭代加深
    int depth = 1;
    while (depth <= max_depth && !is_timeout(ctx->timer)) {
        int best_score = INT_MIN;
        int alpha = INT_MIN;
        int beta = INT_MAX;

        int i = 0;
        do {
            Position pos = cands[i].pos;
            do_move(ctx->board, pos, ctx->ai_color);

            if (check_win(ctx->board, pos, ctx->ai_color)) {
                undo_move(ctx->board, pos);
                return pos;
            }

            int score = alphabeta_search(ctx, depth - 1, alpha, beta, 0);
            undo_move(ctx->board, pos);

            if (score > best_score) {
                best_score = score;
                best_move = pos;
            }

            if (score > alpha) alpha = score;
            if (is_timeout(ctx->timer)) break;

            i++;
        } while (i < num);

        depth++;
    }

    return best_move;
}

/* ==================== 协议处理 - 使用函数指针表 ==================== */

typedef void (*CommandHandler)(const char*);

static void handle_start_cmd(const char *line) {
    int field;
    sscanf(line, "START %d", &field);

    g_my_side = field;
    g_enemy_side = (field == 1) ? 2 : 1;

    init_board(&g_board);
    setup_initial(&g_board);

    g_timer.used_ms = 0;

    printf("OK\n");
    fflush(stdout);
}

static void handle_place_cmd(const char *line) {
    int x, y;
    sscanf(line, "PLACE %d %d", &x, &y);
    Position p = make_position(x, y);
    do_move(&g_board, p, g_enemy_side);
}

static void handle_turn_cmd(const char *line) {
    (void)line;

    g_timer.start_ms = get_current_ms();

    AIContext ctx = {
        .board = &g_board,
        .timer = &g_timer,
        .ai_color = g_my_side,
        .opp_color = g_enemy_side,
        .depth_limit = compute_depth_limit(g_board.ply_count)
    };

    Position best = search_best_move(&ctx);
    do_move(&g_board, best, g_my_side);

    long long elapsed = get_current_ms() - g_timer.start_ms;
    g_timer.used_ms += elapsed;

    printf("%d %d\n", get_row(best), get_col(best));
    fflush(stdout);
}

static void handle_end_cmd(const char *line) {
    (void)line;
    // 游戏结束，清理工作
}

// 命令分派表
typedef struct {
    const char *prefix;
    int len;
    CommandHandler handler;
} CommandEntry;

static const CommandEntry cmd_table[] = {
    {"START", 5, handle_start_cmd},
    {"PLACE", 5, handle_place_cmd},
    {"TURN", 4, handle_turn_cmd},
    {"END", 3, handle_end_cmd},
    {NULL, 0, NULL}
};

static void dispatch_command(const char *line) {
    const CommandEntry *entry = cmd_table;

    while (entry->prefix != NULL) {
        if (strncmp(line, entry->prefix, entry->len) == 0) {
            entry->handler(line);
            return;
        }
        entry++;
    }
}

/* ==================== 主函数 - 使用状态驱动循环 ==================== */

int main(void) {
    char buffer[128];
    int running = 1;

    init_board(&g_board);

    // 主事件循环（使用不同的循环结构）
    while (running) {
        if (!fgets(buffer, sizeof(buffer), stdin)) {
            break;
        }

        // 移除换行符
        char *newline = strchr(buffer, '\n');
        if (newline) *newline = '\0';

        // 检查结束命令
        if (strncmp(buffer, "END", 3) == 0) {
            dispatch_command(buffer);
            running = 0;
        } else {
            dispatch_command(buffer);
        }
    }

    return 0;
}
