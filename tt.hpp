#include "chess.hpp"
#include <random>

using namespace Chess5D;

// Constants for board representation (example for chess)
const int RANKS=8;
const int FILES=8;
const int NUM_SQS = RANKS*FILES;  
const int PIECE_TYPES = 24;

struct TTEntry {
    enum Flag { EXACT, LOWERBOUND, UPPERBOUND };

    U64 key; 
    Move move;  
    int depth;      // Depth of search at which this entry was stored
    bool isWhite;
    bool isQSearch;
    int value;      // Score of the position
    int flag;       // Flag indicating exact, lower bound, or upper bound

    TTEntry() : key(0), depth(0), value(0), flag(0) {}
};

struct TranspositionTable { 
    std::unordered_map<U64,TTEntry> table;

    // Zobrist hash keys
    struct Zobrist{
        U64 piece[NUM_SQS][PIECE_TYPES];
        U64 color[2];
        U64 ep[FILES]; //Considers that pawns cant start on anything besides second rank
        U64 unmoved[NUM_SQS];
        U64 past[NUM_SQS];
    } zobrist;
    

    // Random number generator for Zobrist hashing
    std::mt19937_64 rng;

    TranspositionTable(size_t size) {
        table.reserve(size);

        // Initialize random number generator for Zobrist hashing
        std::random_device rd;
        auto rand=rd();
        std::cout<<rand<<std::endl;
        rng.seed(rand);

        // Initialize Zobrist keys
        initZobristKeys();
    }

    // Initialize Zobrist keys with random numbers
    void initZobristKeys() {
        std::uniform_int_distribution<uint64_t> dist(0, UINT64_MAX);
        zobrist.color[0] = dist(rng);
        zobrist.color[1] = dist(rng);
        for (int sq = 0; sq < NUM_SQS; ++sq) {
            for (int piece = 0; piece < PIECE_TYPES; ++piece) {
                zobrist.piece[sq][piece] = dist(rng);
            }
            zobrist.unmoved[sq] = dist(rng);
            zobrist.past[sq] = dist(rng);
        }
        for (int file = 0; file < FILES; ++file) {
            zobrist.ep[file]=dist(rng);
        }
    }

    // Clear the transposition table
    void clear() {
        table.clear();
    }

    // Compute Zobrist hash key for the current board position
    template <U8 Set, bool White>
    U64 computeHashKey(const Board<Set>& brd) { //Should maybe include enpassant in initialization
        U64 key = 0;
        key ^=zobrist.color[(int)White];
        for (int sq = 0; sq < NUM_SQS; ++sq) {
            int piece = brd.board.mailboxBoard[sq]; // Example function to get piece at square
            if (piece != NoPiece) { // Assuming EMPTY is defined
                key ^= zobrist.piece[sq][piece];
            }
        }
        U64 epTarget=brd.board.epTarget;
        Bitloop(epTarget)
        {
            const U8 ep = SquareOf(epTarget);
            key ^=zobrist.ep[ep%FILES];
        }
        U64 unmoved=brd.board.unmoved&(brd.bitBoard(true, King)|brd.bitBoard(false, King)|brd.bitBoard(true, Rook)|brd.bitBoard(false, Rook));
        Bitloop(unmoved)
        {
            const U8 um = SquareOf(unmoved);
            key ^=zobrist.unmoved[um];
        }
        U64 past=brd.pastMask.center | brd.pastMask.north | brd.pastMask.south | brd.pastMask.northeast | brd.pastMask.southeast | brd.pastMask.southwest | brd.pastMask.northwest;
        Bitloop(past)
        {
            const U8 pst = SquareOf(past);
            key ^=zobrist.past[pst];
        }
        return key;
    }

    // Retrieve an entry from the transposition table
    TTEntry& probe(U64 key) {
        TTEntry& entry = table[key];
        return entry;
    }

    // Store an entry in the transposition table
    void store(const TTEntry& entry) {
        table[entry.key] = entry;
    }
};