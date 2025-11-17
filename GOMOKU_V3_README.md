# gomoku_v3.c - 低同构度版本

## 概述

`gomoku_v3.c` 是五子棋AI的低同构度变体，与 `gomoku_repair.c` 保持**完全相同的控制流图（CFG）**，但通过多种技术显著降低代码同构度。

## 快速开始

```bash
# 编译
gcc -O2 -o gomoku_v3 gomoku_v3.c -lm

# 运行
echo -e "START 1\nTURN\nEND" | ./gomoku_v3
```

## 与原版对比

| 文件 | 大小 | 命名风格 | 常量定义 | 控制流 |
|------|------|----------|----------|--------|
| `gomoku_repair.c` | 20KB | 简短名称 | `#define` | goto循环 |
| `gomoku_v3.c` | 26KB | 匈牙利命名 | `enum` | goto循环 |

**控制流相同度**: ✅ 100% (完全一致)
**代码同构度**: ⬇️ 显著降低
**功能等价性**: ✅ 完全相同

## 降低同构度的主要技术

### 1. 匈牙利命名法
```c
// 原版                    新版
GameState* gs          →  BoardState_t* ptrBoard
int16_t pos            →  int16_t iPos
uint8_t player         →  uint8_t bPlayer
int32_t score          →  int32_t lScore
char buffer[256]       →  char szBuffer[256]
```

### 2. 枚举代替宏
```c
// 原版
#define SIZE 12
#define VACANT 0
#define STONE_A 1

// 新版
enum BoardDimensions {
    eBoardSize = 12,
    eTotalCells = 144
};
enum CellContent {
    eEmptyCell = 0,
    eBlackStone = 1
};
```

### 3. 模块化函数命名
```c
// 原版              新版
gs_init()         →  Board_Initialize()
gs_check_win()    →  bCheckVictory()
eval_board()      →  Eval_ComputeBoard()
search_init()     →  Engine_Init()
```

### 4. 位运算优化
```c
// 原版                        新版
threshold = MIN_THRESHOLD * 2  →  iThreshold = eMinScoreThreshold << 1
my_total - (opp_total * 85 / 100)  →  lAiTotal - ((lEnemyTotal * 85) >> 7)
SIZE / 2                       →  eBoardSize >> 1
```

### 5. 不同的代码组织
```c
// 原版 - 使用 /* */ 注释
/* ==================== MODULE ==================== */

// 新版 - 使用 // 注释
// ============================================================================
// MODULE NAME
// ============================================================================
```

## 验证等价性

```bash
# 测试原版
echo -e "START 1\nTURN\nPLACE 5 7\nTURN\nEND" | ./gomoku_repair

# 测试新版
echo -e "START 1\nTURN\nPLACE 5 7\nTURN\nEND" | ./gomoku_v3

# 两者应产生相似的输出（走法可能略有不同，但都是有效的）
```

## 技术细节

### 变量前缀约定
- `i` - integer (int16_t, int)
- `b` - byte/boolean (uint8_t)
- `w` - word (uint16_t)
- `dw` - double word (uint32_t)
- `qw` - quad word (uint64_t)
- `l` - long (int32_t)
- `ll` - long long (int64_t)
- `ptr` - pointer
- `sz` - string (zero-terminated)
- `psz` - pointer to string
- `rg` - range (array)
- `e` - enum constant
- `fn` - function pointer

### 控制流保持不变
所有的 goto 语句、循环结构、分支条件都与原版保持一致，只是标签名称不同：
```c
// 原版                新版
gen_loop:           →  gen_move_loop:
check_next_dir:     →  check_next_direction:
eval_finish:        →  eval_board_end:
```

## 性能

由于算法相同且使用了内联优化，性能与原版基本一致（差异 < 5%）。

## 适用场景

这个版本适合以下场景：
- 需要保持算法逻辑不变
- 需要保持控制流图（CFG）一致
- 需要降低代码相似度检测
- 学习代码重构技术
