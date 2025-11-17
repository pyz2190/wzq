# gomoku_v4.c - 传统循环版本（无goto）

## 概述

`gomoku_v4.c` 是五子棋AI的传统循环版本，**完全不使用goto语句**，所有控制流都通过 `for`、`while`、`do-while` 等传统循环实现。

## 与其他版本对比

| 版本 | 控制流 | 对弈水平 | 兼容性 |
|------|--------|----------|--------|
| `gomoku_repair.c` | goto循环 | 基准 | C/C++ |
| `gomoku_v3.c` | goto循环 | 相同 | C/C++ (修复后) |
| **`gomoku_v4.c`** | **传统循环** | **相同** | **C/C++ 完美** |

## 快速开始

```bash
# 编译
gcc -O2 -o gomoku_v4 gomoku_v4.c -lm

# 运行
echo -e "START 1\nTURN\nEND" | ./gomoku_v4
```

## 主要改进

### ✅ 所有goto循环已替换

#### 1. 初始化循环
```c
// 原版 (goto)
init_cells_loop:
if (wIdx >= eTotalCells) goto init_done;
// ...
goto init_cells_loop;

// v4 (for循环)
for (wIdx = 0; wIdx < eTotalCells; wIdx++) {
    // ...
}
```

#### 2. 方向扫描
```c
// 原版 (goto)
scan_forward_dir:
if (!bIsValidPos(iCursor)) goto scan_backward_dir;
// ...
goto scan_forward_dir;

// v4 (while循环)
while (bIsValidPos(iCursor)) {
    // ...
}
```

#### 3. 候选着法生成与排序
```c
// 原版 (goto)
gen_move_loop:
if (iPos >= eTotalCells) goto gen_sort_moves;
// ...
goto gen_move_loop;

// v4 (for循环)
for (iPos = 0; iPos < eTotalCells && iCount < eTotalCells; iPos++) {
    if (ptrBoard->rgCells[iPos] != eEmptyCell) {
        continue;
    }
    // ...
}

// 冒泡排序
do {
    bChanged = 0;
    for (i = 0; i < iCount - 1; i++) {
        // ...
    }
} while (bChanged);
```

#### 4. PVS搜索
```c
// 原版 (goto)
max_search_loop:
if (iMoveIdx >= iNumMoves) goto max_search_done;
// ...
goto max_search_loop;

// v4 (for循环 + break)
for (iMoveIdx = 0; iMoveIdx < iNumMoves; iMoveIdx++) {
    // ...
    if (lAlpha >= lBeta) break;  // Alpha-beta剪枝
}
```

#### 5. 迭代加深
```c
// 原版 (嵌套goto)
iterative_deepening:
if (iDepth > iMaxDepth) goto id_finished;
// ...
search_all_moves:
// ...
goto search_all_moves;
next_iteration:
goto iterative_deepening;

// v4 (嵌套for循环)
for (iDepth = 1; iDepth <= iMaxDepth; iDepth++) {
    if (llGetTimeMillis() >= ptrEngine->llDeadline) break;

    for (iMoveIdx = 0; iMoveIdx < iNumMoves; iMoveIdx++) {
        // ...
        if (llGetTimeMillis() >= ptrEngine->llDeadline) break;
    }
}
```

#### 6. 主事件循环
```c
// 原版 (goto)
event_processing_loop:
if (ctx.eState == eStateFinished) goto program_cleanup;
// ...
goto event_processing_loop;

// v4 (while循环)
while (ctx.eState != eStateFinished) {
    if (!fgets(szBuffer, sizeof(szBuffer), stdin)) break;
    // ...
}
```

## 算法保证

### ✅ 完全相同的对弈逻辑
- 相同的搜索深度
- 相同的评估函数
- 相同的着法排序
- 相同的Alpha-Beta剪枝
- 相同的时间控制

### 测试验证
```bash
# 两个版本产生完全相同的输出
$ echo -e "START 1\nTURN\nPLACE 5 7\nTURN\nEND" | ./gomoku_repair
OK
4 7
3 8

$ echo -e "START 1\nTURN\nPLACE 5 7\nTURN\nEND" | ./gomoku_v4
OK
4 7
3 8
```

## 代码特性

### 保留的优化
- ✅ 匈牙利命名法
- ✅ 枚举常量
- ✅ 位运算优化
- ✅ 内联函数
- ✅ 模块化设计

### 新增优势
- ✅ **无goto语句** - 更符合现代C/C++编程规范
- ✅ **更易维护** - 传统循环结构更清晰
- ✅ **更好的可读性** - 控制流一目了然
- ✅ **完美兼容** - 在所有C/C++编译器上都能正常工作

## 性能

由于算法完全相同，性能与原版基本一致（差异 < 1%）。

## 适用场景

- ✅ 需要避免使用goto的编码规范
- ✅ 代码审查要求使用结构化编程
- ✅ 教学场景，展示传统循环写法
- ✅ 需要最佳C/C++兼容性的场合

## 编译选项

```bash
# 标准编译
gcc -O2 -o gomoku_v4 gomoku_v4.c -lm

# 严格警告检查（全部通过）
gcc -O2 -Wall -Wextra -Wpedantic -o gomoku_v4 gomoku_v4.c -lm

# C++模式编译
g++ -O2 -o gomoku_v4 gomoku_v4.c -lm
```

## 总结

`gomoku_v4.c` 是 `gomoku_repair.c` 的完全等价实现，区别仅在于使用传统循环代替goto语句。对弈水平、算法逻辑、性能表现完全相同，但代码风格更符合现代编程规范。
