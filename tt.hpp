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
        U64 pastDir[9]; 
    } zobrist;
    

    // Random number generator for Zobrist hashing
    std::mt19937_64 rng;

    TranspositionTable(size_t size) {
        table.reserve(size);

        // Initialize random number generator for Zobrist hashing
        std::random_device rd;
        auto rand=rd();
        rng.seed(64);
        //rng.seed(rand);

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
        for (int d = 0; d < 9; ++d)
            zobrist.pastDir[d] = dist(rng); 
    }

    // Clear the transposition table
    void clear() {
        table.clear();
        table.rehash(0);
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
        
        // Dirty way of doing past masks
        /*
        U64 past=brd.pastMask.center |  brd.pastMask.east | brd.pastMask.northeast | brd.pastMask.north | brd.pastMask.northwest | brd.pastMask.west |  brd.pastMask.southwest | brd.pastMask.south | brd.pastMask.southeast;
        Bitloop(past)
        {
            const U8 pst = SquareOf(past);
            key ^=zobrist.past[pst];
        }
        */
        
        auto addPast = [&](U64 mask, int dir) {
            Bitloop(mask) {
                const U8 sq = SquareOf(mask);
                key ^= zobrist.past[sq] ^ zobrist.pastDir[dir];
            }
        };
        
        addPast(brd.pastMask.center, 0);
        addPast(brd.pastMask.north, 1);
        addPast(brd.pastMask.northeast, 2);
        addPast(brd.pastMask.east, 3);
        addPast(brd.pastMask.southeast, 4);
        addPast(brd.pastMask.south, 5);
        addPast(brd.pastMask.southwest, 6);
        addPast(brd.pastMask.west, 7);
        addPast(brd.pastMask.northwest, 8);
        
        return key;
    }

    TTEntry* probe(U64 key) {
        auto it = table.find(key);
        return (it != table.end()) ? &it->second : nullptr;
    }

    // Store an entry in the transposition table
    void store(const TTEntry& entry) {
        table[entry.key] = entry; //Replacement policy likely needed to prefer deeper searches
    }
};