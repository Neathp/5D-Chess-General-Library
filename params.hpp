#pragma once
#include <array>

constexpr int NumPieceTypes = 6;
using U8 = unsigned char;

template<U8 Set, U8 Size>
struct EvalParams {
    int softmate;
    int pieceVal;
    int moveVal;
    int kingExp;
    int kingExpTol;
    int kingDef;
    int tlValue;
    int unmoved;
    std::array<int, Set> typeToVal;
    int pieceSq[NumPieceTypes][Size];
};

template<U8 Set>
struct MoveScoreParams {
    int inactiveTravelPenalty;
    int travelBasePenalty;
    int travelPenalty;
    int killerBonus;
    int captureMVVMultiplier;
    int captureLVAMalus;
    int captureBaseBonus;
    int quietMovePenaltyMultiplier;
    int checkBonus;
    int pastCheckBonus;
    int travelCheckBonus;
    std::array<double, Set> typeToVal;
};


struct ExtensionParams {
    int PVExt;
    int NMHReduct;
    int travelReduct;
    int inactiveTravelReduct;
    int onlyReplyThreshold;
    int onlyReply;
    int checkExt;
    int pastCheckExt;
    int travelCheckExt;
    int LMR;
    int LMRStart;
    int LMRDepth;
};


inline EvalParams<12, 64> evalParams{
    -1000000, // softmate
    100,   // pieceVal
    500,    // moveVal
    200,    // kingExp
    4,      // kingExpTol
    100,   //kingDef
    -200000,  // tlValue
    500,      // unmoved
    {100, 500, 500, 300, 1500, 0, 900, 400, 0, 300, 200, 25}, // typeToVal
    {
        {0, 0, 0, 0, 0, 0, 0, 0, 464, 447, 181, 322, 75, 488, 475, 484, 301, 308, 1058, -36, 1586, 230, 138, 30, 91, 149, 79, 661, 284, 139, 34, 137, -18, 96, 188, 306, 270, 10, 76, 22, 21, -53, 89, 77, 227, 23, 35, -1, 17, 37, 41, 44, -7, 34, 9, 14, 49, 2, 3, 14, 7, 0, 0, 14}, //Pawn
        {-11, 263, -17, 9, 32, 18, 53, 0, -24, 6, 24, 247, -141, 24, -24, 15, 116, -82, 297, 32, -123, 1125, 87, 212, -18, -26, 282, 354, 173, -134, 35, 106, 2, 441, 30, 120, 1018, 331, 780, -6, -3, 31, 142, 459, 881, 184, 104, 24, 34, 37, 68, 141, 143, 406, 86, 1, 32, 9, 97, 137, 55, -8, 47, -34}, //Knight
        {-15, 13, 418, -12, 11, 394, 0, -10, -8, 240, 35, 134, -63, -20, 214, -31, 44, 67, 154, 551, 80, -74, 284, 0, -59, -7, 911, -41, 36, 504, -124, 59, -9, 518, 148, 99, 274, 184, 653, -35, 64, -32, 181, 204, 348, 540, 125, 66, -7, 102, 72, 144, 319, 508, 159, 74, 57, -20, 28, 176, 64, 56, 10, 114}, //Bishop
        {1481, 224, 458, 892, 376, 592, -235, 1592, 86, 20, -26, 235, 128, 64, 92, -14, -161, 51, -100, 51, -39, -17, -57, 34, 26, -39, -51, -8, 44, 85, 68, 112, 25, 44, 37, 92, -41, 49, 21, 115, 242, -81, 22, 68, 6, -2, 52, 58, 156, 57, 78, 179, 50, 118, -10, 179, -2, 209, -47, 147, 110, 15, 92, 136}, //Rook
        {-12, -49, 26, -16, 80, 91, 24, 19, 23, -35, 170, -20, 449, -39, -44, 26, 134, 814, 88, 108, -96, 1029, 25, -14, 76, 100, 273, -95, 94, -20, 129, 41, 117, 309, 53, 263, 56, 141, 131, 1258, 33, 82, 178, 80, 115, 66, 250, 8, 15, 158, 25, 274, 138, 1470, -15, 27, 45, 21, 201, 77, 19, -40, 28, 47}, //Queen
        {1, -8, 40, -18, 161, -14, 134, 12, 1, -15, -17, 7, 3, -163, -14, 21, -300, -2, -7, 8, -6, -8, 13, -6, -300, -2, 2, 1, 8, -4, -2, -2, -300, 3, -300, -300, 1, -300, -2, -300, -300, -300, -300, -300, -300, -300, -300, -300, -300, -300, -300, -300, -300, -300, -300, -300, -300, -300, -300, -300, -300, -300, -300, -300} //King
    }
};

// GOOD Opt
/*
inline MoveScoreParams<12> moveParams = {
    629,  // inactiveTravelPenalty
    419,  // travelBasePenalty
    385,  // travelPenalty
    1727, // killerBonus
    0,    // captureMVVMultiplier
    0,    // captureLVAMalus
    29,   // captureBaseBonus
    5,    // quietMovePenaltyMultiplier
    188,  // checkBonus
    503,  // travelCheckBonus
    {1, 5, 5, 3, 15, 0, 9, 4, 0, 3, 2, 0.25} // unchanged constants
};

inline ExtensionParams extParams = {
    1,  // travelReduct
    1,  // inactiveTravelReduct
    4,  // onlyReplyThreshold
    1,  // onlyReply
    0,  // checkExtend
    1,  // travelCheckExtend
    2,  // LMR
    29, // LMRStart
    2   // LMRDepth
};
*/

/*
//Best Opt Before Past
inline MoveScoreParams<12> moveParams = {
    629,   // inactiveTravelPenalty
    419,   // travelBasePenalty
    385,   // travelPenalty
    2145,  // killerBonus
    -9,    // captureMVVMultiplier
    -3,    // captureLVAMalus
    314,   // captureBaseBonus
    79,    // quietMovePenaltyMultiplier
    633,   // checkBonus
    0,   // pastCheckBonus
    503,   // travelCheckBonus
    {1, 5, 5, 3, 15, 0, 9, 4, 0, 3, 2, 0.25}
};

inline ExtensionParams extParams = {
    1,  // PvExt
    1,  // travelReduct
    1,  // inactiveTravelReduct
    3,  // onlyReplyThreshold
    1,  // onlyReply
    1,  // checkExt
    0,  // pastCheckExt
    1,  // travelCheckExt
    1,  // LMR
    23, // LMRStart
    3   // LMRDepth
};
*/


//Best Opt, might be overfit, oddly not as good as before with past check ext
inline MoveScoreParams<12> moveParams = {
    629,   // inactiveTravelPenalty
    419,   // travelBasePenalty
    385,   // travelPenalty
    3326,  // killerBonus
    -15,    // captureMVVMultiplier
    -11,    // captureLVAMalus
    251,   // captureBaseBonus
    152,    // quietMovePenaltyMultiplier
    283,   // checkBonus
    332,   // pastCheckBonus
    503,   // travelCheckBonus
    {1, 5, 5, 3, 15, 0, 9, 4, 0, 3, 2, 0.25}
};

inline ExtensionParams extParams = { // These should be doubles and depth should be handled fractionally.
    1,  // PvExt
    2,  // NMHReduct
    1,  // travelReduct
    1,  // inactiveTravelReduct
    3,  // onlyReplyThreshold
    1,  // onlyReply
    1,  // checkExt
    0,  // pastCheckExt
    1,  // travelCheckExt
    1,  // LMR
    24, // LMRStart
    3   // LMRDepth
};

