#include <algorithm>
#include "chess.hpp"
#include <string>

namespace Positions
{
    template <typename Chess>
    void load(Chess &chess, int posNumber)
    {
        if(posNumber == 0)
        {
            std::string fen = "[r*nbqk*bnr*/p*p*p*p*p*p*p*p*/8/8/8/8/P*P*P*P*P*P*P*P*/R*NBQK*BNR*:0:1:w]\n";
            chess.importFen(fen);
        }
        if(posNumber == 1)
        {
            std::string fen = "[r*nbqk*bnr*/p*p*p*p*p*p*p*p*/8/8/8/8/P*P*P*P*P*P*P*P*/R*NBQK*BNR*:0:1:w]\n";
            chess.importFen(fen);
            std::string pgn = "1. (0T1)Ng1f3 / (0T1)Ng8f6\n"
                              "2. (0T2)d2d4 / (0T2)d7d5\n"
                              "3. (0T3)c2c3 / (0T3)c7c6\n";
            // prep
            //"3. (0T3)Bb5d7 / (0T3)Ke8d8\n"
            //"4. (0T4)Qf3f6\n";
            chess.importPGN(pgn);
        }
        else if (posNumber == 2)
        {
            std::string fen = "[r*n2k*b1r*/w*b1w*qw*w*w*/4w*n2/1w*2N3/8/4P*N2/W*W*1W*1W*W*W*/3QK*B2:0:1:w]\n"; //mate in 2
            //std::string fen = "[1n2k*b2/w*b1w*qw*2/4w*n2/1w*2N3/8/4P*N2/3W*1W*2/3QK*B2:0:1:w]\n"; //simplified position
            chess.importFen(fen);

            std::string pgn = "1. (0T1)Bf1b5 / (0T1)Bb7f3\n"
                              "2. (0T2)Qd1f3 / (0T2)Be7c5\n";
            // prep
            //"3. (0T3)Bb5d7 / (0T3)Ke8d8\n";
            //"4. (0T4)Qf3f6\n";
            chess.importPGN(pgn);
        }
        else if (posNumber == 3)
        {
            std::string fen = "[r*nb2k1r*/1q2p*p*bp*/p4n2/1p6/1P1P1B2/PB2P1N1/3N1P*P*P*/R*2QK*2R*:0:1:w]\n"; // M5
            chess.importFen(fen);

            chess.importFen(fen);
            std::string pgn = "1. (0T1)Bb3a2 / (0T1)Bc8d7\n"
                              "2. (0T2)Ba2b3 / (0T2)Bd7c8\n"
            // prep
            "3. (0T3)Bb3f7 / (0T3)Kf8f7\n"
            "4. (0T4)Ng3f5 / (0T4)Bc8f5\n";
            //"5. (0T5)Bf4b8 / (0T5)Qb7b8\n"
            //"6. (0T6)Qd1b3 / (0T6)Bf5e6\n";

            chess.importPGN(pgn);
        }
        else if (posNumber == 4)
        {
            std::string fen = "[r*nbqk*bnr*/p*p*p*p*p*p*p*p*/8/8/8/8/P*P*P*P*P*P*P*P*/R*NBQK*BNR*:0:1:w]\n";
            chess.importFen(fen);

            std::string pgn = "1.{10:03}(0T1)e2e4 / {10:03}(0T1)Ng8f6\n"
                              "2.{10:02}(0T2)d2d4 / {10:01}(0T2)e7e6\n"
                              "3.{10:01}(0T3)Bc1g5 / {10:01}(0T3)c7c5\n"
                              "4.{10:01}(0T4)b2b4 / {10:01}(0T4)c5b4\n"
                              "5.{10:01}(0T5)Nb1d2 / {9:52}(0T5)h7h5\n"
                              "6.{9:05}(0T6)Bf1b5 / {8:14}(0T6)a7a6\n"
                              "7.{9:00}(0T7)Bb5a4 / {7:45}(0T7)b7b5\n"
                              "8.{8:59}(0T8)Ba4b3 / {7:34}(0T8)Nb8c6\n"
                              "9.{8:27}(0T9)d4d5 / {6:43}(0T9)Nc6d4\n "
                              "10.{8:07}(0T10)Ng1e2 / {5:49}(0T10)Bf8c5\n"
                              "11.{7:38}(0T11)Ne2d4 / {5:49}(0T11)Bc5d4\n"
                              "12.{7:34}(0T12)Nd2f3 / {4:25}(0T12)Bd4f2\n"
                              "13.{7:33}(0T13)Ke1f2 / {4:06}(0T13)Nf6e4\n"
                              "14.{7:33}(0T14)Kf2e1 / {3:44}(0T14)Ne4g5\n"
                              "15.{7:29}(0T15)Qd1d4 / {3:44}(0T15)Ng5f3\n"
                              "16.{7:29}(0T16)g2f3 / {3:11}(0T16)Qd8e7\n"
                              "17.{6:22}(0T17)d5d6 / {1:55}(0T17)Qe7g5\n"
                              "18.{6:15}(0T18)Rh1g1 / {1:19}(0T18)Qg5f6\n"
                              "19.{6:02}(0T19)Qd4f6 / {1:19}(0T19)g7f6\n"
                              "20.{4:37}(0T20)Ra1c1 / {0:55}(0T20)a6a5\n"
                              "21.{4:39}(0T21)c2c4 / {0:40}(0T21)Bc8>>(0T16)c3\n"
                              "22.{2:49}(-1T17)Ke1f1 / {0:40}(-1T17)Bc3d4\n"
                              "23.{2:35}(-1T18)h2h4 / {0:34}(-1T18)Qd8f6\n"
                              "24.{2:26}(-1T19)Rh1h3 / {0:25}(-1T19)Bd4f2\n"
                              "25.{2:25}(-1T20)Kf1f2 / {0:24}(-1T20)Qf6d4\n"
                              "26.{1:48}(-1T21)Kf2g2 / {0:23}(-1T21)Rh8h6\n";
            chess.importPGN(pgn);
        }
        else if (posNumber == 5)
        {
            std::string fen = "[r*nbqk*bnr*/p*p*p*p*p*p*p*p*/8/8/8/8/P*P*P*P*P*P*P*P*/R*NBQK*BNR*:0:1:w]\n"; // Loading test
            chess.importFen(fen);

            std::string pgn = "1.{0:09}(0T1)Ng1f3 / {0:56}(0T1)Ng8f6\n"
                               "2.{0:09}(0T2)d2d4 / {0:56}(0T2)e7e6\n"
                               "3.{0:09}(0T3)c2c3 / {0:56}(0T3)Qd8e7\n"
                               "4.{0:09}(0T4)Bc1g5 / {0:56}(0T4)c7c5\n"
                               "5.{0:09}(0T5)d4d5 / {0:56}(0T5)Qe7d6\n"
                               "6.{0:09}(0T6)Nb1a3 / {0:56}(0T6)Qd6d5\n"
                               "7.{0:09}(0T7)Qd1d5 / {0:56}(0T7)Nf6d5\n"
                               "8.{0:09}(0T8)Na3b5 / {0:56}(0T8)f7f6\n"
                               "9.{0:09}(0T9)Bg5h4 / {0:56}(0T9)a7a6\n"
                               "10.{0:09}(0T10)Nb5a3 / {0:56}(0T10)Nb8c6\n"
                               "11.{0:09}(0T11)Bh4g3 / {0:56}(0T11)b7b5\n"
                               "12.{0:09}(0T12)Ra1d1 / {0:56}(0T12)Nc6b4\n"
                               "13.{0:09}(0T13)c3b4 / {0:56}(0T13)c5b4\n"
                               "14.{0:09}(0T14)Na3c2 / {0:56}(0T14)Nd5b6\n"
                               "15.{0:09}(0T15)b2b3 / {0:56}(0T15)a6a5\n"
                               "16.{0:09}(0T16)e2e3 / {0:56}(0T16)a5a4\n"
                               "17.{0:09}(0T17)Bf1b5 / {0:56}(0T17)a4b3\n"
                               "18.{0:09}(0T18)Nc2d4 / {0:56}(0T18)Ke8f7\n"
                               "19.{0:09}(0T19)Nd4f5 / {0:56}(0T19)e6f5\n"
                               "20.{0:09}(0T20)Nf3>>(0T19)f5 / {0:56}(0T20)Bc8>>(0T15)c3\n"
                               "21.{0:09}(-1T16)Nf3d2 / {0:56}(-1T16)Nb6d5\n"
                               "22.{0:09}(-1T17)e2e3 / {0:56}(-1T17)Bc3d2\n"
                               "23.{0:09}(-1T18)Ke1d2 / {0:56}(-1T18)Bf8c5\n"
                               "24.{0:09}(-1T19)Bf1c4 / {0:56}(-1T19)Nd5e3 (1T19)e6f5\n"
                               "25.{0:09}(-1T20)f2e3 (1T20)Nd4f5 / {0:56}(-1T20)Bc5>>(-1T19)c4\n"
                               "26.{0:09}(-2T20)b3c4 / {0:56}(-2T20)Bc5>(1T20)f5\n"
                               "27.{0:09}(1T21)Nf3>(0T21)f5 (-1T21)Bc4e6 (-2T21)Bg3c7";

            chess.importPGN(pgn);
        }
        else if(posNumber == 6){
            std::string fen = "[r*nb2k1r*/1q2p*p*bp*/p4n2/1p6/1P1P1B2/PB2P1N1/3N1P*P*P*/R*2QK*2R*:0:1:w]\n[r*nb2k1r*/1q2p*p*bp*/p4n2/1p6/1P1P1B2/PB2P1N1/3N1P*P*P*/R*2QK*2R*:1:1:w]\n[r*nb2k1r*/1q2p*p*bp*/p4n2/1p6/1P1P1B2/PB2P1N1/3N1P*P*P*/R*2QK*2R*:-1:1:w]"; // Multitimeline
            chess.importFen(fen);
            std::string pgn = "1. (-1T1)Bb3a2 (0T1)Bb3a2 (1T1)Bb3a2 / (-1T1)Bc8d7 (0T1)Bc8d7 (1T1)Bc8d7\n"
                "2. (-1T2)Ba2b3 (0T2)Ba2b3 (1T2)Ba2b3 / (-1T2)Bd7c8 (0T2)Bd7c8 (1T2)Bd7c8\n" // prep
                "3. (-1T3)Bb3f7 (0T3)Bb3f7 (1T3)Bb3f7 / (-1T3)Kf8f7 (0T3)Kf8f7 (1T3)Kf8f7\n"
                "4. (-1T4)Ng3f5 (0T4)Ng3f5 (1T4)Ng3f5 / (-1T4)Bc8f5 (0T4)Bc8f5 (1T4)Bc8f5\n"
                "5. (-1T5)Bf4b8 (0T5)Bf4b8 (1T5)Bf4b8 / (-1T5)Qb7b8 (0T5)Qb7b8 (1T5)Qb7b8\n";
            chess.importPGN(pgn);
        }
        else if(posNumber == 7){
            std::string fen = "[3qk*b1r*/2p*p*1p*1p*/bpn1p1pn/pN6/P3N2P/1PB1PQ2/2P*2P*P*R/R*3K*B2:0:1:w]"; // Tesseract mate in 6
            chess.importFen(fen);
        }
        else if(posNumber == 8){
            std::string fen = "[r*1bqk*b1r*/p*p*p*p*p*p*p*p*/8/8/8/8/P*P*P*P*P*P*P*P*/R*NBQK*BNR*:0:1:w]\n";
            chess.importFen(fen);
            std::string pgn = "1.(0T1)e2e3 / (0T1)f7f6\n";
                               //"2.(0T2)Qd1h5 / (0T2)g7g6\n";
                               //"3.(0T3)Qh5d5 / (0T3)e7e6\n"
                               //"4.(0T4)Qh5f7\n";
            chess.importPGN(pgn);
        }
        else if(posNumber == 9){
            std::string fen = "[r*nbqk*1r*1/p*p*p*p*1p*p*1/4pnB1/7p/3P4/3Q4/P*P*P*1P*P*P*P*/R*NB1K*B1R*:0:1:w]\n";//Mate in 8
            chess.importFen(fen);
        }
        else if(posNumber == 10){
            std::string fen = "[r*nbq1bnr*/p*p*p*p*p*k*p*p*/8/8/8/8/P*P*P*P*P*P*P*P*/R*NBQK*BNR*:0:1:w]\n";//f7sac
            chess.importFen(fen);
        }
        else if(posNumber == 11){
            std::string fen = "[r*nbqk*bnr*/p*p*p*p*p*p*p*p*/8/8/8/8/P*P*P*P*P*P*P*P*/R*NBQK*BNR*:0:1:w]\n";
            chess.importFen(fen);
            std::string pgn = "1. (0T1)e2e4 / (0T1)Ng8f6\n"
                              "2. (0T2)Ng1f3";
            // prep
            //"3. (0T3)Bb5d7 / (0T3)Ke8d8\n"
            //"4. (0T4)Qf3f6\n";
            chess.importPGN(pgn);
        }

    }
}