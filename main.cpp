#include <algorithm>
#include "ai.hpp"
#include "positions.hpp"

template <U8 Set, U8 Size, U16 L, U16 T, bool White>
void backtrace(Chess<Set, Size, L, T> &chess, int timeline, int depth)
{
    TimelineInfo info = chess.timelineInfo[timeline];
    Board<Set> &brd = chess.boards[timeline][info.turn];

    U64 key = tt.computeHashKey<Set, White>(brd);
    TTEntry &ttEntry = tt.probe(key);

    if (ttEntry.key == key)
    {
        if(ttEntry.isQSearch) std::cout << "invalid\n";
        Move move = ttEntry.move;
        if (!White)
        {
            if (depth == 0)
            {
                std::cout << "1. ... ";
            }
            std::cout << "/" << chess.template moveToPGN<false>(move) << " {" << move.score << "}" << std::endl;
        }
        else
        {
            std::cout << depth / 2 + 1 + (depth) % 2 << ". " << chess.template moveToPGN<true>(move) << " {" << move.score << "}";
        }

        chess.template makeMove<White>(move);
        if (move.type >= Travel)
        {
            U8 newTimeline = chess.origIndex[White] + (White ? -1 : 1) * (chess.timelineNum[White]);
            backtrace<Set, Size, L, T, !White>(chess, newTimeline, depth + 1);
        }
        else
        {
            backtrace<Set, Size, L, T, !White>(chess, timeline, depth + 1);
        }
        chess.template undoMove<White>(move);
    }
    return;
}
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
            chess.importPGN(pgn);
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

            count = 0;
            hitCount = 0;
            collision = 0;
            auto begin = std::chrono::high_resolution_clock::now();
            // Chess5D::Result res = isBlack ? negaMax<Set, Size, L, T, false>(chess, -CHECKMATE, CHECKMATE, depth, std::vector<Chess5D::Move>()) : negaMax<Set, Size, L, T, true>(chess, -CHECKMATE, CHECKMATE, depth, std::vector<Chess5D::Move>());
            Chess5D::Result res = isBlack ? negaMaxIDDFS<Set, Size, L, T, false>(chess, depth, std::chrono::milliseconds(20000)) : negaMaxIDDFS<Set, Size, L, T, true>(chess, depth, std::chrono::milliseconds(20000));
            auto end = std::chrono::high_resolution_clock::now();
            std::cout << std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() / 1000000000.0 << " s\n";
            std::cout << "Positions: " << count << std::endl;
            std::cout << "Hits: " << hitCount << std::endl;
            std::cout << "Collision: " << collision << std::endl;
            std::cout << chess << std::endl;

            std::cout << res.value << std::endl;

            int timeline = chess.origIndex[!isBlack];

            isBlack ? backtrace<Set, Size, L, T, false>(chess, timeline, 0) : backtrace<Set, Size, L, T, true>(chess, timeline, 0);
            /*
            std::cout << "Regular "<< std::endl;
            int i = 0;
            const Chess5D::Result *resPtr = &res;
            while (resPtr != nullptr)
            {
                for (const auto &move : resPtr->moveset)
                {
                    if (isBlack)
                    {
                        if (i == 0)
                        {
                            std::cout << "1. ... ";
                        }
                        std::cout << "/" << chess.template moveToPGN<false>(move) << " {" << move.score << "}" << std::endl;
                    }
                    else
                    {
                        std::cout << i / 2 + 1 + (i) % 2 << ". " << chess.template moveToPGN<true>(move) << " {" << move.score << "}";
                    }
                    isBlack = !isBlack;
                }
                resPtr = resPtr->next.get();
                i++;
            }
            */

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
                Chess5D::Result res = isBlack ? negaMaxIDDFS<Set, Size, L, T, false>(chess, depth, std::chrono::milliseconds(10000)) : negaMaxIDDFS<Set, Size, L, T, true>(chess, depth, std::chrono::milliseconds(10000));
                isBlack ? chess.template makeMove<false>(res.moveset[0]) : chess.template makeMove<true>(res.moveset[0]);
                std::cout << res.value << std::endl;
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

            std::cout << (isBlack ? evaluate<Set, Size, L, T, false>(chess, chess.origIndex[0],0) : evaluate<Set, Size, L, T, true>(chess, chess.origIndex[0],0)) << std::endl;
        }
        else if (option == "exit")
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