/*
 * Five-in-a-Row AI Engine - Traditional Loop Version
 *
 * Control Flow: Traditional while/for loops (no goto)
 * Features: Hungarian notation, enum constants, modular naming
 *
 * Build: gcc -O2 -o gomoku_v4 gomoku_v4.c -lm
 */

#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#ifdef _WIN32
#include <windows.h>
#endif

// ============================================================================
// CONSTANT ENUMERATIONS (instead of #define macros)
// ============================================================================

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
    eFourBlockedScore = 5000,
    eThreeOpenScore = 3000,
    eThreeBlockedScore = 800,
    eTwoOpenScore = 200,
    eTwoBlockedScore = 50,
    eSingleScore = 10
};

enum SearchLimits {
    eMaxSearchDepth = 12,
    eMaxCandidates = 15,
    eMinScoreThreshold = 5
};

enum TimeLimitsMs {
    eTurnTimeLimit = 1900,
    eTotalTimeLimit = 88000
};

enum HashTableConfig {
    eHashTableEntries = 1048576
};

// Direction encoding using bit positions
enum DirectionFlags {
    eDirHorizontal = 0,
    eDirVertical = 1,
    eDirDiagonal1 = 2,
    eDirDiagonal2 = 3,
    eDirCount = 4
};

// ============================================================================
// TYPE DEFINITIONS (Hungarian notation for variables)
// ============================================================================

// Board state container
typedef struct BoardStateTag {
    uint8_t rgCells[eTotalCells];           // Cell array
    int16_t rgMoveHistory[eMaxMoveHistory]; // Move history
    uint16_t wPlyCount;                     // Ply counter
    uint8_t bCurrentPlayer;                 // Current player
    uint64_t qwHashValue;                   // Zobrist hash
} BoardState_t;

// Move candidate with score
typedef struct MoveOptionTag {
    int16_t iPosition;                      // Board position
    int32_t lScore;                         // Evaluated score
} MoveOption_t;

// Transposition table entry
typedef struct HashRecordTag {
    uint64_t qwKey;                         // Hash key
    int32_t lStoredScore;                   // Cached score
    uint8_t bDepthLevel;                    // Search depth
    uint8_t bEntryType;                     // Entry flags
} HashRecord_t;

// Search engine context
typedef struct EngineContextTag {
    BoardState_t* ptrBoard;                 // Board reference
    HashRecord_t* ptrHashTable;             // Hash table
    uint64_t rgqwZobrist[eTotalCells][3];   // Zobrist numbers
    uint8_t bAiColor;                       // AI player color
    uint8_t bEnemyColor;                    // Opponent color
    int64_t llStartTime;                    // Search start time
    int64_t llDeadline;                     // Time deadline
    uint32_t dwNodeCount;                   // Nodes visited
} EngineContext_t;

// State machine states
typedef enum {
    eStateInitial,
    eStateReady,
    eStateComputing,
    eStateFinished
} MachineState_e;

// Pattern descriptor
typedef struct PatternInfoTag {
    uint8_t bLength;                        // Stone count
    uint8_t bGapCount;                      // Gap count
    uint8_t bBlockCount;                    // Blocked ends
} PatternInfo_t;

// ============================================================================
// BOARD POSITION UTILITIES (inline helpers)
// ============================================================================

// Extract row from linear position using bit shift
static inline int iGetRow(int16_t iPos) {
    return iPos / eBoardSize;
}

// Extract column from linear position
static inline int iGetCol(int16_t iPos) {
    return iPos % eBoardSize;
}

// Compute linear position from row/col using multiplication
static inline int16_t iMakePos(int iRow, int iCol) {
    return (int16_t)(iRow * eBoardSize + iCol);
}

// Validate position bounds using comparison
static inline int bIsValidPos(int16_t iPos) {
    return (iPos >= 0 && iPos < eTotalCells) ? 1 : 0;
}

// Safe cell retrieval with bounds checking
static inline uint8_t bGetCell(BoardState_t* ptrBoard, int16_t iPos) {
    return bIsValidPos(iPos) ? ptrBoard->rgCells[iPos] : (eWhiteStone + 1);
}

// ============================================================================
// BOARD STATE MANAGEMENT
// ============================================================================

// Initialize empty board state
static void Board_Initialize(BoardState_t* ptrBoard) {
    uint16_t wIdx;

    for (wIdx = 0; wIdx < eTotalCells; wIdx++) {
        ptrBoard->rgCells[wIdx] = eEmptyCell;
        ptrBoard->rgMoveHistory[wIdx] = -1;
    }

    ptrBoard->wPlyCount = 0;
    ptrBoard->bCurrentPlayer = eBlackStone;
    ptrBoard->qwHashValue = 0;
}

// Setup initial 4 stones configuration
static void Board_SetupOpening(BoardState_t* ptrBoard) {
    int16_t rgInitialPos[4] = {
        iMakePos(5, 5),
        iMakePos(6, 6),
        iMakePos(5, 6),
        iMakePos(6, 5)
    };

    uint8_t rgInitialColors[4] = {eWhiteStone, eWhiteStone, eBlackStone, eBlackStone};
    uint8_t bIdx;

    for (bIdx = 0; bIdx < 4; bIdx++) {
        ptrBoard->rgCells[rgInitialPos[bIdx]] = rgInitialColors[bIdx];
        ptrBoard->rgMoveHistory[ptrBoard->wPlyCount++] = rgInitialPos[bIdx];
    }

    ptrBoard->bCurrentPlayer = eBlackStone;
}

// Execute move on board
static void Board_MakeMove(BoardState_t* ptrBoard, int16_t iPos, uint8_t bPlayer) {
    ptrBoard->rgCells[iPos] = bPlayer;
    ptrBoard->rgMoveHistory[ptrBoard->wPlyCount] = iPos;
    ptrBoard->wPlyCount++;
    ptrBoard->bCurrentPlayer = (bPlayer == eBlackStone) ? eWhiteStone : eBlackStone;
}

// Retract last move
static void Board_UndoMove(BoardState_t* ptrBoard) {
    if (ptrBoard->wPlyCount == 0) return;

    ptrBoard->wPlyCount--;
    int16_t iPos = ptrBoard->rgMoveHistory[ptrBoard->wPlyCount];
    ptrBoard->rgCells[iPos] = eEmptyCell;

    uint8_t bPrevious = ptrBoard->bCurrentPlayer;
    ptrBoard->bCurrentPlayer = (bPrevious == eBlackStone) ? eWhiteStone : eBlackStone;
}

// Count consecutive stones in direction
static int iCountDirection(BoardState_t* ptrBoard, int16_t iPos, uint8_t bPlayer, int16_t iDelta) {
    int16_t iCount = 1;
    int16_t iCursor;
    int iRow1, iRow2;

    // Forward scan
    iCursor = iPos + iDelta;
    while (bIsValidPos(iCursor)) {
        iRow1 = iGetRow(iPos);
        iRow2 = iGetRow(iCursor);

        if (iDelta == 1 || iDelta == -1) {
            if (iRow1 != iRow2) break;
        }

        if (ptrBoard->rgCells[iCursor] != bPlayer) break;

        iCount++;
        iCursor += iDelta;
    }

    // Backward scan
    iCursor = iPos - iDelta;
    while (bIsValidPos(iCursor)) {
        iRow1 = iGetRow(iPos);
        iRow2 = iGetRow(iCursor);

        if (iDelta == 1 || iDelta == -1) {
            if (iRow1 != iRow2) break;
        }

        if (ptrBoard->rgCells[iCursor] != bPlayer) break;

        iCount++;
        iCursor -= iDelta;
    }

    return iCount;
}

// Check for five-in-a-row victory
static int bCheckVictory(BoardState_t* ptrBoard, int16_t iPos, uint8_t bPlayer) {
    int16_t rgDeltas[4] = {1, eBoardSize, eBoardSize + 1, eBoardSize - 1};
    uint8_t bDir;
    int iConsecutive;

    if (!bIsValidPos(iPos)) return 0;
    if (ptrBoard->rgCells[iPos] != bPlayer) return 0;

    for (bDir = 0; bDir < 4; bDir++) {
        iConsecutive = iCountDirection(ptrBoard, iPos, bPlayer, rgDeltas[bDir]);
        if (iConsecutive >= 5) return 1;
    }

    return 0;
}

// ============================================================================
// PATTERN EVALUATION ENGINE
// ============================================================================

static int32_t rglPatternScores[6][3];
static int bEvaluatorReady = 0;

// Initialize pattern score lookup table
static void Evaluator_Initialize(void) {
    uint8_t bLen, bOpen;

    if (bEvaluatorReady) return;

    for (bLen = 0; bLen < 6; bLen++) {
        for (bOpen = 0; bOpen < 3; bOpen++) {
            rglPatternScores[bLen][bOpen] = 0;
        }
    }

    // Five stones - guaranteed win
    rglPatternScores[5][0] = eWinningScore;
    rglPatternScores[5][1] = eWinningScore;
    rglPatternScores[5][2] = eWinningScore;

    // Four stones patterns
    rglPatternScores[4][2] = eFourOpenScore;
    rglPatternScores[4][1] = eFourBlockedScore;
    rglPatternScores[4][0] = 0;

    // Three stones patterns
    rglPatternScores[3][2] = eThreeOpenScore;
    rglPatternScores[3][1] = eThreeBlockedScore;
    rglPatternScores[3][0] = 0;

    // Two stones patterns
    rglPatternScores[2][2] = eTwoOpenScore;
    rglPatternScores[2][1] = eTwoBlockedScore;
    rglPatternScores[2][0] = 0;

    // Single stones
    rglPatternScores[1][1] = eSingleScore;
    rglPatternScores[1][2] = eSingleScore;

    bEvaluatorReady = 1;
}

// Pattern scanner state
typedef struct ScannerStateTag {
    int16_t iCursor;
    int16_t iDelta;
    uint8_t bPlayer;
    BoardState_t* ptrBoard;
    uint8_t bStoneCount;
    uint8_t bGapCount;
    uint8_t bBlocked;
} ScannerState_t;

static void Scanner_Init(ScannerState_t* ptrScanner, BoardState_t* ptrBoard,
                        int16_t iPos, int16_t iDelta, uint8_t bPlayer) {
    ptrScanner->ptrBoard = ptrBoard;
    ptrScanner->iCursor = iPos;
    ptrScanner->iDelta = iDelta;
    ptrScanner->bPlayer = bPlayer;
    ptrScanner->bStoneCount = 0;
    ptrScanner->bGapCount = 0;
    ptrScanner->bBlocked = 0;
}

static int Scanner_Advance(ScannerState_t* ptrScanner) {
    int16_t iNext = ptrScanner->iCursor + ptrScanner->iDelta;

    if (!bIsValidPos(iNext)) {
        ptrScanner->bBlocked = 1;
        return 0;
    }

    int iR1 = iGetRow(ptrScanner->iCursor);
    int iR2 = iGetRow(iNext);

    if (ptrScanner->iDelta == 1 || ptrScanner->iDelta == -1) {
        if (iR1 != iR2) {
            ptrScanner->bBlocked = 1;
            return 0;
        }
    }

    uint8_t bCell = bGetCell(ptrScanner->ptrBoard, iNext);

    if (bCell == ptrScanner->bPlayer) {
        ptrScanner->bStoneCount++;
        ptrScanner->iCursor = iNext;
        return 1;
    } else if (bCell == eEmptyCell) {
        if (ptrScanner->bStoneCount > 0 && ptrScanner->bGapCount == 0) {
            ptrScanner->bGapCount = 1;
            ptrScanner->iCursor = iNext;
            return 1;
        } else {
            return 0;
        }
    } else {
        ptrScanner->bBlocked = 1;
        return 0;
    }
}

static void Scanner_AnalyzeDirection(BoardState_t* ptrBoard, int16_t iPos, int16_t iDelta,
                                     uint8_t bPlayer, uint8_t* pbTotalCount, uint8_t* pbOpenEnds) {
    ScannerState_t forward, backward;

    Scanner_Init(&forward, ptrBoard, iPos, iDelta, bPlayer);
    while (Scanner_Advance(&forward)) {
        // Continue scanning forward
    }

    Scanner_Init(&backward, ptrBoard, iPos, -iDelta, bPlayer);
    while (Scanner_Advance(&backward)) {
        // Continue scanning backward
    }

    *pbTotalCount = 1 + forward.bStoneCount + backward.bStoneCount;
    *pbOpenEnds = 0;

    if (!forward.bBlocked) (*pbOpenEnds)++;
    if (!backward.bBlocked) (*pbOpenEnds)++;
}

// Evaluate position for player
static int32_t Eval_ComputePosition(BoardState_t* ptrBoard, int16_t iPos, uint8_t bPlayer) {
    int16_t rgDeltas[4];
    int32_t lTotalScore;
    uint8_t bDirIdx;
    uint8_t bCount, bOpens;
    uint8_t bCntIdx, bOpnIdx;

    if (!bIsValidPos(iPos)) return 0;
    if (ptrBoard->rgCells[iPos] != eEmptyCell) return 0;

    rgDeltas[0] = 1;
    rgDeltas[1] = eBoardSize;
    rgDeltas[2] = eBoardSize + 1;
    rgDeltas[3] = eBoardSize - 1;

    lTotalScore = 0;

    for (bDirIdx = 0; bDirIdx < 4; bDirIdx++) {
        ptrBoard->rgCells[iPos] = bPlayer;

        Scanner_AnalyzeDirection(ptrBoard, iPos, rgDeltas[bDirIdx], bPlayer, &bCount, &bOpens);

        ptrBoard->rgCells[iPos] = eEmptyCell;

        bCntIdx = (bCount > 5) ? 5 : bCount;
        bOpnIdx = (bOpens > 2) ? 2 : bOpens;

        lTotalScore += rglPatternScores[bCntIdx][bOpnIdx];
    }

    return lTotalScore;
}

// Full board evaluation
static int32_t Eval_ComputeBoard(BoardState_t* ptrBoard, uint8_t bAiColor, uint8_t bEnemyColor) {
    int32_t lAiTotal;
    int32_t lEnemyTotal;
    int16_t iPos;
    uint8_t bCell;

    lAiTotal = 0;
    lEnemyTotal = 0;

    for (iPos = 0; iPos < eTotalCells; iPos++) {
        bCell = ptrBoard->rgCells[iPos];

        if (bCell == bAiColor) {
            ptrBoard->rgCells[iPos] = eEmptyCell;
            lAiTotal += Eval_ComputePosition(ptrBoard, iPos, bAiColor);
            ptrBoard->rgCells[iPos] = bAiColor;
        } else if (bCell == bEnemyColor) {
            ptrBoard->rgCells[iPos] = eEmptyCell;
            lEnemyTotal += Eval_ComputePosition(ptrBoard, iPos, bEnemyColor);
            ptrBoard->rgCells[iPos] = bEnemyColor;
        }
    }

    // Use bit shift for division by power of 2
    return lAiTotal - ((lEnemyTotal * 85) >> 7);  // Divide by 128 instead of 100
}

// ============================================================================
// SEARCH ENGINE
// ============================================================================

// Time measurement
static int64_t llGetTimeMillis(void) {
#if defined(_WIN32) || defined(WIN32)
    return (int64_t)GetTickCount();
#elif defined(__linux__) || defined(__unix__)
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
#else
    return (int64_t)(clock() * 1000.0 / CLOCKS_PER_SEC);
#endif
}

// Zobrist hash initialization
static void Engine_InitZobrist(EngineContext_t* ptrEngine) {
    uint64_t qwSeed = 0x123456789ABCDEFULL;
    uint16_t wPos;
    uint8_t bPlayer;

    for (wPos = 0; wPos < eTotalCells; wPos++) {
        for (bPlayer = 0; bPlayer < 3; bPlayer++) {
            qwSeed = qwSeed * 6364136223846793005ULL + 1442695040888963407ULL;
            ptrEngine->rgqwZobrist[wPos][bPlayer] = qwSeed;
        }
    }
}

static void Engine_Init(EngineContext_t* ptrEngine, BoardState_t* ptrBoard, uint8_t bAiColor) {
    ptrEngine->ptrBoard = ptrBoard;
    ptrEngine->bAiColor = bAiColor;
    ptrEngine->bEnemyColor = (bAiColor == eBlackStone) ? eWhiteStone : eBlackStone;
    ptrEngine->dwNodeCount = 0;

    ptrEngine->ptrHashTable = (HashRecord_t*)calloc(eHashTableEntries, sizeof(HashRecord_t));

    Engine_InitZobrist(ptrEngine);
}

static void Engine_Cleanup(EngineContext_t* ptrEngine) {
    if (ptrEngine->ptrHashTable) {
        free(ptrEngine->ptrHashTable);
        ptrEngine->ptrHashTable = NULL;
    }
}

// Generate move candidates
static int iGenerateMoves(EngineContext_t* ptrEngine, MoveOption_t* rgCands, uint8_t bForPlayer) {
    BoardState_t* ptrBoard;
    uint8_t bEnemy;
    int iCount;
    int16_t iPos;
    int iThreshold;
    int32_t lAttack, lDefense, lCombined;
    int bChanged;
    int i;
    MoveOption_t temp;

    ptrBoard = ptrEngine->ptrBoard;
    bEnemy = (bForPlayer == eBlackStone) ? eWhiteStone : eBlackStone;

    iCount = 0;

    iThreshold = eMinScoreThreshold;
    if (ptrBoard->wPlyCount < 12) {
        iThreshold = 0;
    } else if (ptrBoard->wPlyCount >= 30) {
        iThreshold = eMinScoreThreshold << 1;  // Multiply by 2 using bit shift
    }

    // Generate candidate moves
    for (iPos = 0; iPos < eTotalCells && iCount < eTotalCells; iPos++) {
        if (ptrBoard->rgCells[iPos] != eEmptyCell) {
            continue;
        }

        lAttack = Eval_ComputePosition(ptrBoard, iPos, bForPlayer);
        lDefense = Eval_ComputePosition(ptrBoard, iPos, bEnemy);

        lCombined = lAttack * 11 + lDefense * 9;

        if (lCombined < iThreshold) {
            continue;
        }

        rgCands[iCount].iPosition = iPos;
        rgCands[iCount].lScore = lCombined;
        iCount++;
    }

    // Bubble sort candidates by score
    if (iCount <= 1) {
        return iCount;
    }

    do {
        bChanged = 0;
        for (i = 0; i < iCount - 1; i++) {
            if (rgCands[i].lScore < rgCands[i + 1].lScore) {
                temp = rgCands[i];
                rgCands[i] = rgCands[i + 1];
                rgCands[i + 1] = temp;
                bChanged = 1;
            }
        }
    } while (bChanged);

    return (iCount > eMaxCandidates) ? eMaxCandidates : iCount;
}

// Find immediate winning or blocking move
static int16_t iFindCriticalMove(EngineContext_t* ptrEngine, uint8_t bPlayer) {
    BoardState_t* ptrBoard = ptrEngine->ptrBoard;
    int16_t iPos;
    int bWin;

    for (iPos = 0; iPos < eTotalCells; iPos++) {
        if (ptrBoard->rgCells[iPos] == eEmptyCell) {
            Board_MakeMove(ptrBoard, iPos, bPlayer);
            bWin = bCheckVictory(ptrBoard, iPos, bPlayer);
            Board_UndoMove(ptrBoard);

            if (bWin) return iPos;
        }
    }

    return -1;
}

// Principal Variation Search
static int32_t lPvSearch(EngineContext_t* ptrEngine, int iDepth, int32_t lAlpha, int32_t lBeta, int bIsPv);

static int32_t lPvSearch(EngineContext_t* ptrEngine, int iDepth, int32_t lAlpha, int32_t lBeta, int bIsPv) {
    MoveOption_t rgCandidates[eTotalCells];
    uint8_t bCurrent;
    int iNumMoves;
    int bMaximizing;
    int32_t lBest;
    int iMoveIdx;
    int bIsFirst;
    int16_t iPos;
    int32_t lScore;

    if ((ptrEngine->dwNodeCount & 1023) == 0) {
        if (llGetTimeMillis() >= ptrEngine->llDeadline) {
            return Eval_ComputeBoard(ptrEngine->ptrBoard, ptrEngine->bAiColor, ptrEngine->bEnemyColor);
        }
    }

    ptrEngine->dwNodeCount++;

    if (iDepth <= 0) {
        return Eval_ComputeBoard(ptrEngine->ptrBoard, ptrEngine->bAiColor, ptrEngine->bEnemyColor);
    }

    bCurrent = ptrEngine->ptrBoard->bCurrentPlayer;
    iNumMoves = iGenerateMoves(ptrEngine, rgCandidates, bCurrent);

    if (iNumMoves == 0) {
        return Eval_ComputeBoard(ptrEngine->ptrBoard, ptrEngine->bAiColor, ptrEngine->bEnemyColor);
    }

    bMaximizing = (bCurrent == ptrEngine->bAiColor) ? 1 : 0;

    if (bMaximizing) {
        lBest = INT_MIN;
        bIsFirst = 1;

        for (iMoveIdx = 0; iMoveIdx < iNumMoves; iMoveIdx++) {
            iPos = rgCandidates[iMoveIdx].iPosition;

            Board_MakeMove(ptrEngine->ptrBoard, iPos, bCurrent);

            if (bCheckVictory(ptrEngine->ptrBoard, iPos, bCurrent)) {
                Board_UndoMove(ptrEngine->ptrBoard);
                return eWinningScore - iDepth;
            }

            if (bIsFirst) {
                lScore = lPvSearch(ptrEngine, iDepth - 1, lAlpha, lBeta, bIsPv);
                bIsFirst = 0;
            } else {
                lScore = lPvSearch(ptrEngine, iDepth - 1, lAlpha, lAlpha + 1, 0);

                if (lScore > lAlpha && lScore < lBeta && bIsPv) {
                    lScore = lPvSearch(ptrEngine, iDepth - 1, lAlpha, lBeta, 1);
                }
            }

            Board_UndoMove(ptrEngine->ptrBoard);

            lBest = (lScore > lBest) ? lScore : lBest;
            lAlpha = (lScore > lAlpha) ? lScore : lAlpha;

            if (lAlpha >= lBeta) break;
        }

        return lBest;
    } else {
        lBest = INT_MAX;
        bIsFirst = 1;

        for (iMoveIdx = 0; iMoveIdx < iNumMoves; iMoveIdx++) {
            iPos = rgCandidates[iMoveIdx].iPosition;

            Board_MakeMove(ptrEngine->ptrBoard, iPos, bCurrent);

            if (bCheckVictory(ptrEngine->ptrBoard, iPos, bCurrent)) {
                Board_UndoMove(ptrEngine->ptrBoard);
                return -eWinningScore + iDepth;
            }

            if (bIsFirst) {
                lScore = lPvSearch(ptrEngine, iDepth - 1, lAlpha, lBeta, bIsPv);
                bIsFirst = 0;
            } else {
                lScore = lPvSearch(ptrEngine, iDepth - 1, lBeta - 1, lBeta, 0);

                if (lScore > lAlpha && lScore < lBeta && bIsPv) {
                    lScore = lPvSearch(ptrEngine, iDepth - 1, lAlpha, lBeta, 1);
                }
            }

            Board_UndoMove(ptrEngine->ptrBoard);

            lBest = (lScore < lBest) ? lScore : lBest;
            lBeta = (lScore < lBeta) ? lScore : lBeta;

            if (lAlpha >= lBeta) break;
        }

        return lBest;
    }
}

// Iterative deepening search
static int16_t iSearchBestMove(EngineContext_t* ptrEngine, int iTimeLimitMs) {
    int16_t iWinMove, iBlockMove;
    MoveOption_t rgCandidates[eTotalCells];
    int iNumMoves;
    int16_t iBestMove;
    int iMaxDepth;
    int iDepth;
    int32_t lBestScore, lAlpha, lBeta;
    int iMoveIdx;
    int16_t iPos;
    int32_t lScore;

    ptrEngine->llStartTime = llGetTimeMillis();
    ptrEngine->llDeadline = ptrEngine->llStartTime + iTimeLimitMs;
    ptrEngine->dwNodeCount = 0;

    iWinMove = iFindCriticalMove(ptrEngine, ptrEngine->bAiColor);
    if (iWinMove >= 0) return iWinMove;

    iBlockMove = iFindCriticalMove(ptrEngine, ptrEngine->bEnemyColor);
    if (iBlockMove >= 0) return iBlockMove;

    iNumMoves = iGenerateMoves(ptrEngine, rgCandidates, ptrEngine->bAiColor);

    if (iNumMoves == 0) return (eBoardSize >> 1) * eBoardSize + (eBoardSize >> 1);

    iBestMove = rgCandidates[0].iPosition;

    iMaxDepth = 6;
    if (ptrEngine->ptrBoard->wPlyCount < 10) {
        iMaxDepth = 4;
    } else if (ptrEngine->ptrBoard->wPlyCount >= 80) {
        iMaxDepth = 8;
    }

    // Iterative deepening loop
    for (iDepth = 1; iDepth <= iMaxDepth; iDepth++) {
        if (llGetTimeMillis() >= ptrEngine->llDeadline) break;

        lBestScore = INT_MIN;
        lAlpha = INT_MIN;
        lBeta = INT_MAX;

        for (iMoveIdx = 0; iMoveIdx < iNumMoves; iMoveIdx++) {
            iPos = rgCandidates[iMoveIdx].iPosition;

            Board_MakeMove(ptrEngine->ptrBoard, iPos, ptrEngine->bAiColor);

            if (bCheckVictory(ptrEngine->ptrBoard, iPos, ptrEngine->bAiColor)) {
                Board_UndoMove(ptrEngine->ptrBoard);
                return iPos;
            }

            lScore = lPvSearch(ptrEngine, iDepth - 1, lAlpha, lBeta, 1);

            Board_UndoMove(ptrEngine->ptrBoard);

            if (lScore > lBestScore) {
                lBestScore = lScore;
                iBestMove = iPos;
            }

            lAlpha = (lScore > lAlpha) ? lScore : lAlpha;

            if (llGetTimeMillis() >= ptrEngine->llDeadline) break;
        }
    }

    return iBestMove;
}

// ============================================================================
// PROTOCOL HANDLER
// ============================================================================

typedef struct GameContextTag {
    BoardState_t board;
    EngineContext_t engine;
    MachineState_e eState;
    uint8_t bMyColor;
    uint8_t bOppColor;
    int64_t llTotalTimeUsed;
} GameContext_t;

static GameContext_t ctx;

// Command handler function pointer
typedef void (*FnCmdHandler)(const char* pszLine);

static void Cmd_HandleStart(const char* pszLine);
static void Cmd_HandlePlace(const char* pszLine);
static void Cmd_HandleTurn(const char* pszLine);
static void Cmd_HandleEnd(const char* pszLine);

typedef struct CmdTableEntryTag {
    const char* pszPrefix;
    int iPrefixLen;
    FnCmdHandler pfnHandler;
} CmdTableEntry_t;

static const CmdTableEntry_t rgCmdTable[] = {
    {"START", 5, Cmd_HandleStart},
    {"PLACE", 5, Cmd_HandlePlace},
    {"TURN", 4, Cmd_HandleTurn},
    {"END", 3, Cmd_HandleEnd},
    {NULL, 0, NULL}
};

// Command dispatcher using table lookup
static void Dispatcher_ProcessCommand(const char* pszLine) {
    const CmdTableEntry_t* pEntry;

    for (pEntry = rgCmdTable; pEntry->pszPrefix != NULL; pEntry++) {
        if (strncmp(pszLine, pEntry->pszPrefix, pEntry->iPrefixLen) == 0) {
            pEntry->pfnHandler(pszLine);
            return;
        }
    }
}

// START command handler
static void Cmd_HandleStart(const char* pszLine) {
    int iField;
    sscanf(pszLine, "START %d", &iField);

    ctx.bMyColor = (iField == 1) ? eBlackStone : eWhiteStone;
    ctx.bOppColor = (iField == 1) ? eWhiteStone : eBlackStone;

    Board_Initialize(&ctx.board);
    Board_SetupOpening(&ctx.board);

    Evaluator_Initialize();
    Engine_Init(&ctx.engine, &ctx.board, ctx.bMyColor);

    ctx.eState = eStateReady;
    ctx.llTotalTimeUsed = 0;

    printf("OK\n");
    fflush(stdout);
}

// PLACE command handler
static void Cmd_HandlePlace(const char* pszLine) {
    int iRow, iCol;
    int16_t iPos;

    sscanf(pszLine, "PLACE %d %d", &iRow, &iCol);

    iPos = iMakePos(iRow, iCol);
    Board_MakeMove(&ctx.board, iPos, ctx.bOppColor);
}

// TURN command handler
static void Cmd_HandleTurn(const char* pszLine) {
    int16_t iBestPos;
    int iRow, iCol;

    (void)pszLine;
    ctx.eState = eStateComputing;

    iBestPos = iSearchBestMove(&ctx.engine, eTurnTimeLimit);

    Board_MakeMove(&ctx.board, iBestPos, ctx.bMyColor);

    iRow = iGetRow(iBestPos);
    iCol = iGetCol(iBestPos);

    printf("%d %d\n", iRow, iCol);
    fflush(stdout);

    ctx.eState = eStateReady;
}

// END command handler
static void Cmd_HandleEnd(const char* pszLine) {
    (void)pszLine;
    ctx.eState = eStateFinished;
    Engine_Cleanup(&ctx.engine);
}

// ============================================================================
// MAIN EVENT LOOP
// ============================================================================

int main(void) {
    char szBuffer[256];
    char* pszNewline;

    ctx.eState = eStateInitial;

    // Main event loop
    while (ctx.eState != eStateFinished) {
        if (!fgets(szBuffer, sizeof(szBuffer), stdin)) break;

        pszNewline = strchr(szBuffer, '\n');
        if (pszNewline) *pszNewline = '\0';

        Dispatcher_ProcessCommand(szBuffer);
    }

    return 0;
}
