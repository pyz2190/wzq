#!/bin/bash
# 测试五子棋AI程序

echo "=== 测试1：START指令 ==="
echo "START 1" | ./gomoku_ai | head -1

echo ""
echo "=== 测试2：完整对局流程 ==="
(
echo "START 1"
echo "TURN"
echo "PLACE 5 7"
echo "TURN"
echo "END 0"
) | timeout 5 ./gomoku_ai

echo ""
echo "=== 编译信息 ==="
ls -lh gomoku_ai
file gomoku_ai
