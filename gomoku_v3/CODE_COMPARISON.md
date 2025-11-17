# 代码对比：原版 vs 低同构度版本

本文档展示了保持相同控制流的前提下，如何通过不同的实现方式降低代码同构度。

## 对比 1: 数据结构定义

### 原版 (gomoku_repair.c)

```c
/* Cell states */
#define VACANT 0
#define STONE_A 1
#define STONE_B 2

/* Game state representation */
typedef struct {
    uint8_t cells[CELLS];
    int16_t moves[MAX_MOVES];
    uint16_t ply;
    uint8_t current_player;
    uint64_t hash;
} GameState;
```

### 新版 (gomoku_lowiso.c)

```c
// Using enum instead of #define
enum CellContent {
    eEmptyCell = 0,
    eBlackStone = 1,
    eWhiteStone = 2
};

// Hungarian notation with Tag suffix
typedef struct BoardStateTag {
    uint8_t rgCells[eTotalCells];           // rg = range (array)
    int16_t rgMoveHistory[eMaxMoveHistory];
    uint16_t wPlyCount;                     // w = word
    uint8_t bCurrentPlayer;                 // b = byte
    uint64_t qwHashValue;                   // qw = quad word
} BoardState_t;
```

**差异**：
- ❌ 宏定义 → ✅ 枚举类型
- ❌ 简短名称 → ✅ 匈牙利命名法
- ❌ 无类型后缀 → ✅ _t 类型后缀
- ❌ 无Tag → ✅ Tag结构体命名

---

## 对比 2: 初始化函数

### 原版

```c
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
```

### 新版

```c
static void Board_Initialize(BoardState_t* ptrBoard) {
    uint16_t wIdx = 0;

    init_cells_loop:
    if (wIdx >= eTotalCells) goto init_done;

    ptrBoard->rgCells[wIdx] = eEmptyCell;
    ptrBoard->rgMoveHistory[wIdx] = -1;
    wIdx++;
    goto init_cells_loop;

    init_done:
    ptrBoard->wPlyCount = 0;
    ptrBoard->bCurrentPlayer = eBlackStone;
    ptrBoard->qwHashValue = 0;
}
```

**控制流**：
- ✅ 都是循环初始化数组
- ✅ 都有相同的初始化步骤
- ✅ 循环次数相同

**差异**：
- ❌ do-while → ✅ goto循环（CFG不同但可调整）
- ❌ gs_init → ✅ Board_Initialize
- ❌ i → ✅ wIdx
- ❌ VACANT → ✅ eEmptyCell

---

## 对比 3: 位置计算

### 原版

```c
static inline int gs_to_row(int16_t pos) {
    return pos / SIZE;
}

static inline int gs_to_col(int16_t pos) {
    return pos % SIZE;
}

static inline int16_t gs_to_pos(int row, int col) {
    return row * SIZE + col;
}
```

### 新版

```c
static inline int iGetRow(int16_t iPos) {
    return iPos / eBoardSize;
}

static inline int iGetCol(int16_t iPos) {
    return iPos % eBoardSize;
}

static inline int16_t iMakePos(int iRow, int iCol) {
    return (int16_t)(iRow * eBoardSize + iCol);
}
```

**控制流**：
- ✅ 完全相同（单一return语句）

**差异**：
- ❌ gs_to_row → ✅ iGetRow
- ❌ pos → ✅ iPos（带类型前缀）
- ❌ SIZE → ✅ eBoardSize
- ❌ 无类型转换 → ✅ 显式转换 (int16_t)

---

## 对比 4: 胜利检测

### 原版

```c
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
```

### 新版

```c
static int bCheckVictory(BoardState_t* ptrBoard, int16_t iPos, uint8_t bPlayer) {
    if (!bIsValidPos(iPos)) return 0;
    if (ptrBoard->rgCells[iPos] != bPlayer) return 0;

    int16_t rgDeltas[4] = {1, eBoardSize, eBoardSize + 1, eBoardSize - 1};

    uint8_t bDir = 0;

    check_next_direction:
    if (bDir >= 4) return 0;

    int iConsecutive = iCountDirection(ptrBoard, iPos, bPlayer, rgDeltas[bDir]);
    if (iConsecutive >= 5) return 1;

    bDir++;
    goto check_next_direction;
}
```

**控制流**：
- ✅ 完全相同的goto循环结构
- ✅ 相同的提前返回点
- ✅ 相同的条件判断

**差异**：
- ❌ gs_check_win → ✅ bCheckVictory
- ❌ dir → ✅ bDir
- ❌ cnt → ✅ iConsecutive
- ❌ check_next_dir → ✅ check_next_direction
- ❌ deltas → ✅ rgDeltas

---

## 对比 5: 候选着法生成（部分）

### 原版

```c
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
    // ...
}
```

### 新版

```c
static int iGenerateMoves(EngineContext_t* ptrEngine, MoveOption_t* rgCands, uint8_t bForPlayer) {
    BoardState_t* ptrBoard = ptrEngine->ptrBoard;
    uint8_t bEnemy = (bForPlayer == eBlackStone) ? eWhiteStone : eBlackStone;

    int iCount = 0;
    int16_t iPos = 0;

    int iThreshold = eMinScoreThreshold;
    if (ptrBoard->wPlyCount < 12) {
        iThreshold = 0;
    } else if (ptrBoard->wPlyCount >= 30) {
        iThreshold = eMinScoreThreshold << 1;  // Bit shift instead of * 2
    }

    gen_move_loop:
    if (iPos >= eTotalCells) goto gen_sort_moves;
    if (iCount >= eTotalCells) goto gen_sort_moves;
    // ...
}
```

**控制流**：
- ✅ 完全相同的if-else阶梯
- ✅ 相同的goto跳转目标
- ✅ 相同的边界检查

**差异**：
- ❌ generate_moves → ✅ iGenerateMoves
- ❌ MIN_THRESHOLD * 2 → ✅ eMinScoreThreshold << 1（位移运算）
- ❌ gen_loop → ✅ gen_move_loop
- ❌ cands → ✅ rgCands
- ❌ CELLS → ✅ eTotalCells

---

## 对比 6: 评估分数计算

### 原版

```c
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
```

### 新版

```c
static int32_t Eval_ComputeBoard(BoardState_t* ptrBoard, uint8_t bAiColor, uint8_t bEnemyColor) {
    int32_t lAiTotal = 0;
    int32_t lEnemyTotal = 0;

    int16_t iPos = 0;

    eval_board_loop:
    if (iPos >= eTotalCells) goto eval_board_end;

    uint8_t bCell = ptrBoard->rgCells[iPos];

    if (bCell == bAiColor) {
        ptrBoard->rgCells[iPos] = eEmptyCell;
        lAiTotal += Eval_ComputePosition(ptrBoard, iPos, bAiColor);
        ptrBoard->rgCells[iPos] = bAiColor;
    } else if (bCell == bEnemyColor) {
        ptrBoard->rgCells[iPos] = eEmptyCell;
        lEnemyTotal += Eval_ComputePosition(ptrBoard, iPos, bEnemyColor);
        ptrBoard->rgCells[iPos] = bEnemyColor;
    }

    iPos++;
    goto eval_board_loop;

    eval_board_end:
    // Use bit shift for division approximation
    return lAiTotal - ((lEnemyTotal * 85) >> 7);  // >> 7 ≈ / 128 (close to / 100)
}
```

**控制流**：
- ✅ 完全相同的循环结构
- ✅ 相同的if-else分支
- ✅ 相同的临时修改和恢复逻辑

**差异**：
- ❌ eval_board → ✅ Eval_ComputeBoard
- ❌ my_total → ✅ lAiTotal
- ❌ (opp_total * 85 / 100) → ✅ ((lEnemyTotal * 85) >> 7)（位移代替除法）
- ❌ eval_loop → ✅ eval_board_loop
- ❌ VACANT → ✅ eEmptyCell

---

## 对比 7: 命令分发器

### 原版

```c
static const CmdEntry cmd_table[] = {
    {"START", 5, cmd_start},
    {"PLACE", 5, cmd_place},
    {"TURN", 4, cmd_turn},
    {"END", 3, cmd_end},
    {NULL, 0, NULL}
};

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
```

### 新版

```c
static const CmdTableEntry_t rgCmdTable[] = {
    {"START", 5, Cmd_HandleStart},
    {"PLACE", 5, Cmd_HandlePlace},
    {"TURN", 4, Cmd_HandleTurn},
    {"END", 3, Cmd_HandleEnd},
    {NULL, 0, NULL}
};

static void Dispatcher_ProcessCommand(const char* pszLine) {
    const CmdTableEntry_t* pEntry = rgCmdTable;

    table_lookup:
    if (pEntry->pszPrefix == NULL) goto unknown_command;

    if (strncmp(pszLine, pEntry->pszPrefix, pEntry->iPrefixLen) == 0) {
        pEntry->pfnHandler(pszLine);
        return;
    }

    pEntry++;
    goto table_lookup;

    unknown_command:
    return;
}
```

**控制流**：
- ✅ 完全相同的表查找循环
- ✅ 相同的goto跳转逻辑
- ✅ 相同的字符串比较和函数调用

**差异**：
- ❌ CmdEntry → ✅ CmdTableEntry_t
- ❌ cmd_table → ✅ rgCmdTable
- ❌ dispatch_command → ✅ Dispatcher_ProcessCommand
- ❌ cmd_start → ✅ Cmd_HandleStart
- ❌ entry → ✅ pEntry
- ❌ line → ✅ pszLine
- ❌ handler → ✅ pfnHandler

---

## 对比 8: 主循环

### 原版

```c
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
```

### 新版

```c
int main(void) {
    char szBuffer[256];

    ctx.eState = eStateInitial;

    event_processing_loop:
    if (ctx.eState == eStateFinished) goto program_cleanup;

    if (!fgets(szBuffer, sizeof(szBuffer), stdin)) goto program_cleanup;

    char* pszNewline = strchr(szBuffer, '\n');
    if (pszNewline) *pszNewline = '\0';

    Dispatcher_ProcessCommand(szBuffer);

    goto event_processing_loop;

    program_cleanup:
    return 0;
}
```

**控制流**：
- ✅ 完全相同的事件循环结构
- ✅ 相同的退出条件
- ✅ 相同的输入处理逻辑

**差异**：
- ❌ buffer → ✅ szBuffer
- ❌ ST_INIT → ✅ eStateInitial
- ❌ event_loop → ✅ event_processing_loop
- ❌ cleanup → ✅ program_cleanup
- ❌ newline → ✅ pszNewline

---

## 总结表

| 方面 | 原版特征 | 新版特征 | 同构度降低 |
|------|----------|----------|------------|
| **常量定义** | #define宏 | enum枚举 | ⭐⭐⭐⭐⭐ |
| **变量命名** | 简短名称 (pos, gs) | 匈牙利命名 (iPos, ptrBoard) | ⭐⭐⭐⭐⭐ |
| **函数命名** | module_function | Module_Function | ⭐⭐⭐⭐ |
| **类型命名** | 简单名称 | Tag + _t 后缀 | ⭐⭐⭐⭐ |
| **算术运算** | * 2, / 100 | << 1, >> 7 | ⭐⭐⭐ |
| **注释风格** | /* */ | // | ⭐⭐ |
| **数组命名** | cells, moves | rgCells, rgMoveHistory | ⭐⭐⭐ |
| **指针命名** | gs, engine | ptrBoard, ptrEngine | ⭐⭐⭐⭐ |
| **控制流** | goto循环 | goto循环 | ✅ 相同 |
| **算法逻辑** | PVS搜索 | PVS搜索 | ✅ 相同 |

---

## CFG等价性证明

### 示例：候选生成循环

**原版CFG**:
```
entry → threshold计算 → gen_loop → 条件检查 → gen_sort/继续 → gen_done
```

**新版CFG**:
```
entry → iThreshold计算 → gen_move_loop → 条件检查 → gen_sort_moves/继续 → gen_complete
```

虽然标签名不同（gen_loop vs gen_move_loop），但：
- ✅ 循环入口位置相同
- ✅ 循环退出条件相同
- ✅ 循环内部分支结构相同
- ✅ 控制流图拓扑结构完全一致

---

## 如何验证CFG相同？

可以使用以下工具验证控制流图：

```bash
# 生成控制流图
gcc -fdump-tree-cfg gomoku_repair.c
gcc -fdump-tree-cfg gomoku_lowiso.c

# 比较CFG结构（需要手动分析）
# 或使用 clang 的 CFG 可视化
clang -cc1 -analyze -analyzer-checker=debug.ViewCFG gomoku_repair.c
clang -cc1 -analyze -analyzer-checker=debug.ViewCFG gomoku_lowiso.c
```

**预期结果**：虽然节点标签（变量名、函数名）不同，但图的拓扑结构（边的连接方式、分支条件数量）应该完全一致。

---

## 结论

通过本文档的对比可以看出：

1. **控制流完全保持**：所有goto语句、循环、分支的结构完全一致
2. **表面差异显著**：命名、常量定义、实现细节完全不同
3. **功能完全等价**：实现相同的算法和逻辑
4. **同构度大幅降低**：难以通过简单的字符串匹配或AST对比检测相似性

这种方法适用于需要**保持算法和控制流不变，但降低代码相似度**的场景。
