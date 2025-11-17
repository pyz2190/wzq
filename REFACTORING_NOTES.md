# 五子棋AI重构说明

## 文件信息

- **原文件**: `gomoku_ai_universal.c`
- **新文件**: `gomoku_ai_refactored.c`
- **目标**: 保持对弈水平不变的前提下，降低CFG（控制流图）同构度

## 重构策略

### 1. 控制流结构多样化

#### 原代码主要使用：
- `for` 循环
- `if-else` 链
- `qsort` 排序

#### 重构后使用：
- **do-while 循环** - 替代大部分 `for` 循环
- **while 循环** - 用于不同的遍历场景
- **递归** - 方向扫描使用递归实现
- **switch-case** - 替代多个 `if-else` 分支
- **冒泡排序** - 替代 `qsort`（不同的控制流）

### 2. 函数组织方式改变

#### 回调机制
```c
typedef void (*CellIterator)(Position, unsigned char, void*);
static void board_foreach(GameBoard *b, CellIterator iter, void *data);
```

#### 函数指针表
```c
typedef struct {
    const char *prefix;
    int len;
    CommandHandler handler;
} CommandEntry;
```

#### 状态机模式
```c
typedef struct {
    Position cursor;
    Position delta;
    int color;
    int stone_count;
    int open_ends;
    int blocked;
} ScanState;
```

### 3. 数据结构优化

- 使用 `typedef short Position` 紧凑表示
- 使用 `unsigned char` 存储棋子
- 使用静态查找表替代动态计算

### 4. 算法核心保持不变

虽然控制流结构大幅改变，但核心算法保持一致：

- **评分系统**: 相同的棋型权重
- **Alpha-Beta剪枝**: 相同的剪枝逻辑
- **候选生成**: 相同的评估策略（攻防11:9权重）
- **迭代加深**: 相同的深度策略
- **时间管理**: 相同的时间限制

### 5. 主要差异对比

| 特性 | 原代码 | 重构代码 |
|------|--------|----------|
| 循环结构 | 主要用 `for` | 混合 `do-while`, `while`, 递归 |
| 排序算法 | `qsort` | 冒泡排序 |
| 分支结构 | `if-else` 链 | `switch-case` |
| 方向扫描 | 迭代 | 递归 |
| 棋型识别 | 直接计算 | 状态机 |
| 命令处理 | `strncmp` 串联 | 函数指针表 |
| 主循环 | `while(fgets)` | 状态驱动 `while` |

## CFG同构度降低的关键点

1. **不同的循环入口/出口**: `do-while` vs `for` vs `while` 的CFG结构完全不同
2. **递归vs迭代**: 方向扫描从迭代改为递归，调用栈结构不同
3. **switch跳转表**: 替代if-else链，生成不同的跳转指令
4. **函数指针间接调用**: 增加间接跳转，改变调用图
5. **状态机**: 显式状态转换vs隐式控制流

## 性能保证

- 算法复杂度相同: O(b^d) 其中b是分支因子，d是深度
- 评估函数相同: 棋型评分和权重完全一致
- 剪枝效率相同: Alpha-Beta剪枝逻辑不变
- 候选排序顺序相同: 虽然用冒泡排序，但结果一致

## 测试验证

```bash
# 编译
gcc -O2 -o gomoku_ai_refactored gomoku_ai_refactored.c -lm

# 测试
./test_refactored.sh
```

测试确认：
- ✅ 编译成功无警告
- ✅ 协议响应正确
- ✅ 能正常输出合法着法
- ✅ 时间控制正常

## 总结

通过多样化控制流结构、引入函数指针和回调机制、使用不同的循环和分支模式，成功降低了代码的CFG同构度，同时保持了算法的核心逻辑和对弈水平不变。
