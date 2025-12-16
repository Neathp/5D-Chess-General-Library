#include <iostream>
#include <string>
#include <chrono>
#include <memory>
#include "ai.hpp"
#include "positions.hpp"
#include "params.hpp"  // Include params
int checkmatesFound=0;

int test(int posNumber, int depth, bool isBlack){
    constexpr U8 Set  = Chess5D::NoPiece;
    constexpr U8 Size = 8;
    constexpr U16 L   = 32;
    constexpr U16 T   = 128;

    Chess5D::Chess<Set, Size, L, T> chess{};
    Positions::load(chess, posNumber);

    count = 0;
    hitCount = 0;
    collision = 0;
    tt.clear();
    killerTable.empty();
    // Use evalParams and moveParams from params.hpp directly or pass them as needed.
    // Assuming negaMax1 accesses these globals internally, no need to change calls.
    auto start = std::chrono::high_resolution_clock::now();
    std::unique_ptr<Result> res = isBlack
        ? negaMax1<Set, Size, L, T, false, false, NodeTravel>(
              chess, -CHECKMATE, CHECKMATE, depth, 0,
              chess.origIndex[0],
              Chess5D::Move(0,0,0,0,Chess5D::NullMove,0,0,0,0))
        : negaMax1<Set, Size, L, T, true, false, NodeTravel>(
              chess, -CHECKMATE, CHECKMATE, depth, 0,
              chess.origIndex[0],
              Chess5D::Move(0,0,0,0,Chess5D::NullMove,0,0,0,0));

    auto end = std::chrono::high_resolution_clock::now();
    double elapsedSec = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / 1e9;
    
    std::cout<< res->value<<std::endl;
    if(res->value > CHECKMATE - depth*10) {
        std::cout << "Checkmate found: "<< posNumber << std::endl;
        checkmatesFound++;
    }
    return count;
}

int testIDDFS(int posNumber, int depth, bool isBlack){
    constexpr U8 Set  = Chess5D::NoPiece;
    constexpr U8 Size = 8;
    constexpr U16 L   = 32;
    constexpr U16 T   = 128;

    Chess5D::Chess<Set, Size, L, T> chess{};
    Positions::load(chess, posNumber);

    count = 0;
    hitCount = 0;
    collision = 0;
    tt.clear();
    killerTable.empty();
    // Use evalParams and moveParams from params.hpp directly or pass them as needed.
    // Assuming negaMax1 accesses these globals internally, no need to change calls.
    auto start = std::chrono::high_resolution_clock::now();
    int timeGiven = 10000;
    std::unique_ptr<Chess5D::Result> res = isBlack ? negaMaxIDDFS<Set, Size, L, T, false>(chess, depth, std::chrono::milliseconds(timeGiven)) : negaMaxIDDFS<Set, Size, L, T, true>(chess, depth, std::chrono::milliseconds(timeGiven));

    auto end = std::chrono::high_resolution_clock::now();
    double elapsedSec = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / 1e9;
    
    std::cout<< res->value<<std::endl;
    if(res->value > CHECKMATE - depth*10) {
        std::cout << "Checkmate found: "<< posNumber << std::endl;
        checkmatesFound++;
    }
    return count;
}

int testQ(int posNumber, int depth, bool isBlack){
    constexpr U8 Set  = Chess5D::NoPiece;
    constexpr U8 Size = 8;
    constexpr U16 L   = 32;
    constexpr U16 T   = 128;

    Chess5D::Chess<Set, Size, L, T> chess{};
    Positions::load(chess, posNumber);

    count = 0;
    hitCount = 0;
    collision = 0;
    tt.clear();
    killerTable.empty();
    // Use evalParams and moveParams from params.hpp directly or pass them as needed.
    // Assuming negaMax1 accesses these globals internally, no need to change calls.
    auto start = std::chrono::high_resolution_clock::now();
    std::unique_ptr<Result> res = isBlack
        ? qSearch<Set, Size, L, T, false>(chess, -CHECKMATE, CHECKMATE, 0, chess.origIndex[0], Chess5D::Move(0,0,0,0,Chess5D::NullMove,0,0,0,0))
        : qSearch<Set, Size, L, T, true>(chess, -CHECKMATE, CHECKMATE, 0, chess.origIndex[0], Chess5D::Move(0,0,0,0,Chess5D::NullMove,0,0,0,0));

    auto end = std::chrono::high_resolution_clock::now();
    double elapsedSec = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / 1e9;
    
    std::cout<< res->value<<std::endl;
    if(res->value > CHECKMATE - depth*10) {
        std::cout << "Checkmate found: "<< posNumber << std::endl;
        checkmatesFound++;
    }
    return count;
}


int main(int argc, char* argv[])
{
    int argi = 1;
    while (argi < argc) {
        std::string tag = argv[argi++];
        if (tag == "eval") {
            if (argi + 7 > argc) {
                std::cerr << "Not enough eval params\n";
                return 1;
            }
            evalParams.softmate   = std::stoi(argv[argi++]);
            evalParams.pieceVal   = std::stod(argv[argi++]);
            evalParams.moveVal    = std::stod(argv[argi++]);
            evalParams.kingExp    = std::stod(argv[argi++]);
            evalParams.kingExpTol = std::stoi(argv[argi++]);
            evalParams.tlValue    = std::stoi(argv[argi++]);
            evalParams.unmoved    = std::stoi(argv[argi++]);
        }
        else if (tag == "move") {
            if (argi + 11 > argc) {
                std::cerr << "Not enough move params\n";
                return 1;
            }
            moveParams.inactiveTravelPenalty       = std::stoi(argv[argi++]);
            moveParams.travelBasePenalty           = std::stoi(argv[argi++]);
            moveParams.travelPenalty               = std::stoi(argv[argi++]);
            moveParams.killerBonus                 = std::stoi(argv[argi++]);
            moveParams.captureMVVMultiplier        = std::stoi(argv[argi++]);
            moveParams.captureLVAMalus             = std::stoi(argv[argi++]);
            moveParams.captureBaseBonus            = std::stoi(argv[argi++]);
            moveParams.quietMovePenaltyMultiplier  = std::stoi(argv[argi++]);
            moveParams.checkBonus                  = std::stoi(argv[argi++]);
            moveParams.pastCheckBonus              = std::stoi(argv[argi++]);
            moveParams.travelCheckBonus            = std::stoi(argv[argi++]);
        }
        else if (tag == "ext") {
            if (argi + 12 > argc) {
                std::cerr << "Not enough ext params\n";
                return 1;
            }
            extParams.PVExt                = std::stoi(argv[argi++]);
            extParams.NMHReduct            = std::stoi(argv[argi++]);
            extParams.travelReduct         = std::stoi(argv[argi++]);
            extParams.inactiveTravelReduct = std::stoi(argv[argi++]);
            extParams.onlyReplyThreshold   = std::stoi(argv[argi++]);
            extParams.onlyReply            = std::stoi(argv[argi++]);
            extParams.checkExt             = std::stoi(argv[argi++]);
            extParams.pastCheckExt         = std::stoi(argv[argi++]);
            extParams.travelCheckExt       = std::stoi(argv[argi++]);
            extParams.LMR                  = std::stoi(argv[argi++]);
            extParams.LMRStart             = std::stoi(argv[argi++]);
            extParams.LMRDepth             = std::stoi(argv[argi++]);
        }
        else {
            std::cerr << "Unknown tag: " << tag << "\n";
            return 1;
        }
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    U64 totalCount = 0;

    std::vector<int> positions = {3, 7, 9, 10, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24};

    for (int pos : positions) {
        totalCount += testIDDFS(pos, 7, false);

        std::cout << "Intermediate nodes: " << totalCount << std::endl;

        auto end = std::chrono::high_resolution_clock::now();
        double elapsedSec = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / 1e9;
        if (elapsedSec > 240){
            break;
        }
    }
    double elapsedSec = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - start).count() / 1e9;
    
    std::cout << "solved=" << checkmatesFound
              << " time=" << elapsedSec
              << " nodes=" << totalCount
              << std::endl;

    //Best Yet: nodes=1,161,423,211

    return 0;
}
