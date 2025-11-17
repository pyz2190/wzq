#!/bin/bash
# 测试重构后的AI

echo "Testing gomoku_ai_refactored..."

# 简单的协议测试
(
  echo "START 1"
  sleep 0.1
  echo "TURN"
  sleep 0.1
  echo "PLACE 6 7"
  echo "TURN"
  sleep 0.1
  echo "END"
) | timeout 10 ./gomoku_ai_refactored

echo ""
echo "Test completed!"
