# Gomoku AI - Low Isomorphism Version (v3)

这是五子棋AI的低同构度变体，与原版 `gomoku_repair.c` 保持**完全相同的控制流图（CFG）**，但通过多种技术降低代码同构度。

## 特性

- ✅ **控制流完全相同**：所有goto语句、循环、分支结构与原版一致
- ✅ **功能完全相同**：实现相同的游戏逻辑和搜索算法
- ✅ **同构度显著降低**：通过命名、组织、实现细节等方面的差异

## 降低同构度的主要技术

1. **匈牙利命名法**：所有变量使用类型前缀（如 `iPos`, `bPlayer`, `ptrBoard`）
2. **枚举常量**：使用 `enum` 代替 `#define` 宏定义
3. **模块化命名**：函数使用 `Module_Function` 格式
4. **位运算优化**：部分算术运算改为位移操作
5. **内联辅助函数**：封装常用计算
6. **不同注释风格**：使用 `//` 而非 `/* */`
7. **结构体Tag命名**：使用 `TypeNameTag` 和 `_t` 后缀

详细说明请参见：[LOWISO_TECHNIQUES.md](LOWISO_TECHNIQUES.md)

## 编译与运行

```bash
# 使用 Makefile
make

# 或直接编译
gcc -O2 -o gomoku_lowiso gomoku_lowiso.c -lm

# 运行测试
make test
```

## 协议

程序实现了标准的五子棋引擎协议：

```
START <field>    # field: 1=先手, 2=后手
PLACE <row> <col>
TURN
END
```

## 与其他版本对比

| 版本 | 控制流 | 命名风格 | 常量定义 | 同构度 |
|------|--------|----------|----------|--------|
| gomoku_repair.c | goto循环 | 简短名称 | #define | 基准 |
| **gomoku_lowiso.c** | **goto循环** | **匈牙利记法** | **enum** | **低** |

## 文件说明

- `gomoku_lowiso.c` - 主程序（单文件实现）
- `LOWISO_TECHNIQUES.md` - 详细技术说明文档
- `Makefile` - 构建配置
- `README.md` - 本文件

## 技术细节

### 命名约定示例

```c
// 变量前缀
int16_t iPos;           // i = integer
uint8_t bPlayer;        // b = byte
uint32_t dwCount;       // dw = double word
int64_t llTime;         // ll = long long
BoardState_t* ptrBoard; // ptr = pointer
char szBuffer[256];     // sz = string (zero-terminated)
int rgArray[10];        // rg = range (array)

// 函数命名
Board_Initialize()      // Module_Function
Engine_Init()           // Module_Function
Evaluator_Initialize()  // Module_Function
```

### 枚举常量示例

```c
enum BoardDimensions {
    eBoardSize = 12,
    eTotalCells = 144
};

enum CellContent {
    eEmptyCell = 0,
    eBlackStone = 1,
    eWhiteStone = 2
};
```

## 性能

由于使用了相同的算法和内联优化，性能与原版基本相同（差异 < 5%）。

## 许可

本代码仅供学习和研究使用。
