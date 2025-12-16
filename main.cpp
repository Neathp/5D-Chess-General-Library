#include <algorithm>
#include "ai.hpp"
#include "positions.hpp"

template <U8 Set, U8 Size, U16 L, U16 T>
void playChess()
{
    Chess5D::Chess<Set, Size, L, T> chess{};

    // Prompt user for FEN input or predefined position
    std::cout << "Enter FEN string or a number to load a predefined position: ";
    std::string input;
    std::getline(std::cin, input);

    bool isNumber = !input.empty() && std::all_of(input.begin(), input.end(), ::isdigit);

    if (isNumber) {
        int posNumber = std::stoi(input);
        Positions::load(chess, posNumber);
    } else if (input.empty()) {
        Positions::load(chess, 0);
    } else {
        std::string fen = input;
        chess.importFen(fen);
    }
    std::cout << chess << std::endl;

    while (true)
    {
        std::cout << "Choose an option (move/engine/exit): ";
        std::string option;
        std::getline(std::cin, option);

        if (option == "move")
        {
            // Prompt user for multi-line PGN input
            std::cout << "Enter PGN string (end with an empty line):" << std::endl;
            std::string pgn, line;
            while (true)
            {
                std::getline(std::cin, line);
                if (line.empty())
                    break;
                pgn += line + "\n";
            }
            chess.importPGN(pgn); //TODO: should really be able to handle multiple lines. AKA dont need to say 1. Nb5d6 can just say Nb5d6
            std::cout << chess << std::endl;
        }
        else if (option == "engine")
        {
            Chess5D::killerTable.empty();
            // Prompt user for depth
            int depth;
            std::cout << "Enter depth: ";
            std::cin >> depth;
            std::cin.ignore(); // Ignore newline character left in the buffer

            std::cout << "Enter Color(w/b): ";
            std::string color;
            std::getline(std::cin, color);
            bool isBlack = (color == "b");

            int timeline = chess.origIndex[!isBlack];
            count = 0;
            hitCount = 0;
            collision = 0;
            auto begin = std::chrono::high_resolution_clock::now();
            int timeGiven = 20000;
            int maxSimuls = 1000;
            //std::unique_ptr<Chess5D::Result> res = isBlack ? negaMaxIDDFS<Set, Size, L, T, false>(chess, depth, std::chrono::milliseconds(timeGiven)) : negaMaxIDDFS<Set, Size, L, T, true>(chess, depth, std::chrono::milliseconds(timeGiven));
            std::unique_ptr<Chess5D::Result> res = isBlack ? MCTS<Set, Size, L, T, false>(chess, maxSimuls, depth) : MCTS<Set, Size, L, T, true>(chess, maxSimuls, depth);
            //std::unique_ptr<Result> res = isBlack ? negaMax1<Set, Size, L, T, false, true, NodeTravel>(chess, -CHECKMATE, CHECKMATE, depth, 0, timeline, Chess5D::Move(0, 0, 0, 0, Chess5D::NullMove, 0, 0, 0, 0)) : negaMax1<Set, Size, L, T, true, true, NodeTravel>(chess, -CHECKMATE, CHECKMATE, depth, 0, timeline, Chess5D::Move(0, 0, 0, 0, Chess5D::NullMove, 0, 0, 0, 0));;
            //std::unique_ptr<Result> res = isBlack ? qSearch<Set, Size, L, T, false>(chess, -CHECKMATE, CHECKMATE, 0, chess.origIndex[0], Chess5D::Move(0,0,0,0,Chess5D::NullMove,0,0,0,0)) : qSearch<Set, Size, L, T, true>(chess, -CHECKMATE, CHECKMATE, 0, chess.origIndex[0], Chess5D::Move(0,0,0,0,Chess5D::NullMove,0,0,0,0));
            
            auto end = std::chrono::high_resolution_clock::now();
            std::cout << std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() / 1000000000.0 << " s\n";
            std::cout << "Max Depth: " << maxPlyReached << std::endl;
            std::cout << "Positions: " << count << std::endl;
            std::cout << "QPositions: " << qCount << std::endl;
            std::cout << "NMH: " << NMHcut << std::endl;
            std::cout << "Hits: " << hitCount << std::endl;
            std::cout << "Collision: " << collision << std::endl;
            std::cout << chess << std::endl;
            
            Chess5D::TimelineInfo info = chess.timelineInfo[timeline];
            Chess5D::Board<Set> &brd1 = chess.boards[timeline][info.turn];
            Chess5D::Board<Set> &brd2 = chess.boards[timeline][info.turn-1];
            //printMasks<Set>(brd1);
            //printMasks<Set>(brd2);
            /*
            for (int i=info.tailIndex; i<=info.turn; ++i) {
                std::cout << "Turn " << i-info.tailIndex << std::endl;
                Chess5D::Board<Set> &brd = chess.boards[timeline][i];
                printMasks<Set>(brd);
            }
            */
            std::cout <<"Score: " <<res->value << std::endl;

            isBlack ? backtrace<Set, Size, L, T, false>(chess, 0, res) : backtrace<Set, Size, L, T, true>(chess, 0, res);

            std::cout << std::endl;
        }
        else if (option == "test")
        {
            int depth;
            std::cout << "Enter depth: ";
            std::cin >> depth;
            std::cin.ignore();

            std::cout << "Enter Color(w/b): ";
            std::string color;
            std::getline(std::cin, color);
            bool isBlack = (color == "b");

            while (true)
            {
                std::unique_ptr<Chess5D::Result> res = isBlack ? negaMaxIDDFS<Set, Size, L, T, false>(chess, depth, std::chrono::milliseconds(10000)) : negaMaxIDDFS<Set, Size, L, T, true>(chess, depth, std::chrono::milliseconds(10000));
                std::cout << "Here" << std::endl;
                isBlack ? chess.template makeMove<false>(res->moveset[0]) : chess.template makeMove<true>(res->moveset[0]); //Result needs to change
                std::cout << res->value << std::endl;
                std::cout << chess << std::endl;
                isBlack = !isBlack;

                std::cout << "Press Enter to continue or 'q' to quit...";
                std::string cont;
                std::getline(std::cin, cont);
                if (cont == "q")
                {
                    break;
                }
            }
        }
        else if (option == "eval")
        {
            std::cout << "Enter Color(w/b): ";
            std::string color;
            std::getline(std::cin, color);
            bool isBlack = (color == "b");

            int timeline = chess.origIndex[0];
            TimelineInfo &info = chess.timelineInfo[timeline];
            Board<Set> &brd = chess.boards[timeline][info.turn];
            std::vector<Move> moves;
            moves.reserve(100);
            std::cout<<"waybefore"<<std::endl;
            for (int i = 0; i < 64; i++) {
                std::cout << "pinMasks[" << i << "]" << info.pinMasks[i] << "\n";
                std::cout << "checkMasks[" << i << "]" << info.checkMasks[i] << "\n";
            }
            if (isBlack) {
                chess.template generateMoves<false>(moves, timeline);
            } else {
                chess.template generateMoves<true>(moves, timeline);
            }

            TimelineInfo infoBefore = info;
            Board<Set> brdBefore = brd;
            std::cout<<"before"<<std::endl;
            for (int i = 0; i < 64; i++) {
                std::cout << "pinMasks[" << i << "]" << info.pinMasks[i] << "\n";
                std::cout << "checkMasks[" << i << "]" << info.checkMasks[i] << "\n";
            }

            if (isBlack) {
                chess.template makeMove<false>(moves[0]);
                chess.template undoMove<false>(moves[0]);
            } else {
                chess.template makeMove<true>(moves[0]);
                chess.template undoMove<true>(moves[0]);
            }
            std::cout<<"afteer"<<std::endl;
            for (int i = 0; i < 64; i++) {
                std::cout << "pinMasks[" << i << "]" << info.pinMasks[i] << "\n";
                std::cout << "checkMasks[" << i << "]" << info.checkMasks[i] << "\n";
            }
            debugCompare(infoBefore, info);
            debugCompareBoard<Set>(brdBefore, brd);

            //std::cout << "Eval: "<<(isBlack ? evaluate<Set, Size, L, T, false>(chess, chess.origIndex[0],0) : evaluate<Set, Size, L, T, true>(chess, chess.origIndex[0],0)) << std::endl;
        }
        else if (option == "moves")
        {   
            std::cout << "Enter Color(w/b): ";
            std::string color;
            std::getline(std::cin, color);
            bool isBlack = (color == "b");

            std::vector<Move> moves;
            moves.reserve(100);
            isBlack ? chess.template generateMoves<false>(moves, chess.origIndex[0]): chess.template generateMoves<true>(moves, chess.origIndex[0]);
            
            for (int i = 0; i < moves.size(); ++i){
              std::cout << (isBlack ? chess.template moveToPGN<false>(moves[i]) : chess.template moveToPGN<true>(moves[i]))  << std::endl;  
            }
        }
        else if (option == "exit" || option == "q")
        {
            break;
        }
        else
        {
            std::cout << "Invalid option. Please enter 'move', 'engine', or 'exit'." << std::endl;
        }
    }
}

int main()
{
    // To allow unicode characters

    system("chcp 65001 > nul");

    std::ofstream outputFile("output.txt");

    constexpr U8 Set = Chess5D::NoPiece;
    constexpr U8 Size = 8;
    constexpr U16 L = 32;
    constexpr U16 T = 128;

    constexpr bool White = true;

    /*
    Chess5D::Chess<Set, Size, L, T> chess{};
    Positions::load(chess, 7);
    std::cout << chess;

    Chess5D::TimelineInfo info = chess.timelineInfo[chess.origIndex[1]-1];
    Chess5D::Board<Set> &brd = chess.boards[chess.origIndex[1]-1][info.turn];

    std::vector<Chess5D::Move> moves;
    moves.reserve(100);
    chess.template generateMoves<White>(moves, chess.origIndex[1]-1);

    std::cout << "MOVE LENGTH" << moves.size() << std::endl;
    for (int i = 0; i < moves.size(); ++i)
    {
        std::cout << "MOVE" << i << std::endl;
        Chess5D::Move move = moves[i];
        std::cout << chess.template moveToPGN<White>(move) << std::endl;
    }
    */
    playChess<Set, Size, L, T>();

    return 0;
}