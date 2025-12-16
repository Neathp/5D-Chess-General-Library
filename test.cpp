#include "ai.hpp"
#include "gtest/gtest.h"
#include "positions.hpp"

TEST(negaMax, SapphiaMateIn2) { //not actually mate in 2 with travels
    constexpr U8 Set = Chess5D::BPrincess;
    constexpr U8 Size = 8;
    constexpr U16 L = 32;
    constexpr U16 T = 128;

    constexpr bool White = true;

    Chess5D::Chess<Set, Size, L, T> chess{};
    std::string fen = "[r*n2k*b1r*/p*b1p*qp*p*p*/4p*n2/1p*2N3/8/4P*N2/P*P*1P*1P*P*P*/3QK*B2:0:1:w]\n";
    chess.importFen(fen);

    std::string pgn =
        "1. (0T1)Bf1b5 / (0T1)Bb7f3\n"
        "2. (0T2)Qd1f3 / (0T2)Be7c5\n"; //Setup
    //    "3. (0T3)Bb5d7 / (0T3)Ke8d8\n"
    //    "4. (0T4)Qf3f6\n"

    chess.importPGN(pgn);
    U8 timeline=chess.origIndex[0];
    std::unique_ptr<Result> resPtr = negaMax1<Set, Size, L, T, White, false, NodeSpatial>(chess, -CHECKMATE, CHECKMATE, 3, 0, timeline, Chess5D::Move(0, 0, 0, 0, Chess5D::NullMove, 0, 0, 0, 0));
    if (resPtr->value != CHECKMATE-3) {
        backtrace<Set, Size, L, T, White>(chess, timeline, 0);
    }
    EXPECT_EQ(resPtr->value, CHECKMATE-3);
}


TEST(negaMax, MageLosMateIn5) {
    constexpr U8 Set = Chess5D::NoPiece;
    constexpr U8 Size = 8;
    constexpr U16 L = 32;
    constexpr U16 T = 128;

    constexpr bool White = true;

    Chess5D::Chess<Set, Size, L, T> chess{};
    std::string fen = "[r*nb2k1r*/1q2p*p*bp*/p4n2/1p6/1P1P1B2/PB2P1N1/3N1P*P*P*/R*2QK*2R*:0:1:w]\n"; //M5
    chess.importFen(fen);

    std::string pgn =
        "1. (0T1)Bb3a2 / (0T1)Bc8d7\n"
        "2. (0T2)Ba2b3 / (0T2)Bd7c8\n"; //prep
    /*
    "3. (0T3)Bb3f7 / (0T3)Kf8f7\n";
    "4. (0T4)Ng3f5 / (0T4)Bc8f5\n";
    "5. (0T5)Bf4b8 / (0T5)Qb7b8\n";
    */

    chess.importPGN(pgn);
    U8 timeline=chess.origIndex[0];
    std::unique_ptr<Result> resPtr = negaMax1<Set, Size, L, T, White, false, NodeTravel>(chess, -CHECKMATE, CHECKMATE, 9, 0, timeline, Chess5D::Move(0, 0, 0, 0, Chess5D::NullMove, 0, 0, 0, 0));
    if (resPtr->value != CHECKMATE-9) {
        backtrace<Set, Size, L, T, White>(chess, timeline, 0);
    }
    EXPECT_EQ(resPtr->value, CHECKMATE-9);
}

template<U8 Set, U8 Size, U16 L, U16 T, bool White>
void runPerftTest(Chess5D::Chess<Set, Size, L, T>& chess, int depth, int expectedValue, int expectedCount, int expectedMates, bool gtCount = false, bool gtMates = false) {
    count = 0;
    mates = 0;
    std::unique_ptr<Result> resPtr = negaMax_Test<Set, Size, L, T, White>(chess, depth, chess.origIndex[0]);
    std::cout<< count << " nodes, " << mates << " mates found at depth " << depth << std::endl;
    EXPECT_EQ(resPtr->value, expectedValue);
    if (gtCount) {
        EXPECT_GT(count, expectedCount);
    } else {
        EXPECT_EQ(count, expectedCount);
    }
    if (gtMates) {
        EXPECT_GT(mates, expectedMates);
    } else {
        EXPECT_EQ(mates, expectedMates);
    }
}


TEST(negaMax, Perft) {
    constexpr U8 Set = Chess5D::BPrincess;
    constexpr U8 Size = 8;
    constexpr U16 L = 32;
    constexpr U16 T = 128;
    constexpr bool White = true;

    Chess5D::Chess<Set, Size, L, T> chess{};
    std::string fen = "[r*nbqk*bnr*/p*p*p*p*p*p*p*p*/8/8/8/8/P*P*P*P*P*P*P*P*/R*NBQK*BNR*:0:1:w]\n";
    chess.importFen(fen);

    runPerftTest<Set, Size, L, T, White>(chess, 1, 0, 20, 0);
    runPerftTest<Set, Size, L, T, White>(chess, 2, 0, 400, 0);
    runPerftTest<Set, Size, L, T, White>(chess, 3, 0, 9822, 0);
    runPerftTest<Set, Size, L, T, White>(chess, 4, 0, 240317, 8); //197281 min
    runPerftTest<Set, Size, L, T, White>(chess, 5, 0, 6903429, 3946); //4865609 min, 3432 min
}

TEST(negaMax, PerftCastle) {
    constexpr U8 Set = Chess5D::BPrincess;
    constexpr U8 Size = 8;
    constexpr U16 L = 32;
    constexpr U16 T = 128;
    constexpr bool White = true;

    Chess5D::Chess<Set, Size, L, T> chess{};
    Positions::load(chess, 27); //Castle Enabled Position

    //Run for large length to test invariants for later positions
    std::unique_ptr<Result> resPtr = negaMax_Test<Set, Size, L, T, White>(chess, 4, chess.origIndex[0]);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}