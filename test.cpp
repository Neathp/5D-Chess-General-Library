#include "ai.hpp"
#include "gtest/gtest.h"


// TEST(negaMax, SapphiaMateIn2) { //not actually mate in 2
//     constexpr U8 Set = Chess5D::BPrincess;
//     constexpr U8 Size = 8;
//     constexpr U16 L = 32;
//     constexpr U16 T = 128;

//     constexpr bool White = true;

//     Chess5D::Chess<Set, Size, L, T> chess{};
//     std::string fen = "[r*n2k*b1r*/p*b1p*qp*p*p*/4p*n2/1p*2N3/8/4P*N2/P*P*1P*1P*P*P*/3QK*B2:0:1:w]\n";
//     chess.importFen(fen);

//     std::string pgn=
//     "1. (0T1)Bf1b5 / (0T1)Bb7f3\n"
//     "2. (0T2)Qd1f3 / (0T2)Be7c5\n"; //Setup
//     "3. (0T3)Bb5d7 / (0T3)Ke8d8\n"
//     "4. (0T4)Qf3f6\n";
    
//     chess.importPGN(pgn);

//     Chess5D::Result res = negaMax1<Set, Size, L, T, White>(chess,-1000000000,1000000000, 3, chess.origIndex[1], Chess5D::Move(0, 0, 0, 0, Chess5D::NullMove, 0, 0, 0, 0));
//     EXPECT_EQ(res.value, CHECKMATE);
// };


TEST(negaMax, MageLosMateIn5) {
    constexpr U8 Set = Chess5D::BPrincess;
    constexpr U8 Size = 8;
    constexpr U16 L = 32;
    constexpr U16 T = 128;

    constexpr bool White = true;

    Chess5D::Chess<Set, Size, L, T> chess{};
    std::string fen = "[r*nb2k1r*/1q2p*p*bp*/p4n2/1p6/1P1P1B2/PB2P1N1/3N1P*P*P*/R*2QK*2R*:0:1:w]\n"; //M5
    chess.importFen(fen);
 
    std::string pgn="1. (0T1)Bb3a2 / (0T1)Bc8d7\n"
    "2. (0T2)Ba2b3 / (0T2)Bd7c8\n"; //prep
    /*
    "3. (0T3)Bb3f7 / (0T3)Kf8f7\n";
    "4. (0T4)Ng3f5 / (0T4)Bc8f5\n";
    "5. (0T5)Bf4b8 / (0T5)Qb7b8\n";
    */

    chess.importPGN(pgn);

    Chess5D::Result res = negaMax1<Set, Size, L, T, White>(chess,-1000000000,1000000000, 9, chess.origIndex[1], Chess5D::Move(0, 0, 0, 0, Chess5D::NullMove, 0, 0, 0, 0));
    EXPECT_EQ(res.value, CHECKMATE);
};

TEST(negaMax, Perft) {
    constexpr U8 Set = Chess5D::BPrincess;
    constexpr U8 Size = 8;
    constexpr U16 L = 32;
    constexpr U16 T = 128;

    constexpr bool White = true;

    Chess5D::Chess<Set, Size, L, T> chess{};
    std::string fen = "[r*nbqk*bnr*/p*p*p*p*p*p*p*p*/8/8/8/8/P*P*P*P*P*P*P*P*/R*NBQK*BNR*:0:1:w]\n";
    chess.importFen(fen);

    count=0;
    mates=0;

    Chess5D::Result res = negaMax_Test<Set, Size, L, T, White>(chess, 1, chess.origIndex[1]);
    EXPECT_EQ(res.value, 0);
    EXPECT_EQ(count, 20);
    EXPECT_EQ(mates, 0);

    count=0;
    mates=0;

    res = negaMax_Test<Set, Size, L, T, White>(chess, 2, chess.origIndex[1]);
    EXPECT_EQ(res.value, 0);
    EXPECT_EQ(count, 400);
    EXPECT_EQ(mates, 0);

    count=0;
    mates=0;

    res = negaMax_Test<Set, Size, L, T, White>(chess, 3, chess.origIndex[1]);
    EXPECT_EQ(res.value, 0);
    EXPECT_EQ(count, 9822); //9822 actually but for now
    EXPECT_EQ(mates, 0);

    count=0;
    mates=0;

    res = negaMax_Test<Set, Size, L, T, White>(chess, 4, chess.origIndex[1]);
    EXPECT_EQ(res.value, 0);
    EXPECT_GT(count, 197281); //not currently known
    EXPECT_EQ(mates, 8);

    count=0;
    mates=0;

    res = negaMax_Test<Set, Size, L, T, White>(chess, 5, chess.origIndex[1]);
    EXPECT_EQ(res.value, 0);
    EXPECT_GT(count, 4865609	); //not currently known
    EXPECT_GT(mates,  3432);
};


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}