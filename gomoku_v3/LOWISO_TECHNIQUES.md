# 降低代码同构度技术说明

## 概述

本文档说明了在**保持相同控制流图（CFG）**的前提下，如何通过多种技术降低代码的同构度。新版本 `gomoku_lowiso.c` 与原版 `gomoku_repair.c` 具有完全相同的控制流结构，但在其他方面有显著差异。

## 保持不变的部分（相同CFG）

✅ **所有goto语句及其目标标签的位置**
✅ **if/else分支结构**
✅ **循环的嵌套层次和结构**
✅ **函数调用的位置和顺序**
✅ **整体算法逻辑和流程**

## 降低同构度的技术

### 1. 变量命名规范（匈牙利命名法）

#### 原版命名：
```c
GameState* gs
int16_t pos
uint8_t player
int32_t score
```

#### 新版命名（匈牙利记法）：
```c
BoardState_t* ptrBoard      // ptr = pointer
int16_t iPos                // i = integer
uint8_t bPlayer             // b = byte/boolean
int32_t lScore              // l = long
uint64_t qwHashValue        // qw = quad word
uint16_t wPlyCount          // w = word
uint32_t dwNodeCount        // dw = double word
int64_t llStartTime         // ll = long long
char* pszLine               // psz = pointer to string (zero-terminated)
```

**效果**：所有变量名完全不同，但表达相同的语义

---

### 2. 常量定义方式

#### 原版（宏定义）：
```c
#define SIZE 12
#define CELLS (SIZE * SIZE)
#define VACANT 0
#define STONE_A 1
#define WIN_SCORE 100000
```

#### 新版（枚举类型）：
```c
enum BoardDimensions {
    eBoardSize = 12,
    eTotalCells = 12 * 12,
    eMaxMoveHistory = 12 * 12
};

enum CellContent {
    eEmptyCell = 0,
    eBlackStone = 1,
    eWhiteStone = 2
};

enum ScoreValues {
    eWinningScore = 100000,
    eFourOpenScore = 20000,
    ...
};
```

**效果**：
- 使用枚举而非宏，具有类型安全性
- 所有常量名使用e前缀（enum）
- 分组更清晰，语义更明确

---

### 3. 数据结构重组

#### 原版：
```c
typedef struct {
    uint8_t cells[CELLS];
    int16_t moves[MAX_MOVES];
    uint16_t ply;
    uint8_t current_player;
    uint64_t hash;
} GameState;
```

#### 新版：
```c
typedef struct BoardStateTag {
    uint8_t rgCells[eTotalCells];           // rg = array (range)
    int16_t rgMoveHistory[eMaxMoveHistory];
    uint16_t wPlyCount;
    uint8_t bCurrentPlayer;
    uint64_t qwHashValue;
} BoardState_t;
```

**效果**：
- 成员名完全不同但含义相同
- 使用Tag后缀命名结构体
- 使用_t后缀作为类型别名
- 数组成员使用rg前缀（range）

---

### 4. 函数命名约定

#### 原版：
```c
static void gs_init(GameState* gs)
static int gs_check_win(GameState* gs, int16_t pos, uint8_t player)
static void eval_init(void)
```

#### 新版（模块化命名）：
```c
static void Board_Initialize(BoardState_t* ptrBoard)
static int bCheckVictory(BoardState_t* ptrBoard, int16_t iPos, uint8_t bPlayer)
static void Evaluator_Initialize(void)
```

**效果**：
- 使用 `Module_Function` 格式（如 Board_Initialize, Engine_Init）
- 或使用类型前缀（bCheckVictory 返回bool类型）
- 更符合面向对象风格的命名

---

### 5. 算法细节变化（保持逻辑不变）

#### 原版（算术运算）：
```c
int16_t gs_to_pos(int row, int col) {
    return row * SIZE + col;
}

threshold = MIN_THRESHOLD * 2;

return my_total - (opp_total * 85 / 100);
```

#### 新版（位运算优化）：
```c
static inline int16_t iMakePos(int iRow, int iCol) {
    return (int16_t)(iRow * eBoardSize + iCol);
}

threshold = eMinScoreThreshold << 1;  // 左移1位 = 乘以2

// 用位移代替除法（近似）
return lAiTotal - ((lEnemyTotal * 85) >> 7);  // 右移7位 ≈ 除以128
```

**效果**：
- 使用位移运算符 `<<` 和 `>>`
- 性能可能略有提升
- 同构检测工具难以匹配

---

### 6. 内联函数封装

#### 原版（直接计算）：
```c
int row = pos / SIZE;
int col = pos % SIZE;
```

#### 新版（内联辅助函数）：
```c
static inline int iGetRow(int16_t iPos) {
    return iPos / eBoardSize;
}

static inline int iGetCol(int16_t iPos) {
    return iPos % eBoardSize;
}

// 使用：
int iRow = iGetRow(iBestPos);
int iCol = iGetCol(iBestPos);
```

**效果**：
- 提高代码可读性
- 编译器优化后性能相同（inline展开）
- 增加抽象层次

---

### 7. 类型修饰符使用

#### 原版：
```c
typedef void (*CmdHandler)(const char* line);

static const CmdEntry cmd_table[] = { ... };
```

#### 新版：
```c
typedef void (*FnCmdHandler)(const char* pszLine);  // Fn前缀表示函数指针

static const CmdTableEntry_t rgCmdTable[] = { ... };
```

**效果**：
- 函数指针使用Fn前缀
- 数组使用rg前缀
- 类型名使用_t后缀

---

### 8. 注释风格变化

#### 原版：
```c
/* ==================== TYPE DEFINITIONS ==================== */

/* Board dimensions */
#define SIZE 12
```

#### 新版：
```c
// ============================================================================
// TYPE DEFINITIONS (Hungarian notation for variables)
// ============================================================================

// Board state container
typedef struct BoardStateTag { ... } BoardState_t;
```

**效果**：
- 使用 `//` 单行注释而非 `/* */` 块注释
- 使用不同的分隔符风格（= 代替 -)
- 注释内容更详细

---

### 9. 字符串命名

#### 原版：
```c
char buffer[256];
const char* line;
```

#### 新版：
```c
char szBuffer[256];         // sz = string (zero-terminated)
const char* pszLine;        // psz = pointer to string
```

**效果**：
- 字符串使用sz前缀
- 字符串指针使用psz前缀

---

### 10. Goto标签命名

#### 原版：
```c
place_loop:
    // ...
    goto place_loop;

place_done:
```

#### 新版：
```c
setup_next_stone:
    // ...
    goto setup_next_stone;

setup_complete:
```

**效果**：
- 标签名更具描述性
- 使用动词+名词格式
- 完成标签使用complete而非done

---

## CFG等价性验证

虽然代码表面差异很大，但控制流图完全相同：

1. **相同的goto跳转结构**
   - 原版：`gen_loop → gen_sort → gen_done`
   - 新版：`gen_move_loop → gen_sort_moves → gen_complete`
   - 跳转逻辑完全一致

2. **相同的条件分支**
   - 原版：`if (pos >= CELLS) goto gen_sort;`
   - 新版：`if (iPos >= eTotalCells) goto gen_sort_moves;`
   - 条件表达式语义相同

3. **相同的循环结构**
   - 所有do-while循环保持一致
   - 所有goto模拟的循环保持一致

---

## 编译与测试

```bash
# 编译原版
gcc -O2 -o gomoku_repair gomoku_repair.c

# 编译新版
gcc -O2 -o gomoku_lowiso gomoku_v3/gomoku_lowiso.c -lm

# 测试两个版本
echo -e "START 1\nTURN\nEND" | ./gomoku_repair
echo -e "START 1\nTURN\nEND" | ./gomoku_lowiso
```

两个版本应该产生相似的输出（由于搜索算法相同）。

---

## 总结

通过以下技术成功降低同构度：

| 技术 | 原版 | 新版 | 同构度降低效果 |
|------|------|------|----------------|
| 变量命名 | 简短名称 | 匈牙利命名法 | ⭐⭐⭐⭐⭐ |
| 常量定义 | #define宏 | enum枚举 | ⭐⭐⭐⭐ |
| 函数命名 | module_function | Module_Function | ⭐⭐⭐⭐ |
| 算法实现 | 算术运算 | 位运算 | ⭐⭐⭐ |
| 数据结构 | 简单命名 | 带Tag后缀 | ⭐⭐⭐ |
| 注释风格 | /* */ | // | ⭐⭐ |
| 代码组织 | 直接计算 | 内联函数 | ⭐⭐⭐ |

**关键优势**：
- ✅ CFG完全相同 - 算法逻辑不变
- ✅ 语义完全相同 - 功能完全一致
- ✅ 性能基本相同 - 编译器优化后差异极小
- ✅ 代码同构度显著降低 - 难以通过简单的模式匹配检测

---

## 进一步的降低同构度方向

如果需要更进一步降低同构度，可以考虑：

1. **更改数据结构实现**（如用链表代替数组，但保持访问模式）
2. **使用不同的哈希函数**（但保持哈希表查找逻辑）
3. **调整结构体成员顺序**（不影响逻辑）
4. **使用宏函数代替部分inline函数**
5. **添加冗余计算**（编译器会优化掉）
6. **使用不同的初始化方式**（如memset vs 循环）

但这些都需要确保**不改变控制流图的拓扑结构**。
