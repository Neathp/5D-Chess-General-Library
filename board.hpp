#pragma once

#include <vector>
#include <map>
#include "lookup.hpp"

namespace Chess5D
{
  struct Move
  {
    U8 from = 0;
    U8 to = 0;
    U8 special1 = 0;
    U8 special2 = 0;

    U8 sTimeline = 0;
    U8 sTurn = 0;
    U8 eTimeline = 0;
    U8 eTurn = 0;

    U8 type = 0;
    int score = 0;
    int E = 0;

    constexpr Move() {}

    constexpr Move(const U8 f, const U8 t, const U8 s1, const U8 s2, const U8 tp, const U8 sl, const U8 st, const U8 el, const U8 et) : from(f), to(t), special1(s1), special2(s2), type(tp), sTimeline(sl), sTurn(st), eTimeline(el), eTurn(et)
    {
    }

    // Equality operator overloading
    constexpr bool operator==(const Move &other) const
    {
      return (from == other.from) &&
             (to == other.to) &&
             (special1 == other.special1) &&
             (special2 == other.special2) &&
             (sTimeline == other.sTimeline) &&
             (sTurn == other.sTurn) &&
             (eTimeline == other.eTimeline) &&
             (eTurn == other.eTurn) &&
             (type == other.type);
    }
  };

  struct TimelineInfo
  {
    U8 timeline{0};
    U8 turn{0};
    U8 tailIndex{0};

    U64 pinHV{};
    U64 pinD12{};
    U64 doublePin{}; // actually the not of double pinned pieces
    U64 pinMasks[64]{};
    U64 checkMasks[64]{};
  };

  template <U8 Set>
  struct Board
  {
    struct
    {
      Piece mailboxBoard[64]{NoPiece};
      U64 bitBoard[Set]{EMPTY};
      U64 occ{FULL};
      U64 white{EMPTY};
      U64 black{EMPTY};
      U64 unmoved{EMPTY};
      U64 epTarget{EMPTY};
    } board; // TODO: rename/reorganize so that there isnt a board within board.

    struct
    {
      U64 north{EMPTY};
      U64 east{EMPTY};
      U64 south{EMPTY};
      U64 west{EMPTY};
      U64 northeast{EMPTY};
      U64 southeast{EMPTY};
      U64 southwest{EMPTY};
      U64 northwest{EMPTY};
      U64 center{EMPTY};
    } pastMask;

    U64 checkMask{EMPTY};
    U64 pastCheck{FULL};
    U64 banMask{EMPTY};
    bool traveled = false;

    template <bool White, PieceType Type>
    _Compiletime U64 &bitBoard();

    _Compiletime U64 bitBoard(const bool White, const PieceType Type) const;
    _Compiletime U64 pawns(const bool White) const;
    _Compiletime U64 bishops(const bool White, const bool Royal) const;
    _Compiletime U64 rooks(const bool White, const bool Royal) const;
    _Compiletime U64 kings(const bool White, const bool Royal) const;
    _Compiletime U64 unicorns(const bool White, const bool Royal) const;
    _Compiletime U64 dragons(const bool White, const bool Royal) const;
    _Compiletime U64 royalty(const bool White) const;

    template <U8 Size, bool White, bool Check>
    _Compiletime void genPawnMoves(std::vector<Move> &moves, const TimelineInfo &info, const U64 legalMask, const U64 notPin, const U64 notPinHV, const U64 notPinD12, const TMask &tMask) const;
    template <U8 Size, bool White>
    _Compiletime void genCastle(std::vector<Move> &moves, const TimelineInfo &info, const U64 banMask) const;

    template <bool White, MoveType Type>
    _Compiletime void makeMove(const Move &move);
    template <bool White, bool Capture>
    _Compiletime void makeMoveTravel(const Move &move, Piece piece);
    template <bool White>
    _Compiletime void refresh(TimelineInfo &info);

    template <U8 Size, bool White>
    _Compiletime void genBitMoves(std::vector<Move> &moves, const TimelineInfo &info, const U64 &legalMask, const U64 &pastCheckMask) const;
    template <U8 Size, bool White>
    _Compiletime void genRoyalBitMoves(std::vector<Move> &moves, const TimelineInfo &info, const U64 &pastCheckMask) const;

    _Compiletime Board()
    {
      for (U16 i = 0; i < 64; ++i)
        board.mailboxBoard[i] = NoPiece;
    }
  };

  template <U8 Set>
  void printMasks(Board<Set> &brd)
  {
    std::cout << "north:      " << brd.pastMask.north << std::endl;
    std::cout << "east:       " << brd.pastMask.east << std::endl;
    std::cout << "south:      " << brd.pastMask.south << std::endl;
    std::cout << "west:       " << brd.pastMask.west << std::endl;
    std::cout << "northeast:  " << brd.pastMask.northeast << std::endl;
    std::cout << "southeast:  " << brd.pastMask.southeast << std::endl;
    std::cout << "southwest:  " << brd.pastMask.southwest << std::endl;
    std::cout << "northwest:  " << brd.pastMask.northwest << std::endl;
    std::cout << "center:     " << brd.pastMask.center << std::endl;
        
    U64 combined = brd.pastMask.north | brd.pastMask.east | brd.pastMask.south | brd.pastMask.west |
                   brd.pastMask.northeast | brd.pastMask.southeast | brd.pastMask.southwest | brd.pastMask.northwest | brd.pastMask.center;
    std::cout << "Combined OR: " << combined << std::endl;
  }

  struct LT
  {
    int timeline;
    int turn;
    constexpr LT(const int L, const int T) : timeline(L), turn(T)
    {
    }
  };

  template <Direction Dir>
  _Compiletime static LT dirShift()
  {
    return Dir == North       ? LT(1, 0)
           : Dir == East      ? LT(0, 1)
           : Dir == South     ? LT(-1, 0)
           : Dir == West      ? LT(0, -1)
           : Dir == NorthEast ? LT(1, 1)
           : Dir == SouthEast ? LT(-1, 1)
           : Dir == SouthWest ? LT(-1, -1)
           : Dir == NorthWest ? LT(1, -1)
                              : LT(0, 0);
  }

  template <Direction Dir>
  _Compiletime static U64 Not()
  {
    return Dir == North       ? 0xff00000000000000
           : Dir == East      ? 0x7f7f7f7f7f7f7f7f
           : Dir == South     ? 0x00000000000000ff
           : Dir == West      ? 0xfefefefefefefefe
           : Dir == NorthEast ? 0xff80808080808080
           : Dir == SouthEast ? 0x80808080808080ff
           : Dir == SouthWest ? 0x01010101010101ff
           : Dir == NorthWest ? 0xff01010101010101
                              : EMPTY;
  }

  template <bool White, Direction Dir>
  _Compiletime static U64 pawnShift(const U64 mask)
  {
    return White              ? Dir == North       ? mask << 8
                                : Dir == NorthWest ? mask << 7
                                : Dir == NorthEast ? mask << 9
                                                   : mask << 16
           : Dir == North     ? mask >> 8
           : Dir == NorthWest ? mask >> 9
           : Dir == NorthEast ? mask >> 7
                              : mask >> 16;
  }

  template <bool White, Direction Dir>
  _Compiletime static U8 pawnSquare(const U8 sq)
  {
    return White              ? Dir == North       ? sq + 8
                                : Dir == NorthWest ? sq + 7
                                : Dir == NorthEast ? sq + 9
                                                   : sq + 16
           : Dir == North     ? sq - 8
           : Dir == NorthWest ? sq - 9
           : Dir == NorthEast ? sq - 7
                              : sq - 16;
  }

  template <bool White, PieceType Type>
  _Compiletime static void updateMasks(const U64 &friendly, const U64 &occ, const U64 &royaltyOcc, const U64 &pieces, const U8 &sq, const U16 &offset, U64 &pinDir, U64 &curCheckMask, TimelineInfo &info)
  {
    const U64 attackMask = Type == Rook ? Lookup::RookMask[sq] : Lookup::BishopMask[sq];
    if (pieces & attackMask)
    {
      U64 attackers = Lookup::movement<Type>(sq, royaltyOcc) & pieces;
      Bitloop(attackers) curCheckMask &= Lookup::PinBetween[offset + SquareOf(attackers)];

      U64 pinners = pieces & Lookup::xray<Type>(sq, occ);
      Bitloop(pinners)
      {
        const U64 pin = Lookup::PinBetween[offset + SquareOf(pinners)];
        pinDir |= info.pinMasks[SquareOf(pin & friendly)] = pin;
      }

      (Type == Rook ? info.pinHV : info.pinD12) |= pinDir;
    }
  }

  template <bool White, Direction Dir>
  _Compiletime static void pawnPrune(const TimelineInfo &info, U64 &pawn)
  {
    constexpr bool horizonal = Dir == North || Dir == South;
    U64 pinned = pawn & (horizonal ? info.pinHV : info.pinD12);
    pawn ^= pinned;

    Bitloop(pinned)
    {
      const U64 sq = SquareOf(pinned);
      pawn |= 1ull << sq & pawnShift<!White, Dir>(info.pinMasks[sq]);
    }
  }

  template <U8 Set, bool White, bool BitSolver, Direction Dir, MoveType Type>
  _Compiletime static void pawnMoves(std::vector<Move> &moves, const TimelineInfo &info, U64 &pawn, const U8 &solverSq)
  {
    constexpr bool promo = Type == Promotion || Type == PromoCapture;

    Bitloop(pawn)
    {
      const U8 sq = SquareOf(pawn);
      const U8 to = BitSolver ? solverSq : pawnSquare<White, Dir>(sq);
      if (promo && Set > WKnight)
        moves.emplace_back(sq, to, toPiece(White, Knight), 0, Type, info.timeline, info.turn, 0, 0);
      if (promo && Set > WBishop)
        moves.emplace_back(sq, to, toPiece(White, Bishop), 0, Type, info.timeline, info.turn, 0, 0);
      if (promo && Set > WRook)
        moves.emplace_back(sq, to, toPiece(White, Rook), 0, Type, info.timeline, info.turn, 0, 0);
      if (promo && Set > WQueen)
        moves.emplace_back(sq, to, toPiece(White, Queen), 0, Type, info.timeline, info.turn, 0, 0);
      if (promo && Set > WPrincess)
        moves.emplace_back(sq, to, toPiece(White, Princess), 0, Type, info.timeline, info.turn, 0, 0);
      if (promo && Set > WCKing)
        moves.emplace_back(sq, to, toPiece(White, CKing), 0, Type, info.timeline, info.turn, 0, 0);
      if (promo && Set > WUnicorn)
        moves.emplace_back(sq, to, toPiece(White, Unicorn), 0, Type, info.timeline, info.turn, 0, 0);
      if (promo && Set > WDragon)
        moves.emplace_back(sq, to, toPiece(White, Dragon), 0, Type, info.timeline, info.turn, 0, 0);
      if (!promo)
        moves.emplace_back(sq, to, 0, 0, Type, info.timeline, info.turn, 0, 0);
    }
  }

  template <PieceType Type>
  _Compiletime static void spatialMoves(std::vector<Move> &moves, const TimelineInfo &info, const U8 &sq, const U64 &movable, const U64 &occ, const U64 &enemy)
  {
    U64 move = Lookup::movement<Type>(sq, occ) & movable;
    U64 cap = move & enemy;
    move ^= cap;
    Bitloop(move) moves.emplace_back(sq, SquareOf(move), 0, 0, Normal, info.timeline, info.turn, 0, 0);
    Bitloop(cap) moves.emplace_back(sq, SquareOf(cap), 0, 0, Capture, info.timeline, info.turn, 0, 0);
  }

  template <Direction Dir>
  _Compiletime static void nonSpatialMoves(std::vector<Move> &moves, const TimelineInfo &info, const U8 &sq, U64 move, U64 cap)
  {
    Bitloop(move)
    {
      const U8 mvSq = SquareOf(move);
      const U8 dist = Lookup::Dist[(sq << 6) + mvSq];

      const U8 eTimeline = info.timeline + dist * dirShift<Dir>().timeline;
      const U8 eTurn = info.turn + 2 * dist * dirShift<Dir>().turn;

      moves.emplace_back(sq, mvSq, 0, 0, Travel, info.timeline, info.turn, eTimeline, eTurn);
    }
    Bitloop(cap)
    {
      const U8 mvSq = SquareOf(cap);
      const U8 dist = Lookup::Dist[(sq << 6) + mvSq];

      const U8 eTimeline = info.timeline + dist * dirShift<Dir>().timeline;
      const U8 eTurn = info.turn + 2 * dist * dirShift<Dir>().turn;

      moves.emplace_back(sq, mvSq, 0, 0, TravelCapture, info.timeline, info.turn, eTimeline, eTurn);
    }
  }

  template <PieceType diag, PieceType orth>
  _Compiletime static void travelMaskMoves(std::vector<Move> &moves, const TimelineInfo &info, const TMask &tMask, U64 sq)
  {
    U512 d[7];

    for (int i = 0; i < 7; ++i) // can probably combine with loops below? but for simplicity rn ill keep like this
    {
      d[i] = U512Set(Lookup::Rings[sq][i], Lookup::Rings[sq][i], Lookup::Rings[sq][i], Lookup::Rings[sq][i], Lookup::Rings[sq][i], Lookup::Rings[sq][i], Lookup::Rings[sq][i], 0);
    }

    U512 occ = tMask.o[0] & d[0];
    U512 legal = tMask.m[0] & d[0];
    U512 enemy = tMask.e[0] & d[0];

    for (int i = 1; i < 7; ++i)
    {
      occ |= tMask.o[i] & d[i];
      legal |= tMask.m[i] & d[i];
      enemy |= tMask.e[i] & d[i];
    }

    const U512 pext = {Lookup::movement<diag>(sq, occ[0]), Lookup::movement<orth>(sq, occ[1]), Lookup::movement<diag>(sq, occ[2]), Lookup::movement<orth>(sq, occ[3]),
                       Lookup::movement<diag>(sq, occ[4]), Lookup::movement<orth>(sq, occ[5]), Lookup::movement<diag>(sq, occ[6]), 0};

    U512 move = pext & legal;
    U512 cap = move & enemy;

    move ^= cap;

    nonSpatialMoves<NorthEast>(moves, info, sq, move[0], cap[0]);
    nonSpatialMoves<North>(moves, info, sq, move[1], cap[1]);
    nonSpatialMoves<NorthWest>(moves, info, sq, move[2], cap[2]);
    nonSpatialMoves<West>(moves, info, sq, move[3], cap[3]);
    nonSpatialMoves<SouthWest>(moves, info, sq, move[4], cap[4]);
    nonSpatialMoves<South>(moves, info, sq, move[5], cap[5]);
    nonSpatialMoves<SouthEast>(moves, info, sq, move[6], cap[6]);
  }

  template <U8 Set>
  template <bool White, PieceType Type>
  _Compiletime U64 &Board<Set>::bitBoard()
  {
    return Type < NoType ? board.bitBoard[toPiece(White, Type)] : White ? board.white
                                                                        : board.black;
  }

  template <U8 Set>
  _Compiletime U64 Board<Set>::bitBoard(const bool White, const PieceType Type) const
  {
    return Type < NoType ? board.bitBoard[toPiece(White, Type)] : White ? board.white
                                                                        : board.black;
  }

  template <U8 Set>
  _Compiletime U64 Board<Set>::pawns(const bool White) const
  {
    return Set > WBrawn  ? bitBoard(White, Pawn) | bitBoard(White, Brawn)
           : Set > WPawn ? bitBoard(White, Pawn)
                         : EMPTY;
  }

  template <U8 Set>
  _Compiletime U64 Board<Set>::bishops(const bool White, const bool Royal) const
  {
    return Set > WRQueen && Royal ? bitBoard(White, Bishop) | bitBoard(White, Queen) | bitBoard(White, Princess) | bitBoard(White, RQueen)
           : Set > WPrincess      ? bitBoard(White, Bishop) | bitBoard(White, Queen) | bitBoard(White, Princess)
           : Set > WQueen         ? bitBoard(White, Bishop) | bitBoard(White, Queen)
           : Set > WBishop        ? bitBoard(White, Bishop)
                                  : EMPTY;
  }

  template <U8 Set>
  _Compiletime U64 Board<Set>::rooks(const bool White, const bool Royal) const
  {
    return Set > WRQueen && Royal ? bitBoard(White, Rook) | bitBoard(White, Queen) | bitBoard(White, Princess) | bitBoard(White, RQueen)
           : Set > WPrincess      ? bitBoard(White, Rook) | bitBoard(White, Queen) | bitBoard(White, Princess)
           : Set > WQueen         ? bitBoard(White, Rook) | bitBoard(White, Queen)
           : Set > WRook          ? bitBoard(White, Rook)
                                  : EMPTY;
  }

  template <U8 Set>
  _Compiletime U64 Board<Set>::kings(const bool White, const bool Royal) const
  {
    return Set > WCKing           ? bitBoard(White, King) | bitBoard(White, CKing)
           : Set > WKing && Royal ? bitBoard(White, King)
                                  : EMPTY;
  }

  template <U8 Set>
  _Compiletime U64 Board<Set>::unicorns(const bool White, const bool Royal) const
  {
    return Set > WUnicorn           ? bitBoard(White, Queen) | bitBoard(White, RQueen) | bitBoard(White, Unicorn)
           : Set > WRQueen && Royal ? bitBoard(White, Queen) | bitBoard(White, RQueen)
           : Set > WQueen           ? bitBoard(White, Queen)
                                    : EMPTY;
  }

  template <U8 Set>
  _Compiletime U64 Board<Set>::dragons(const bool White, const bool Royal) const
  {
    return Set > WDragon            ? bitBoard(White, Queen) | bitBoard(White, RQueen) | bitBoard(White, Dragon)
           : Set > WRQueen && Royal ? bitBoard(White, Queen) | bitBoard(White, RQueen)
           : Set > WQueen           ? bitBoard(White, Queen)
                                    : EMPTY;
  }

  template <U8 Set>
  _Compiletime U64 Board<Set>::royalty(const bool White) const
  {
    return Set > WRQueen ? bitBoard(White, King) | bitBoard(White, RQueen)
           : Set > WKing ? bitBoard(White, King)
                         : EMPTY;
  }

  _Compiletime void bitsToMoves(std::vector<Move> &moves, U64 move, MoveType type, U8 sTimeline, U8 sTurn, U8 eTimeline, U8 eTurn, int shift)
  {
    Bitloop(move)
    {
      const U8 mvSq = SquareOf(move);
      moves.emplace_back(mvSq + shift, mvSq, 0, 0, TravelCapture, sTimeline, sTurn, eTimeline, eTurn);
    }
  }

  template <bool HasSq = true>
  _Compiletime void jumpMoves(std::vector<Move> &moves, U64 move, U64 cap, U8 sq, U8 sTimeline, U8 sTurn, LT shift)
  {
    Bitloop(move) moves.emplace_back(HasSq ? sq : SquareOf(move), SquareOf(move), 0, 0, Travel, sTimeline, sTurn, sTimeline + shift.timeline, sTurn + 2 * shift.turn);
    Bitloop(cap) moves.emplace_back(HasSq ? sq : SquareOf(cap), SquareOf(cap), 0, 0, TravelCapture, sTimeline, sTurn, sTimeline + shift.timeline, sTurn + 2 * shift.turn);
  }

  template <bool Royal>
  _Compiletime void kingTravels(std::vector<Move> &moves, U8 sq, U8 sTimeline, U8 sTurn, TMask tMask)
  {
    U512 kingAttack = U512Set1(Lookup::KingTravels[sq]);

    U512 legal = kingAttack & (Royal ? tMask.m[0] & tMask.b[0] : tMask.m[0]);
    U512 move = legal & ~tMask.o[0];
    U512 cap = legal & tMask.e[0];

    jumpMoves(moves, move[0], cap[0], sq, sTimeline, sTurn, dirShift<NorthEast>());
    jumpMoves(moves, move[1], cap[1], sq, sTimeline, sTurn, dirShift<North>());
    jumpMoves(moves, move[2], cap[2], sq, sTimeline, sTurn, dirShift<NorthWest>());
    jumpMoves(moves, move[3], cap[3], sq, sTimeline, sTurn, dirShift<West>());
    jumpMoves(moves, move[4], cap[4], sq, sTimeline, sTurn, dirShift<SouthWest>());
    jumpMoves(moves, move[5], cap[5], sq, sTimeline, sTurn, dirShift<South>());
    jumpMoves(moves, move[6], cap[6], sq, sTimeline, sTurn, dirShift<SouthEast>());
  }

  _Compiletime void knightTravels(std::vector<Move> &moves, U8 sq, U8 sTimeline, U8 sTurn, TMask tMask)
  { // excludes strictly-LT
    U512 knightAttack = U512Set(Lookup::Knight1Attacks[sq], Lookup::Knight2Attacks[sq], Lookup::Knight1Attacks[sq], Lookup::Knight2Attacks[sq],
                                Lookup::Knight1Attacks[sq], Lookup::Knight2Attacks[sq], 0, 0);

    U512 movable = U512Set(tMask.m[0][1], tMask.m[1][1], tMask.m[0][3], tMask.m[1][3],
                           tMask.m[0][5], tMask.m[1][5], 0, 0);

    U512 notOcc = ~U512Set(tMask.o[0][1], tMask.o[1][1], tMask.o[0][3], tMask.o[1][3],
                           tMask.o[0][5], tMask.o[1][5], 0, 0);

    U512 enemy = U512Set(tMask.e[0][1], tMask.e[1][1], tMask.e[0][3], tMask.e[1][3],
                         tMask.e[0][5], tMask.e[1][5], 0, 0);

    U512 legal = knightAttack & movable;
    U512 move = legal & notOcc;
    U512 cap = legal & enemy;

    jumpMoves(moves, move[0], cap[0], sq, sTimeline, sTurn, LT(1, 0));
    jumpMoves(moves, move[1], cap[1], sq, sTimeline, sTurn, LT(2, 0));
    jumpMoves(moves, move[2], cap[2], sq, sTimeline, sTurn, LT(0, -1));
    jumpMoves(moves, move[3], cap[3], sq, sTimeline, sTurn, LT(0, -2));
    jumpMoves(moves, move[4], cap[4], sq, sTimeline, sTurn, LT(-1, 0));
    jumpMoves(moves, move[5], cap[5], sq, sTimeline, sTurn, LT(-2, 0));
  }

  template <U8 Set>
  template <U8 Size, bool White, bool Check>
  _Compiletime void Board<Set>::genPawnMoves(std::vector<Move> &moves, const TimelineInfo &info, const U64 legalMask, const U64 notPin, const U64 notPinHV, const U64 notPinD12, const TMask &tMask) const
  {
    constexpr U64 lastRank = White ? 0xffull << (8 * Size - 8) : 0xffull;

    const U64 pawn = pawns(White) & info.doublePin;
    const U64 pawnHV = pawn & notPinD12;
    const U64 pawnLR = pawn & notPinHV;

    const U64 empty = ~board.occ;
    const U64 enemy = (bitBoard(!White, NoType) | pawnShift<White, North>(board.epTarget)) & legalMask;

    U64 pawnF = pawnHV & pawnShift<!White, North>(empty);
    U64 pawnP = pawnF & board.unmoved & pawnShift<!White, South>(empty & legalMask);
    pawnF &= pawnShift<!White, North>(legalMask);

    U64 pawnR = pawnLR & pawnShift<!White, NorthWest>(enemy & Not<West>());
    U64 pawnL = pawnLR & pawnShift<!White, NorthEast>(enemy & Not<East>());

    pawnPrune<White, North>(info, pawnF);
    pawnPrune<White, South>(info, pawnP);
    pawnPrune<White, NorthWest>(info, pawnR);
    pawnPrune<White, NorthEast>(info, pawnL);

    if (board.epTarget)
    {
      const U64 pawnEPR = pawnR & (board.epTarget & Not<West>()) >> 1;
      const U64 pawnEPL = pawnL & (board.epTarget & Not<East>()) << 1;
      pawnR ^= pawnEPR;
      pawnL ^= pawnEPL;

      const U8 epSq = SquareOf(board.epTarget);
      U64 pawnEP = pawnEPR | pawnEPL;
      Bitloop(pawnEP) moves.emplace_back(SquareOf(pawnEP), pawnSquare<White, North>(epSq), epSq, 0, Enpassant, info.timeline, info.turn, 0, 0);
    }

    constexpr U64 lastRankF = pawnShift<!White, North>(lastRank);
    constexpr U64 lastRankP = pawnShift<!White, South>(lastRank);
    constexpr U64 lastRankL = pawnShift<!White, NorthEast>(lastRank & Not<East>());
    constexpr U64 lastRankR = pawnShift<!White, NorthWest>(lastRank & Not<West>());

    if ((pawnF & lastRankF) | (pawnP & lastRankP) | (pawnL & lastRankL) | (pawnR & lastRankR))
    {
      U64 proF = pawnF & lastRankF;
      U64 proP = pawnP & lastRankP;
      U64 proL = pawnR & lastRankL;
      U64 proR = pawnL & lastRankR;

      pawnF ^= proF;
      pawnP ^= proP;
      pawnR ^= proL;
      pawnL ^= proR;

      pawnMoves<Set, White, false, North, Promotion>(moves, info, proF, 0);
      pawnMoves<Set, White, false, South, Promotion>(moves, info, proP, 0);
      pawnMoves<Set, White, false, NorthEast, PromoCapture>(moves, info, proL, 0);
      pawnMoves<Set, White, false, NorthWest, PromoCapture>(moves, info, proR, 0);
    }

    pawnMoves<Set, White, false, North, Normal>(moves, info, pawnF, 0);
    pawnMoves<Set, White, false, South, Push>(moves, info, pawnP, 0);
    pawnMoves<Set, White, false, NorthEast, Capture>(moves, info, pawnR, 0);
    pawnMoves<Set, White, false, NorthWest, Capture>(moves, info, pawnL, 0);

    if (!Check)
    {
      U64 brawns = 0;
      if (Set > WBrawn)
        brawns = bitBoard(White, Brawn) & notPin;
      U64 pawnlike = pawn & notPin;

      U8 dirShift = White ? 1 : 5;
      U8 dirShiftL = White ? 2 : 4;
      U8 dirShiftR = White ? 0 : 6;
      U8 dirShiftT = 3;

      // gen pawnlike moves
      U64 move = pawnlike & ~tMask.o[0][dirShift];
      U64 move2 = move & board.unmoved & ~tMask.o[1][dirShift] & tMask.m[1][dirShift];
      move &= tMask.m[0][dirShift];

      bitsToMoves(moves, move, Travel, info.timeline, info.turn, info.timeline + White ? 1 : -1, info.turn, 0);
      bitsToMoves(moves, move2, Travel, info.timeline, info.turn, info.timeline + White ? 2 : -2, info.turn, 0);

      move = pawnlike & tMask.e[0][dirShiftL] & tMask.m[0][dirShiftL];
      bitsToMoves(moves, move, TravelCapture, info.timeline, info.turn, info.timeline + White ? 1 : -1, info.turn - 2, 0);

      move = pawnlike & tMask.e[0][dirShiftR] & tMask.m[0][dirShiftR];
      bitsToMoves(moves, move, TravelCapture, info.timeline, info.turn, info.timeline + White ? 1 : -1, info.turn + 2, 0);

      if (Set > WBrawn)
      {
        move = pawnShift<White, North>(brawns) & tMask.e[0][dirShiftT] & tMask.m[0][dirShiftT];
        U64 promoMoves = move & lastRank;
        move ^= promoMoves;
        bitsToMoves(moves, move, TravelCapture, info.timeline, info.turn, info.timeline, info.turn - 2, pawnSquare<!White, North>(0));

        Bitloop(promoMoves)
        {
          const U8 sq = SquareOf(promoMoves);
          const U8 to = pawnSquare<White, North>(sq);

          if (Set > WKnight)
            moves.emplace_back(sq, to, toPiece(White, Knight), 0, TravelPromoCapture, info.timeline, info.turn, info.timeline, info.turn - 2);
          if (Set > WBishop)
            moves.emplace_back(sq, to, toPiece(White, Bishop), 0, TravelPromoCapture, info.timeline, info.turn, info.timeline, info.turn - 2);
          if (Set > WRook)
            moves.emplace_back(sq, to, toPiece(White, Rook), 0, TravelPromoCapture, info.timeline, info.turn, info.timeline, info.turn - 2);
          if (Set > WQueen)
            moves.emplace_back(sq, to, toPiece(White, Queen), 0, TravelPromoCapture, info.timeline, info.turn, info.timeline, info.turn - 2);
          if (Set > WPrincess)
            moves.emplace_back(sq, to, toPiece(White, Princess), 0, TravelPromoCapture, info.timeline, info.turn, info.timeline, info.turn - 2);
          if (Set > WCKing)
            moves.emplace_back(sq, to, toPiece(White, CKing), 0, TravelPromoCapture, info.timeline, info.turn, info.timeline, info.turn - 2);
          if (Set > WUnicorn)
            moves.emplace_back(sq, to, toPiece(White, Unicorn), 0, TravelPromoCapture, info.timeline, info.turn, info.timeline, info.turn - 2);
          if (Set > WDragon)
            moves.emplace_back(sq, to, toPiece(White, Dragon), 0, TravelPromoCapture, info.timeline, info.turn, info.timeline, info.turn - 2);
        }

        move = pawnShift<White, North>(brawns) & tMask.e[0][dirShift] & tMask.m[0][dirShift];
        promoMoves = move & lastRank;
        move ^= promoMoves;
        bitsToMoves(moves, move, TravelCapture, info.timeline, info.turn, info.timeline + White ? 1 : -1, info.turn, pawnSquare<!White, North>(0));

        Bitloop(promoMoves)
        {
          const U8 sq = SquareOf(promoMoves);
          const U8 to = pawnSquare<White, North>(sq);

          if (Set > WKnight)
            moves.emplace_back(sq, to, toPiece(White, Knight), 0, TravelPromoCapture, info.timeline, info.turn, info.timeline + White ? 1 : -1, info.turn);
          if (Set > WBishop)
            moves.emplace_back(sq, to, toPiece(White, Bishop), 0, TravelPromoCapture, info.timeline, info.turn, info.timeline + White ? 1 : -1, info.turn);
          if (Set > WRook)
            moves.emplace_back(sq, to, toPiece(White, Rook), 0, TravelPromoCapture, info.timeline, info.turn, info.timeline + White ? 1 : -1, info.turn);
          if (Set > WQueen)
            moves.emplace_back(sq, to, toPiece(White, Queen), 0, TravelPromoCapture, info.timeline, info.turn, info.timeline + White ? 1 : -1, info.turn);
          if (Set > WPrincess)
            moves.emplace_back(sq, to, toPiece(White, Princess), 0, TravelPromoCapture, info.timeline, info.turn, info.timeline + White ? 1 : -1, info.turn);
          if (Set > WCKing)
            moves.emplace_back(sq, to, toPiece(White, CKing), 0, TravelPromoCapture, info.timeline, info.turn, info.timeline + White ? 1 : -1, info.turn);
          if (Set > WUnicorn)
            moves.emplace_back(sq, to, toPiece(White, Unicorn), 0, TravelPromoCapture, info.timeline, info.turn, info.timeline + White ? 1 : -1, info.turn);
          if (Set > WDragon)
            moves.emplace_back(sq, to, toPiece(White, Dragon), 0, TravelPromoCapture, info.timeline, info.turn, info.timeline + White ? 1 : -1, info.turn);
        }

        move = (brawns >> 1 & Not<West>()) & tMask.e[dirShift][0] & tMask.m[dirShift][0];
        bitsToMoves(moves, move, TravelCapture, info.timeline, info.turn, info.timeline + White ? 1 : -1, info.turn, 1);

        move = (brawns << 1 & Not<East>()) & tMask.e[dirShift][0] & tMask.m[dirShift][0];
        bitsToMoves(moves, move, TravelCapture, info.timeline, info.turn, info.timeline + White ? 1 : -1, info.turn, -1);
      }
    }
  }

  template <U8 Set>
  template <U8 Size, bool White>
  _Compiletime void Board<Set>::genCastle(std::vector<Move> &moves, const TimelineInfo &info, const U64 banMask) const
  {
    U64 king = bitBoard(White, King) & board.unmoved;
    Bitloop(king)
    {
      const U64 sq = SquareOf(king);
      const U64 bit = 1ull << sq;
      const U64 rank = 0xffull << (sq >> 3 << 3);

      const U64 kingLow = bit - 1;
      const U64 rook = bitBoard(White, Rook) & board.unmoved & rank;
      const U64 legal = rank & banMask;
      const U64 empty = board.occ ^ rook;

      const U64 qRook = rook & 1ull << (63 - __builtin_clzll((kingLow & rook) | 1ull));
      const U64 qLegal = bit - (rank & bit >> 2);

      if (qRook && ((bit - qRook) & empty) == 0 && (qLegal & legal) == qLegal)
        moves.emplace_back(sq, sq - 2, SquareOf(qRook), sq - 1, Castle, info.timeline, info.turn, 0, 0);

      const U64 kRook = _blsi_u64(~kingLow & rook);
      const U64 kLegal = ((rank & bit << 2) - bit) << 1;

      if (kRook && (((kRook - bit) << 1) & empty) == 0 && (kLegal & legal) == kLegal)
        moves.emplace_back(sq, sq + 2, SquareOf(kRook), sq + 1, Castle, info.timeline, info.turn, 0, 0);
    }
  }

  template <PieceType Type, bool Check>
  _Compiletime static void pieceMoves(std::vector<Move> &moves, const TimelineInfo &info, U64 pieces, const U64 &movable, const U64 &occ, const U64 &enemy, const U64 &pin, const TMask &tMask) // honestly should probably be moved to chess.hpp
  {
    constexpr bool pinnable = Type != Knight;
    constexpr bool royal = Type == King || Type == RQueen;
    constexpr bool spatial = Type != Unicorn && Type != Dragon;

    if (pinnable && spatial && !royal) // TODO: could possibly avoid generating this when you are in check and there is only 1 king on the board
    {
      U64 pinPieces = pieces & pin;
      pieces ^= pinPieces;
      Bitloop(pinPieces)
      {
        const U64 sq = SquareOf(pinPieces);
        spatialMoves<Type>(moves, info, sq, movable & info.pinMasks[sq], occ, enemy);
      }
    }
    U64 piecesNoPin = pieces & ~pin;
    if (spatial)
    {
      Bitloop(pieces)
      {
        const U64 sq = SquareOf(pieces);
        spatialMoves<Type>(moves, info, sq, royal ? movable & info.checkMasks[sq] : movable, occ, enemy);
      }
    }
    if (!Check || royal)
    {
      Bitloop(piecesNoPin)
      {
        const U64 sq = SquareOf(piecesNoPin);

        if (royal && info.checkMasks[sq] != FULL)
          continue;

        constexpr PieceType orth = Type == RQueen                       ? RQueen
                                   : Type == Queen                      ? Queen
                                   : Type == Unicorn                    ? Bishop
                                   : Type == Bishop || Type == Princess ? Rook
                                                                        : NoType;
        constexpr PieceType diag = Type == RQueen    ? RQueen
                                   : Type == Queen   ? Queen
                                   : Type == Dragon  ? Bishop
                                   : Type == Unicorn ? Rook
                                                     : NoType;

        if (orth != NoType && diag != NoType)
          travelMaskMoves<orth, diag>(moves, info, tMask, sq);

        if (Type == King || Type == CKing)
          kingTravels<royal>(moves, sq, info.timeline, info.turn, tMask);

        if (Type == Knight)
        {
          knightTravels(moves, sq, info.timeline, info.turn, tMask);
        }
      }
    }
  }

  template <bool White, PieceType Type>
  _Compiletime static void pieceBitMoves(std::vector<Move> &moves, const TimelineInfo &info, U64 pieces, const U64 &movable, const U64 &enemy, const U64 &pin)
  {
    constexpr bool pinnable = Type != Knight;
    constexpr bool nonRoyal = Type != King && Type != RQueen;
    constexpr bool spatial = Type != Unicorn && Type != Dragon;

    if (pinnable && spatial && nonRoyal)
    {
      U64 pinPieces = pieces & pin;
      pieces ^= pinPieces;
      Bitloop(pinPieces)
      {
        const U64 sq = SquareOf(pinPieces);
        if (movable & info.pinMasks[sq])
          moves.emplace_back(sq, enemy, 0, 0, Capture, info.timeline, info.turn, 0, 0);
      }
    }
    Bitloop(pieces)
    {
      const U64 sq = SquareOf(pieces);
      if (nonRoyal || movable & info.checkMasks[sq])
        moves.emplace_back(sq, enemy, 0, 0, Capture, info.timeline, info.turn, 0, 0);
    }
  }

  template <U8 Set>
  template <U8 Size, bool White>
  _Compiletime void Board<Set>::genBitMoves(std::vector<Move> &moves, const TimelineInfo &info, const U64 &legalMask, const U64 &pastCheckMask) const
  {
    constexpr U64 lastRank = 0xffull | 0xffull << (8 * Size - 8);
    constexpr U64 mask = Size == 1 ? 0x0000000000000001 : Size == 2 ? 0x0000000000000303
                                                      : Size == 3   ? 0x0000000000070707
                                                      : Size == 4   ? 0x000000000f0f0f0f
                                                      : Size == 5   ? 0x0000001f1f1f1f1f
                                                      : Size == 6   ? 0x00003f3f3f3f3f3f
                                                      : Size == 7   ? 0x007f7f7f7f7f7f7f
                                                                    : FULL;

    const U64 pin = info.pinHV | info.pinD12;
    const U64 notPin = ~pin;
    const U64 notPinHV = ~info.pinHV;
    const U64 notPinD12 = ~info.pinD12;

    const U64 movable = legalMask & mask;
    const U64 royalMovable = banMask & pastCheckMask & mask;

    const U8 sq = SquareOf(movable);

    // Pawns and brawns
    if (Set > WPawn)
    {
      const U64 movableEast = movable & Not<East>();
      const U64 movableWest = movable & Not<East>();
      const U64 pawnLR = pawns(White) & info.doublePin & notPinHV;

      const U64 epR = pawnShift<!White, East>(board.epTarget & movableEast);
      const U64 epL = pawnShift<!White, West>(board.epTarget & movableWest);

      U64 pawnR = pawnLR & (epR | pawnShift<!White, NorthEast>(movableEast));
      U64 pawnL = pawnLR & (epL | pawnShift<!White, NorthWest>(movableWest));

      pawnPrune<White, NorthEast>(info, pawnR);
      pawnPrune<White, NorthWest>(info, pawnL);

      U64 pawn = pawnR | pawnL;

      U64 pawnEP = pawn & (epR | epL);
      pawn ^= pawnEP;
      Bitloop(pawnEP) moves.emplace_back(SquareOf(pawnEP), pawnSquare<White, North>(sq), sq, 0, Enpassant, info.timeline, info.turn, 0, 0);

      if (movable & lastRank)
        pawnMoves<Set, White, true, NorthEast, PromoCapture>(moves, info, pawn, sq);
      else
        pawnMoves<Set, White, true, NorthEast, Capture>(moves, info, pawn, sq);
    }

    // All other pieces
    if (Set > WKnight)
      pieceBitMoves<White, Knight>(moves, info, bitBoard(White, Knight) & Lookup::movement<Knight>(sq, 0) & notPin, movable, sq, pin);
    if (Set > WBishop)
      pieceBitMoves<White, NoType>(moves, info, bishops(White, false) & Lookup::movement<Bishop>(sq, board.occ) & notPinHV & info.doublePin, movable, sq, pin);
    if (Set > WRook)
      pieceBitMoves<White, NoType>(moves, info, rooks(White, false) & Lookup::movement<Rook>(sq, board.occ) & notPinD12 & info.doublePin, movable, sq, pin);
    if (Set > WKing)
      pieceBitMoves<White, King>(moves, info, bitBoard(White, King) & Lookup::movement<King>(sq, 0), royalMovable, sq, pin);
    if (Set > WRQueen)
      pieceBitMoves<White, RQueen>(moves, info, bitBoard(White, RQueen) & Lookup::movement<Queen>(sq, board.occ), royalMovable, sq, pin);
    if (Set > WCKing)
      pieceBitMoves<White, NoType>(moves, info, bitBoard(White, CKing) & Lookup::movement<King>(sq, 0) & info.doublePin, movable, sq, pin);
  }

  template <U8 Set>
  template <U8 Size, bool White>
  _Compiletime void Board<Set>::genRoyalBitMoves(std::vector<Move> &moves, const TimelineInfo &info, const U64 &pastCheckMask) const
  {
    constexpr U64 mask = Size == 1 ? 0x0000000000000001 : Size == 2 ? 0x0000000000000303
                                                      : Size == 3   ? 0x0000000000070707
                                                      : Size == 4   ? 0x000000000f0f0f0f
                                                      : Size == 5   ? 0x0000001f1f1f1f1f
                                                      : Size == 6   ? 0x00003f3f3f3f3f3f
                                                      : Size == 7   ? 0x007f7f7f7f7f7f7f
                                                                    : FULL;

    const U64 royalMovable = banMask & pastCheckMask & mask;
    if (royalMovable)
    {
      const U8 sq = SquareOf(royalMovable); // TODO fix. Royalmoveable can be 0 causing sq to be 64

      // All other pieces
      if (Set > WKing)
        pieceBitMoves<White, King>(moves, info, bitBoard(White, King) & Lookup::movement<King>(sq, 0), royalMovable, sq, 0);
      if (Set > WRQueen)
        pieceBitMoves<White, RQueen>(moves, info, bitBoard(White, RQueen) & Lookup::movement<Queen>(sq, board.occ), royalMovable, sq, 0);
    }
  }

  template <U8 Set>
  template <bool White, MoveType Type>
  _Compiletime void Board<Set>::makeMove(const Move &move)
  {
    constexpr bool capture = Type == Capture || Type == Enpassant || Type == PromoCapture;
    constexpr bool push = Type == Push;
    constexpr bool enpassant = Type == Enpassant;
    constexpr bool promotion = Type == Promotion || Type == PromoCapture;
    constexpr bool castle = Type == Castle;

    const U64 from = castle ? 1ull << move.from | 1ull << move.to
                            : 1ull << move.from;
    const U64 to = castle      ? 1ull << move.special1 | 1ull << move.special2
                   : enpassant ? 1ull << move.special1
                               : 1ull << move.to;
    const U64 fromTo = castle ? from ^ to
                              : 1ull << move.from | 1ull << move.to;

    const Piece piece = board.mailboxBoard[move.from];

    if(castle){
      bitBoard<White, King>() ^= from;
      bitBoard<White, Rook>() ^= to;
      board.mailboxBoard[move.special1] = toPiece(White, Rook);
    }
    else{
      board.bitBoard[piece] ^= promotion ? from : fromTo;
    }

    if (promotion)
      board.bitBoard[move.special1] ^= to;
    if (capture){
      bitBoard<!White, NoType>() ^= to;
      board.bitBoard[board.mailboxBoard[enpassant ? move.special1 : move.to]] ^= to;
    }

    bitBoard<White, NoType>() ^= fromTo;
    board.occ ^= enpassant ? fromTo | to : capture ? from
                                                   : fromTo;
    board.unmoved &= ~fromTo;
    board.epTarget = push ? 1ull << move.to : 0;

    board.mailboxBoard[move.from] = NoPiece;
    board.mailboxBoard[move.to] = castle ? toPiece(White, King) : promotion ? Piece(move.special1)
                                                                            : piece;
    if (enpassant)
      board.mailboxBoard[move.special1] = NoPiece;
      
  }

  template <U8 Set>
  template <bool White, bool Capture>
  _Compiletime void Board<Set>::makeMoveTravel(const Move &move, Piece piece)
  {
    const U64 to = 1ull << move.to;
    board.bitBoard[piece] ^= to;
    if (Capture)
    {
      bitBoard<!White, NoType>() ^= to;
      board.bitBoard[board.mailboxBoard[move.to]] ^= to;
      board.unmoved &= ~to;
    }
    else
    {
      board.occ ^= to;
    }
    bitBoard<White, NoType>() ^= to;
    board.epTarget = 0;
    board.mailboxBoard[move.to] = piece;
  }

  template <U8 Set>
  template <bool White>
  _Compiletime void Board<Set>::refresh(TimelineInfo &info)
  {
    // Resetting data
    checkMask = FULL;
    info.pinHV = EMPTY;
    info.pinD12 = EMPTY;
    info.doublePin = EMPTY;

    // Curent royalty
    U64 royal = royalty(White);

    // Enemy pieces
    const U64 ePawns = pawns(!White);
    const U64 ePL = pawnShift<!White, NorthWest>(ePawns & Not<West>());
    const U64 ePR = pawnShift<!White, NorthEast>(ePawns & Not<East>());
    U64 eKnights = bitBoard(!White, Knight);
    U64 eBishops = bishops(!White, true);
    U64 eRooks = rooks(!White, true);
    U64 eKings = kings(!White, true);

    // Removing EP if necessary
    if (board.epTarget)
    {
      const U8 sq = SquareOf(board.epTarget);
      const U64 movement = 0xffull << (sq >> 3 << 3) & Lookup::movement<Rook>(sq, board.occ ^ pawns(White) & ((board.epTarget & Not<East>()) >> 1 | (board.epTarget & Not<West>()) << 1));

      if (royal & movement && eRooks & movement)
        board.epTarget = 0;
    }

    // Generating pinMasks and checkMasks
    U8 n = 0;
    U8 sq[64];
    U64 pins[64];
    const U64 royaltyOcc = board.occ ^ royal;
    Bitloop(royal)
    {
      sq[n] = SquareOf(royal);
      const U16 offset = sq[n] << 6;
      const U64 bit = 1ull << sq[n];

      // Since double check from leaper pieces cannot happen onto one royal piece, this mask will only ever contain at most one bit set.
      U64 curCheckMask = pawnShift<White, NorthEast>(ePL & bit) | pawnShift<White, NorthWest>(ePR & bit) |
                         (Lookup::movement<Knight>(sq[n], 0) & eKnights) | (Lookup::movement<King>(sq[n], 0) & eKings);
      curCheckMask |= -(curCheckMask == 0);

      U64 pinHV = 0;
      U64 pinD12 = 0;
      updateMasks<White, Rook>(bitBoard(White, NoType), board.occ, royaltyOcc, eRooks, sq[n], offset, pinHV, curCheckMask, info);
      updateMasks<White, Bishop>(bitBoard(White, NoType), board.occ, royaltyOcc, eBishops, sq[n], offset, pinD12, curCheckMask, info);
      pins[n] = pinHV | pinD12;

      for (U8 i = 0; i < n; ++i)
      {
        info.checkMasks[sq[i]] &= curCheckMask;
        info.doublePin |= pins[i] & pins[n];
      }

      info.checkMasks[sq[n]] = checkMask;
      checkMask &= curCheckMask;
      ++n;
    }

    info.doublePin = ~info.doublePin;

    // Generate banMask
    banMask = ePL | ePR;
    Bitloop(eKnights) banMask |= Lookup::movement<Knight>(SquareOf(eKnights), 0);
    Bitloop(eKings) banMask |= Lookup::movement<King>(SquareOf(eKings), 0);
    Bitloop(eBishops) banMask |= Lookup::movement<Bishop>(SquareOf(eBishops), royaltyOcc);
    Bitloop(eRooks) banMask |= Lookup::movement<Rook>(SquareOf(eRooks), royaltyOcc);
    banMask = ~banMask;
  }
};