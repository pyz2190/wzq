/*
 * 五子棋AI程序（完全通用版本 - 适用于所有平台）
 * 实现基于Alpha-Beta剪枝的搜索算法
 * 作者：基于原创设计实现
 * 日期：2025-11-17
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>

/* ==================== 常量定义 ==================== */

#define BOARD_SIZE 12
#define EMPTY 0
#define BLACK 1
#define WHITE 2

// 棋型评分（原创设计）
#define SCORE_FIVE       100000
#define SCORE_LIVE_FOUR   20000
#define SCORE_RUSH_FOUR    5000
#define SCORE_LIVE_THREE   3000
#define SCORE_SLEEP_THREE   800
#define SCORE_LIVE_TWO      200
#define SCORE_SLEEP_TWO      50
#define SCORE_SINGLE         10

// 搜索参数
#define MAX_CANDIDATES 15
#define MIN_SCORE_THRESHOLD 5  // 最低评分阈值，低于此值的候选位置不考虑
#define INF 999999999

// 时间控制（毫秒）
#define TURN_TIME_LIMIT 1900
#define TOTAL_TIME_LIMIT 88000

/* ==================== 数据结构定义 ==================== */

// 棋盘结构
typedef struct {
    int grid[BOARD_SIZE][BOARD_SIZE];
    int move_count;
} Board;

// 走法结构
typedef struct {
    int row;
    int col;
    int priority;
} Move;

// 时间管理器
typedef struct {
    long long total_time_used;
    long long current_start;
    int turn_time_limit;
    int total_time_limit;
} TimeManager;

// 搜索上下文
typedef struct {
    Board* board;
    TimeManager* timer;
    int my_side;
    int enemy_side;
} SearchContext;

/* ==================== 全局变量 ==================== */

Board game_board;
TimeManager time_manager;
int my_color = 0;
int enemy_color = 0;

// 四个主方向（水平、垂直、两条对角线）
const int dir_x[4] = {1, 0, 1, 1};
const int dir_y[4] = {0, 1, 1, -1};

/* ==================== 时间管理函数 ==================== */

// 获取当前时间（毫秒）- 使用标准C库的通用实现
long long get_time_ms() {
    // 使用标准C库的clock()函数
    // 注意：clock()返回的是CPU时间，不是墙上时间，但对于这个应用足够了
    return (long long)(clock() * 1000.0 / CLOCKS_PER_SEC);
}

// 初始化时间管理器
void init_timer(TimeManager* tm) {
    tm->total_time_used = 0;
    tm->current_start = 0;
    tm->turn_time_limit = TURN_TIME_LIMIT;
    tm->total_time_limit = TOTAL_TIME_LIMIT;
}

// 开始一个回合
void start_turn(TimeManager* tm) {
    tm->current_start = get_time_ms();
}

// 结束一个回合
void end_turn(TimeManager* tm) {
    long long elapsed = get_time_ms() - tm->current_start;
    tm->total_time_used += elapsed;
}

// 检查是否超时
int is_timeout(TimeManager* tm) {
    long long current = get_time_ms();
    long long turn_elapsed = current - tm->current_start;

    if (turn_elapsed >= tm->turn_time_limit) {
        return 1;
    }

    if (tm->total_time_used + turn_elapsed >= tm->total_time_limit) {
        return 1;
    }

    return 0;
}

/* ==================== 棋盘管理函数 ==================== */

// 初始化棋盘
void init_board(Board* board) {
    int i, j;
    for (i = 0; i < BOARD_SIZE; i++) {
        for (j = 0; j < BOARD_SIZE; j++) {
            board->grid[i][j] = EMPTY;
        }
    }
    board->move_count = 0;
}

// 边界检查
int in_bounds(int row, int col) {
    return row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE;
}

// 检查是否为空位
int is_empty(Board* board, int row, int col) {
    if (!in_bounds(row, col)) return 0;
    return board->grid[row][col] == EMPTY;
}

// 落子
void place_stone(Board* board, int row, int col, int color) {
    if (in_bounds(row, col) && board->grid[row][col] == EMPTY) {
        board->grid[row][col] = color;
        board->move_count++;
    }
}

// 悔棋
void undo_stone(Board* board, int row, int col) {
    if (in_bounds(row, col) && board->grid[row][col] != EMPTY) {
        board->grid[row][col] = EMPTY;
        board->move_count--;
    }
}

// 检查从某点开始是否形成五连
int check_five(Board* board, int row, int col, int color) {
    int d, i, count;

    if (!in_bounds(row, col) || board->grid[row][col] != color) {
        return 0;
    }

    // 检查四个方向
    for (d = 0; d < 4; d++) {
        count = 1;

        // 正方向
        for (i = 1; i < 5; i++) {
            int nr = row + dir_x[d] * i;
            int nc = col + dir_y[d] * i;
            if (in_bounds(nr, nc) && board->grid[nr][nc] == color) {
                count++;
            } else {
                break;
            }
        }

        // 反方向
        for (i = 1; i < 5; i++) {
            int nr = row - dir_x[d] * i;
            int nc = col - dir_y[d] * i;
            if (in_bounds(nr, nc) && board->grid[nr][nc] == color) {
                count++;
            } else {
                break;
            }
        }

        if (count >= 5) return 1;
    }

    return 0;
}

/* ==================== 评估函数 ==================== */

// 判断棋型得分
int get_pattern_score(int count, int left_open, int right_open) {
    int open_ends = left_open + right_open;

    if (count >= 5) {
        return SCORE_FIVE;
    }

    if (count == 4) {
        if (open_ends == 2) return SCORE_LIVE_FOUR;
        if (open_ends == 1) return SCORE_RUSH_FOUR;
        return 0;
    }

    if (count == 3) {
        if (open_ends == 2) return SCORE_LIVE_THREE;
        if (open_ends == 1) return SCORE_SLEEP_THREE;
        return 0;
    }

    if (count == 2) {
        if (open_ends == 2) return SCORE_LIVE_TWO;
        if (open_ends == 1) return SCORE_SLEEP_TWO;
        return 0;
    }

    return SCORE_SINGLE;
}

// 快速单点评估（用于候选排序）
int evaluate_point(Board* board, int row, int col, int color) {
    int total = 0;
    int d, i, count, left_open, right_open;
    int nr, nc;

    // 四个方向
    for (d = 0; d < 4; d++) {
        count = 1;  // 包含当前点
        left_open = 0;
        right_open = 0;

        // 正方向统计
        for (i = 1; i < 6; i++) {
            nr = row + dir_x[d] * i;
            nc = col + dir_y[d] * i;
            if (!in_bounds(nr, nc)) break;
            if (board->grid[nr][nc] == color) {
                count++;
            } else if (board->grid[nr][nc] == EMPTY) {
                right_open = 1;
                break;
            } else {
                break;
            }
        }

        // 反方向统计
        for (i = 1; i < 6; i++) {
            nr = row - dir_x[d] * i;
            nc = col - dir_y[d] * i;
            if (!in_bounds(nr, nc)) break;
            if (board->grid[nr][nc] == color) {
                count++;
            } else if (board->grid[nr][nc] == EMPTY) {
                left_open = 1;
                break;
            } else {
                break;
            }
        }

        total += get_pattern_score(count, left_open, right_open);
    }

    return total;
}

// 全局局面评估
int evaluate_board(Board* board, int my_side, int enemy_side) {
    int my_score = 0;
    int enemy_score = 0;
    int r, c;

    for (r = 0; r < BOARD_SIZE; r++) {
        for (c = 0; c < BOARD_SIZE; c++) {
            if (board->grid[r][c] == my_side) {
                // 临时假设落子来评估
                int old = board->grid[r][c];
                board->grid[r][c] = EMPTY;
                my_score += evaluate_point(board, r, c, my_side);
                board->grid[r][c] = old;
            } else if (board->grid[r][c] == enemy_side) {
                int old = board->grid[r][c];
                board->grid[r][c] = EMPTY;
                enemy_score += evaluate_point(board, r, c, enemy_side);
                board->grid[r][c] = old;
            }
        }
    }

    // 非对称权重：进攻略优于防守
    return my_score - (enemy_score * 85 / 100);
}

/* ==================== 搜索引擎 ==================== */

// 比较函数（用于qsort）
int compare_moves(const void* a, const void* b) {
    Move* ma = (Move*)a;
    Move* mb = (Move*)b;
    return mb->priority - ma->priority;  // 降序
}

// 生成候选走法（基于评分筛选）
int generate_candidates(SearchContext* ctx, Move* candidates, int for_side) {
    Board* board = ctx->board;
    int count = 0;
    int r, c;
    int attack, defense, total_value;
    int enemy = (for_side == BLACK) ? WHITE : BLACK;

    // 计算动态阈值：根据棋局阶段调整筛选标准
    int move_count = board->move_count;
    int threshold = MIN_SCORE_THRESHOLD;

    // 开局阶段：更宽松的筛选（允许更多候选位置）
    if (move_count < 12) {
        threshold = 0;  // 开局考虑所有位置
    } else if (move_count < 30) {
        threshold = MIN_SCORE_THRESHOLD;
    } else {
        // 中后局：提高阈值，聚焦高价值位置
        threshold = MIN_SCORE_THRESHOLD * 2;
    }

    for (r = 0; r < BOARD_SIZE; r++) {
        for (c = 0; c < BOARD_SIZE; c++) {
            if (board->grid[r][c] != EMPTY) continue;

            // 评估该点的攻击和防守价值
            attack = evaluate_point(board, r, c, for_side);
            defense = evaluate_point(board, r, c, enemy);

            // 计算综合价值（攻防权重 11:9）
            total_value = attack * 11 + defense * 9;

            // 基于评分的智能筛选：只保留有价值的位置
            if (total_value < threshold) continue;

            candidates[count].row = r;
            candidates[count].col = c;
            candidates[count].priority = total_value;
            count++;

            if (count >= 256) break;  // 防止溢出
        }
        if (count >= 256) break;
    }

    // 排序：优先级从高到低
    qsort(candidates, count, sizeof(Move), compare_moves);

    // 截断到MAX_CANDIDATES
    if (count > MAX_CANDIDATES) {
        count = MAX_CANDIDATES;
    }

    return count;
}

// 检查一步必胜
int find_winning_move(SearchContext* ctx, int side, Move* result) {
    Board* board = ctx->board;
    int r, c;

    for (r = 0; r < BOARD_SIZE; r++) {
        for (c = 0; c < BOARD_SIZE; c++) {
            if (board->grid[r][c] != EMPTY) continue;

            // 尝试落子
            place_stone(board, r, c, side);
            int win = check_five(board, r, c, side);
            undo_stone(board, r, c);

            if (win) {
                result->row = r;
                result->col = c;
                return 1;
            }
        }
    }

    return 0;
}

// 前向声明
int alpha_beta(SearchContext* ctx, int depth, int alpha, int beta, int is_max);

// 计算自适应搜索深度
int get_search_depth(Board* board) {
    int move_count = board->move_count;

    if (move_count < 10) {
        return 4;  // 开局：浅搜
    } else if (move_count < 80) {
        return 6;  // 中局：标准深度
    } else {
        return 8;  // 残局：深搜
    }
}

// Alpha-Beta搜索
int alpha_beta(SearchContext* ctx, int depth, int alpha, int beta, int is_max) {
    // 超时检查
    if (is_timeout(ctx->timer)) {
        return evaluate_board(ctx->board, ctx->my_side, ctx->enemy_side);
    }

    // 到达叶子节点
    if (depth == 0) {
        return evaluate_board(ctx->board, ctx->my_side, ctx->enemy_side);
    }

    Move candidates[256];
    int num_moves;
    int current_side = is_max ? ctx->my_side : ctx->enemy_side;
    int i, score;

    // 生成候选
    num_moves = generate_candidates(ctx, candidates, current_side);

    if (num_moves == 0) {
        return evaluate_board(ctx->board, ctx->my_side, ctx->enemy_side);
    }

    if (is_max) {
        // 己方走棋
        int max_eval = -INF;

        for (i = 0; i < num_moves; i++) {
            place_stone(ctx->board, candidates[i].row, candidates[i].col, ctx->my_side);

            // 立即检查胜利
            if (check_five(ctx->board, candidates[i].row, candidates[i].col, ctx->my_side)) {
                score = SCORE_FIVE - depth;  // 越快胜利越好
            } else {
                score = alpha_beta(ctx, depth - 1, alpha, beta, 0);
            }

            undo_stone(ctx->board, candidates[i].row, candidates[i].col);

            if (score > max_eval) max_eval = score;
            if (score > alpha) alpha = score;
            if (alpha >= beta) break;  // Beta剪枝

            if (is_timeout(ctx->timer)) break;
        }

        return max_eval;
    } else {
        // 对手走棋
        int min_eval = INF;

        for (i = 0; i < num_moves; i++) {
            place_stone(ctx->board, candidates[i].row, candidates[i].col, ctx->enemy_side);

            // 立即检查对手胜利
            if (check_five(ctx->board, candidates[i].row, candidates[i].col, ctx->enemy_side)) {
                score = -SCORE_FIVE + depth;  // 对手越晚胜利越好
            } else {
                score = alpha_beta(ctx, depth - 1, alpha, beta, 1);
            }

            undo_stone(ctx->board, candidates[i].row, candidates[i].col);

            if (score < min_eval) min_eval = score;
            if (score < beta) beta = score;
            if (alpha >= beta) break;  // Alpha剪枝

            if (is_timeout(ctx->timer)) break;
        }

        return min_eval;
    }
}

// 搜索最佳走法
void search_best_move(SearchContext* ctx, Move* best_move) {
    // 步骤1：检查一步必胜
    if (find_winning_move(ctx, ctx->my_side, best_move)) {
        return;
    }

    // 步骤2：检查必须防守
    if (find_winning_move(ctx, ctx->enemy_side, best_move)) {
        return;
    }

    // 步骤3：正常搜索
    Move candidates[256];
    int num_moves = generate_candidates(ctx, candidates, ctx->my_side);

    if (num_moves == 0) {
        // 兜底：返回中心附近
        best_move->row = BOARD_SIZE / 2;
        best_move->col = BOARD_SIZE / 2;
        return;
    }

    // 至少选第一个候选
    *best_move = candidates[0];

    int depth = get_search_depth(ctx->board);
    int best_score = -INF;
    int alpha = -INF;
    int beta = INF;
    int i, score;

    for (i = 0; i < num_moves; i++) {
        place_stone(ctx->board, candidates[i].row, candidates[i].col, ctx->my_side);

        if (check_five(ctx->board, candidates[i].row, candidates[i].col, ctx->my_side)) {
            score = SCORE_FIVE;
        } else {
            score = alpha_beta(ctx, depth - 1, alpha, beta, 0);
        }

        undo_stone(ctx->board, candidates[i].row, candidates[i].col);

        if (score > best_score) {
            best_score = score;
            *best_move = candidates[i];
        }

        if (score > alpha) alpha = score;

        if (is_timeout(ctx->timer)) {
            break;
        }
    }
}

/* ==================== 主控制和IO ==================== */

// 放置初始四子
void place_initial_stones(Board* board) {
    place_stone(board, 5, 5, WHITE);
    place_stone(board, 6, 6, WHITE);
    place_stone(board, 5, 6, BLACK);
    place_stone(board, 6, 5, BLACK);
}

// 处理START指令
void handle_start(const char* cmd) {
    int field;
    sscanf(cmd, "START %d", &field);

    my_color = field;
    enemy_color = (field == 1) ? 2 : 1;

    init_board(&game_board);
    place_initial_stones(&game_board);
    init_timer(&time_manager);

    printf("OK\n");
    fflush(stdout);
}

// 处理PLACE指令
void handle_place(const char* cmd) {
    int x, y;
    sscanf(cmd, "PLACE %d %d", &x, &y);
    place_stone(&game_board, x, y, enemy_color);
}

// 处理TURN指令
void handle_turn() {
    start_turn(&time_manager);

    SearchContext ctx;
    ctx.board = &game_board;
    ctx.timer = &time_manager;
    ctx.my_side = my_color;
    ctx.enemy_side = enemy_color;

    Move best_move;
    search_best_move(&ctx, &best_move);

    place_stone(&game_board, best_move.row, best_move.col, my_color);

    printf("%d %d\n", best_move.row, best_move.col);
    fflush(stdout);

    end_turn(&time_manager);
}

// 处理END指令
void handle_end(const char* cmd) {
    // 游戏结束，不需要特殊处理
}

/* ==================== 主函数 ==================== */

int main() {
    char command[128];

    init_board(&game_board);
    init_timer(&time_manager);

    while (fgets(command, sizeof(command), stdin)) {
        command[strcspn(command, "\n")] = 0;

        if (strncmp(command, "START", 5) == 0) {
            handle_start(command);
        } else if (strncmp(command, "PLACE", 5) == 0) {
            handle_place(command);
        } else if (strncmp(command, "TURN", 4) == 0) {
            handle_turn();
        } else if (strncmp(command, "END", 3) == 0) {
            handle_end(command);
            break;
        }
    }

    return 0;
}
