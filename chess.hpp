#pragma once

#include <sstream>
#include <iostream>
#include <string>
#include <regex>
#include <fstream>
#include "board.hpp"

namespace Chess5D
{
  template <U8 Set, U8 Size, U16 L, U16 T>
  struct Chess
  {
    Board<Set> boards[L + 16][T + 32]{};
    TimelineInfo timelineInfo[L + 16]{};

    U8 origIndex[2]{(L + 16) / 2, (L + 16) / 2};
    U8 timelineNum[2]{0, 0};
    U8 activeNum[2]{0, 0};
    U8 present=0; //should be stored and updated each time a moveset is made

    template <bool White>
    _Compiletime void makeMove(const Move &move);
    template <bool White>
    _Compiletime void undoMove(const Move &move);
    template <bool White>
    _Compiletime void generateMoves(std::vector<Move> &moves, U16 timeline);
    template <bool isWhite>
    _Compiletime std::string moveToPGN(Move move);
    template <bool White>
    _Compiletime Move PGNtoMove(std::smatch matches);
    _Compiletime void importPGN(std::string PGN);
    _Compiletime void importFen(std::string fen);
    _Compiletime void printToFile(std::ofstream &file);

    _Compiletime Chess()
    {
      for (U16 i = 0; i < L + 16; ++i)
        timelineInfo[i].timeline = i;
    }
  };

  template <U8 Set, U8 Size, U16 L, U16 T>
  _Compiletime std::ostream &operator<<(std::ostream &os, const Chess<Set, Size, L, T> &chess)
  {
    const int top = chess.origIndex[0] + chess.timelineNum[0];
    const int bot = chess.origIndex[1] - chess.timelineNum[1];

    int min = chess.timelineInfo[bot].tailIndex;
    int max = chess.timelineInfo[bot].turn;
    for (int i = top; i > bot; --i)
    {
      min = std::min(min, int(chess.timelineInfo[i].tailIndex));
      max = std::max(max, int(chess.timelineInfo[i].turn));
    }

    for (int i = 0; i < 20; ++i)
      os << "----------";
    os << "\n";

    for (int i = min; i <= max; i += 20)
    {
      for (int j = top; j >= bot; --j)
      {
        const int tail = chess.timelineInfo[j].tailIndex;
        const int head = chess.timelineInfo[j].turn;
        if (i + 19 < tail || i > head)
          continue;

        const int cur_min = std::max(i, tail);
        const int cur_max = std::min(i + 19, head);

        for (int k = i; k < cur_min; ++k)
          os << "          ";
        for (int k = cur_min; k <= cur_max; ++k)
        {
          std::string label = std::to_string(chess.origIndex[1] - j) + "," + std::to_string((k - 16) / 2);
          os << "╔" + label + std::string("════════", 24 - 3 * label.length()) + "╗";
        }
        for (int k = cur_max + 1; k < i + 20; ++k)
          os << "          ";
        os << "\n";

        for (int k = 7; k >= 0; --k)
        {
          for (int l = i; l < cur_min; ++l)
            os << "          ";
          for (int l = cur_min; l <= cur_max; ++l)
          {
            os << "║";
            for (size_t m = 0; m < 8; ++m)
              os << pieceToChar(chess.boards[j][l].board.mailboxBoard[k * 8 + m]);
            os << "║";
          }
          for (int l = cur_max + 1; l < i + 20; ++l)
            os << "          ";
          os << "\n";
        }

        for (int k = i; k < cur_min; ++k)
          os << "          ";
        for (int k = cur_min; k <= cur_max; ++k)
          os << "╚════════╝";
        for (int k = cur_max + 1; k < i + 20; ++k)
          os << "          ";
        os << "\n";
      }

      for (size_t j = 0; j < 20; ++j)
        os << "----------";
      os << "\n";
    }

    return os;
  }

  template <U8 Set, U8 Size, U16 L, U16 T>
  template <bool White>
  _Compiletime std::string Chess<Set, Size, L, T>::moveToPGN(Move move)
  {
    std::string notation = "";
    if (move.type == NullMove)
    {
      return "Null";
    }

    notation += "(" + std::to_string(move.sTimeline - origIndex[White]) + "T" + std::to_string((move.sTurn - 16) / 2) + ")";

    Board<Set> &brd = boards[move.sTimeline][move.sTurn];
    notation += toupper(pieceToChar(brd.board.mailboxBoard[move.from]));

    notation += sqToString[move.from];

    if (move.type >= Travel)
    {
      notation += ">>(" + std::to_string(move.eTimeline - origIndex[White]) + "T" + std::to_string((move.eTurn - 16) / 2) + ")";

      Board<Set> &brdTo = boards[move.eTimeline][move.eTurn];
    }

    if (move.type == Capture || move.type == Enpassant || move.type == PromoCapture || move.type == TravelCapture || move.type == TravelPromoCapture)
    {
      notation += "x";
    }

    notation += sqToString[move.to];

    if (move.type == TravelPromoCapture || move.type == Promotion || move.type == PromoCapture)
    {
      notation += "=" + toupper(pieceToChar(static_cast<Piece>(move.special1)));
    }
    return notation;
  }

  template <U8 Set, bool White>
  _Compiletime void refreshMask(Board<Set> *brd)
  {
    const U64 royalty = (brd - 1)->royalty(White);
    const U64 notOcc = ~(brd - 1)->board.occ;
    brd->pastMask.center = ((notOcc & (brd - 2)->pastMask.center) | royalty);
    brd->pastMask.north = ((notOcc & (brd - 2)->pastMask.north) | royalty) << 8;
    brd->pastMask.east = ((notOcc & (brd - 2)->pastMask.east & Not<East>()) | royalty) << 1;
    brd->pastMask.south = ((notOcc & (brd - 2)->pastMask.south) | royalty) >> 8;
    brd->pastMask.west = ((notOcc & (brd - 2)->pastMask.west & Not<West>()) | royalty) >> 1;
    brd->pastMask.northeast = ((notOcc & (brd - 2)->pastMask.northeast & Not<East>()) | royalty) << 9;
    brd->pastMask.southeast = ((notOcc & (brd - 2)->pastMask.southeast & Not<East>()) | royalty) >> 7;
    brd->pastMask.southwest = ((notOcc & (brd - 2)->pastMask.southwest & Not<West>()) | royalty) >> 9;
    brd->pastMask.northwest = ((notOcc & (brd - 2)->pastMask.northwest & Not<West>()) | royalty) << 7;
  }

  template <U8 Set, U16 T, bool White>
  _Compiletime void createMask(Board<Set> *brd)  
  {
    constexpr U512 notE = {Not<East>(), Not<East>(), Not<East>(), 0, Not<East>(), Not<East>(), Not<East>(), 0};
    constexpr U512 notW = {Not<West>(), Not<West>(), Not<West>(), 0, Not<West>(), Not<West>(), Not<West>(), 0};

    const U64 rook = brd->rooks(!White, true);
    const U64 bishop = brd->bishops(!White, true);
    const U64 unicorn = brd->unicorns(!White, true);
    const U64 dragon = brd->dragons(!White, true);

    const U512 center = U512Set(0, bishop, rook, bishop, rook, bishop, rook, bishop);
    const U512 orth = U512Set(0, unicorn, bishop, unicorn, bishop, unicorn, bishop, unicorn);
    const U512 diag = U512Set(0, dragon, unicorn, dragon, unicorn, dragon, unicorn, dragon);

    U512 o[6];
    U512 r[7];
    for (int i = 0; i < 6; ++i)
    {
      o[i] = ~U512Set(0, (brd - T*(i+1) + (2*i)+3)->board.occ, (brd - T*(i+1) + 1)->board.occ, (brd - T*(i+1) - (2*i+1))->board.occ,
                      0, (brd + T*(i+1) + (2*i)+3)->board.occ, (brd + T*(i+1) + 1)->board.occ, (brd + T*(i+1) - (2*i+1))->board.occ);
    }
    for (int i = 0; i < 7; ++i)
    {
      r[i] = U512Set(0, (brd - T*(i+1) + (2*i)+3)->royalty(White), (brd - T*(i+1) + 1)->royalty(White), (brd - T*(i+1) - (2*i+1))->royalty(White),
                     0, (brd + T*(i+1) + (2*i)+3)->royalty(White), (brd + T*(i+1) + 1)->royalty(White), (brd + T*(i+1) - (2*i+1))->royalty(White));
    }

    U512 maskC = r[6], maskN = r[6], maskE = r[6], maskW = r[6], maskS = r[6], maskNE = r[6], maskSE = r[6], maskSW = r[6], maskNW = r[6];

    for (int i = 5; i >= 0; i--)
    {
      maskC = (o[i] & maskC) | r[i]; // TODO: technically not correct
      maskN = ((o[i] & maskN) | r[i]) << 8;
      maskE = (((o[i] & maskE) | r[i]) & notE) << 1;
      maskS = ((o[i] & maskS) | r[i]) >> 8;
      maskW = (((o[i] & maskW) | r[i])& notW) >> 1;
      maskNE = (((o[i] & maskNE ) | r[i])& notE) << 9;
      maskSE = (((o[i] & maskSE ) | r[i])& notE) >> 7;
      maskSW = (((o[i] & maskSW ) | r[i])& notW) >> 9;
      maskNW = (((o[i] & maskNW ) | r[i])& notW) << 7;
    }

    //Assumes all boards are loaded correctly
    maskC[4] = brd->pastMask.center;
    maskN[4] = brd->pastMask.north;
    maskE[4] = brd->pastMask.east;
    maskS[4] = brd->pastMask.south;
    maskW[4] = brd->pastMask.west;
    maskNE[4] = brd->pastMask.northeast;
    maskSE[4] = brd->pastMask.southeast;
    maskSW[4] = brd->pastMask.southwest;
    maskNW[4] = brd->pastMask.northwest;
    

    const U64 maskSlider = U512_REDUCE_OR((center & maskC) | (orth & maskN) | (orth & maskE) | (orth & maskW) | (orth & maskS) | (diag & maskNE) | (diag & maskSE) | (diag & maskSW) | (diag & maskNW));

    U64 knight = brd->bitBoard(!White, Knight);
    U64 maskKnight = knight & ((brd + T - 3)->royalty(White) | (brd + T + 5)->royalty(White) | (brd + T*2 - 1)->royalty(White) | (brd + T*2 + 3)->royalty(White) | (brd - T - 3)->royalty(White) | (brd - T + 5)->royalty(White) | (brd - T*2 - 1)->royalty(White) | (brd - T*2 + 3)->royalty(White));
    const U64 royaltyKnightD1 = (brd - 1)->royalty(White) | (brd + T + 1)->royalty(White) | (brd - T + 1)->royalty(White);
    const U64 royaltyKnightD2 = (brd - 3)->royalty(White) | (brd + T*2 + 1)->royalty(White) | (brd - T*2 + 1)->royalty(White);
    Bitloop(knight)
    {
      const U8 sq = SquareOf(knight);
      maskKnight |= 1ull << sq & -((Lookup::Knight1Attacks[sq] & royaltyKnightD1 | Lookup::Knight2Attacks[sq] & royaltyKnightD2) != 0);
    }

    U64 king = brd->kings(!White, true);
    const U64 royaltyKing = (brd - 1)->royalty(White) | U512_REDUCE_OR(r[0]);
    U64 maskKing = king & royaltyKing;
    Bitloop(king)
    {
      const U8 sq = SquareOf(king);
      maskKing |= 1ull << sq & -((Lookup::movement<King>(sq, 0) & royaltyKing) != 0);
    }

    constexpr short f1 = White ? -T : T;
    const U64 maskPawn = brd->pawns(!White) & ((brd + f1 - 1)->royalty(White) | (brd + f1 + 3)->royalty(White));

    const U64 royaltyBrawnsLR = (brd + f1 + 1)->royalty(White);
    const U64 royaltyBrawnsF = royaltyBrawnsLR | (brd + f1 - 1)->royalty(White) | (brd + f1 + 3)->royalty(White);
    const U64 maskBrawn = brd->bitBoard(!White, Brawn) & (pawnShift<!White, North>(royaltyBrawnsF) | (royaltyBrawnsLR & Not<East>()) << 1 | (royaltyBrawnsLR & Not<West>()) >> 1);

    brd->pastCheck = maskSlider | maskKnight | maskKing | maskPawn | maskBrawn;
  }

  template <U8 Set, U16 L, U16 T, bool White>
  _Compiletime TMask travelMasks(Board<Set> (&boards)[L + 16][T + 32], U8 timeline, U8 turn)
  {

    TMask tMask;
    for (int i = 0; i < 7; ++i)
    { // i=distance from board
      Board<Set> brds[7] = {
          boards[timeline + (i + 1)][turn + 2 * (i + 1)],
          boards[timeline + (i + 1)][turn],
          boards[timeline + (i + 1)][turn - 2 * (i + 1)],
          boards[timeline][turn - 2 * (i + 1)],
          boards[timeline - (i + 1)][turn - 2 * (i + 1)],
          boards[timeline - (i + 1)][turn],
          boards[timeline - (i + 1)][turn + 2 * (i + 1)]};

      tMask.o[i] = U512Set(
          brds[0].board.occ,
          brds[1].board.occ,
          brds[2].board.occ,
          brds[3].board.occ,
          brds[4].board.occ,
          brds[5].board.occ,
          brds[6].board.occ,
          0);

      const U512 c = U512Set(
          brds[0].checkMask,
          brds[1].checkMask,
          brds[2].checkMask,
          brds[3].checkMask,
          brds[4].checkMask,
          brds[5].checkMask,
          brds[6].checkMask,
          0);

      // TODO: these two can maybe be combined without assigning them and entered into m[i]
      const U512 em = U512Set(
          brds[0].bitBoard(White, NoType),
          brds[1].bitBoard(White, NoType),
          brds[2].bitBoard(White, NoType),
          brds[3].bitBoard(White, NoType),
          brds[4].bitBoard(White, NoType),
          brds[5].bitBoard(White, NoType),
          brds[6].bitBoard(White, NoType),
          0);

      tMask.m[i] = c & ~em;

      tMask.b[i] = U512Set(
          brds[0].banMask,
          brds[1].banMask,
          brds[2].banMask,
          brds[3].banMask,
          brds[4].banMask,
          brds[5].banMask,
          brds[6].banMask,
          0);  
     
      tMask.e[i] = U512Set(
          brds[0].bitBoard(!White, NoType),
          brds[1].bitBoard(!White, NoType),
          brds[2].bitBoard(!White, NoType),
          brds[3].bitBoard(!White, NoType),
          brds[4].bitBoard(!White, NoType),
          brds[5].bitBoard(!White, NoType),
          brds[6].bitBoard(!White, NoType),
          0);
          
    }
    return tMask;
  }

  template <U8 Size, U8 Set, U16 L, U16 T, bool White, bool Royal, Direction Dir>
  _Compiletime void genInfMoves(std::vector<Move> &moves, Board<Set> (&boards)[L + 16][T + 32], Board<Set> board, const TimelineInfo &info, U64 pieces)
  {
    int dist = 1;
    while (pieces)
    {
      U16 eTimeline = info.timeline + dist * dirShift<Dir>().timeline;
      U16 eTurn = info.turn + 2 * dist * dirShift<Dir>().turn;

      Board<Set> curBrd = boards[eTimeline][eTurn];

      U64 move = pieces & ~curBrd.bitBoard(White, NoType) & curBrd.checkMask;
      pieces &= ~curBrd.board.occ;

      if (Royal)
        move ^= board.bitBoard(White, RQueen) & curBrd.banMask;

      U64 cap = move & curBrd.bitBoard(!White, NoType);
      move ^= cap;

      Bitloop(move) moves.emplace_back(SquareOf(move), SquareOf(move), 0, 0, Travel, info.timeline, info.turn, eTimeline, eTurn);
      Bitloop(cap) moves.emplace_back(SquareOf(cap), SquareOf(cap), 0, 0, TravelCapture, info.timeline, info.turn, eTimeline, eTurn);
      dist++;
    }
  }

  template <U8 Size, U8 Set, U16 L, U16 T, bool White, bool Check>
  _Compiletime void genAllMoves(std::vector<Move> &moves, Board<Set> (&boards)[L + 16][T + 32], Board<Set> board, const TimelineInfo &info, const U64 &legalMask, const U64 &pastCheckMask, const TMask &tMask)
  {
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

    const U64 enemyOrEmpty = ~board.bitBoard(White, NoType);
    const U64 enemy = board.bitBoard(!White, NoType);
    const U64 movable = enemyOrEmpty & legalMask & mask;
    const U64 royalMovable = enemyOrEmpty & board.banMask & pastCheckMask & mask;

    // Pawns and brawns
    if (Set > WPawn)
      board.template genPawnMoves<Size, White, Check>(moves, info, legalMask, notPin, notPinHV, notPinD12, tMask);

    // All regular pieces
    if (Set > WKnight)
    {
      pieceMoves<Knight, Check>(moves, info, board.bitBoard(White, Knight) & notPin, movable, board.board.occ, enemy, pin, tMask);
      U512 knightAttack = U512Set1(board.bitBoard(White, Knight) & notPin);

      Board<Set> knightBoards[8] = {
          boards[info.timeline + 1][info.turn + 4],
          boards[info.timeline + 2][info.turn + 2],
          boards[info.timeline + 2][info.turn - 2],
          boards[info.timeline + 1][info.turn - 4],
          boards[info.timeline - 1][info.turn - 4],
          boards[info.timeline - 2][info.turn - 2],
          boards[info.timeline - 2][info.turn + 2],
          boards[info.timeline - 1][info.turn + 4],
      };

      U512 movable = U512Set(knightBoards[0].checkMask, knightBoards[1].checkMask, knightBoards[2].checkMask, knightBoards[3].checkMask,
                             knightBoards[4].checkMask, knightBoards[5].checkMask, knightBoards[6].checkMask, knightBoards[7].checkMask) &
                     ~U512Set(knightBoards[0].bitBoard(White, NoType),knightBoards[1].bitBoard(White, NoType),knightBoards[2].bitBoard(White, NoType),knightBoards[3].bitBoard(White, NoType),
                           knightBoards[4].bitBoard(White, NoType),knightBoards[5].bitBoard(White, NoType),knightBoards[6].bitBoard(White, NoType),knightBoards[7].bitBoard(White, NoType));

      U512 notOcc = ~U512Set(knightBoards[0].board.occ, knightBoards[1].board.occ, knightBoards[2].board.occ, knightBoards[3].board.occ,
                             knightBoards[4].board.occ, knightBoards[5].board.occ, knightBoards[6].board.occ, knightBoards[7].board.occ);

      U512 enemy = U512Set(knightBoards[0].bitBoard(!White, NoType),knightBoards[1].bitBoard(!White, NoType),knightBoards[2].bitBoard(!White, NoType),knightBoards[3].bitBoard(!White, NoType),
                           knightBoards[4].bitBoard(!White, NoType),knightBoards[5].bitBoard(!White, NoType),knightBoards[6].bitBoard(!White, NoType),knightBoards[7].bitBoard(!White, NoType));

      U512 legal = knightAttack & movable;
      U512 move = legal & notOcc;
      U512 cap = legal & enemy;

      jumpMoves<false>(moves, move[0], cap[0], 0, info.timeline, info.turn, LT(1, 2));
      jumpMoves<false>(moves, move[1], cap[1], 0, info.timeline, info.turn, LT(2, 1));
      jumpMoves<false>(moves, move[2], cap[2], 0, info.timeline, info.turn, LT(2, -1));
      jumpMoves<false>(moves, move[3], cap[3], 0, info.timeline, info.turn, LT(1, -2));
      jumpMoves<false>(moves, move[4], cap[4], 0, info.timeline, info.turn, LT(-1, -2));
      jumpMoves<false>(moves, move[5], cap[5], 0, info.timeline, info.turn, LT(-2, -1));
      jumpMoves<false>(moves, move[6], cap[6], 0, info.timeline, info.turn, LT(-2, 1));
      jumpMoves<false>(moves, move[7], cap[7], 0, info.timeline, info.turn, LT(-1, 2));
    }
    // LT only moves
    if (Set > WBishop)
      pieceMoves<Bishop, Check>(moves, info, board.bitBoard(White, Bishop) & notPinHV & info.doublePin, movable, board.board.occ, enemy, pin, tMask);
    if (Set > WRook)
      pieceMoves<Rook, Check>(moves, info, board.bitBoard(White, Rook) & notPinD12 & info.doublePin, movable, board.board.occ, enemy, pin, tMask);
    if (Set > WQueen)
      pieceMoves<Queen, Check>(moves, info, board.bitBoard(White, Queen) & info.doublePin, movable, board.board.occ, enemy, pin, tMask);
    if (Set > WKing)
      pieceMoves<King, Check>(moves, info, board.bitBoard(White, King), royalMovable, board.board.occ, enemy, 0, tMask);
    if (Set > WPrincess)
      pieceMoves<Princess, Check>(moves, info, board.bitBoard(White, Princess) & info.doublePin, movable, board.board.occ, enemy, pin, tMask);
    if (Set > WCKing)
      pieceMoves<CKing, Check>(moves, info, board.bitBoard(White, CKing) & info.doublePin, movable, board.board.occ, enemy, pin, tMask);
    if (Set > WRQueen)
      pieceMoves<RQueen, Check>(moves, info, board.bitBoard(White, RQueen), royalMovable, board.board.occ, enemy, 0, tMask);

    if (!Check) // cant castle or travel when in check (except for royal queens)
    {
      if (Set > WUnicorn)
        pieceMoves<Unicorn, Check>(moves, info, board.bitBoard(White, Unicorn) & notPin, movable, board.board.occ, enemy, pin, tMask);
      if (Set > WDragon)
        pieceMoves<Dragon, Check>(moves, info, board.bitBoard(White, Dragon) & notPin, movable, board.board.occ, enemy, pin, tMask);

      U64 orth = board.rooks(White, true) & notPin;
      U64 diag = board.bishops(White, true) & notPin;

      // TODO: Rqueen may need to be in its own place, its moves also need to be generated when in check
      genInfMoves<Size, Set, L, T, White, (Set > WRQueen), NorthWest>(moves, boards, board, info, diag);
      genInfMoves<Size, Set, L, T, White, (Set > WRQueen), North>(moves, boards, board, info, orth);
      genInfMoves<Size, Set, L, T, White, (Set > WRQueen), NorthEast>(moves, boards, board, info, diag);
      genInfMoves<Size, Set, L, T, White, (Set > WRQueen), East>(moves, boards, board, info, orth);
      genInfMoves<Size, Set, L, T, White, (Set > WRQueen), SouthEast>(moves, boards, board, info, diag);
      genInfMoves<Size, Set, L, T, White, (Set > WRQueen), South>(moves, boards, board, info, orth);
      genInfMoves<Size, Set, L, T, White, (Set > WRQueen), SouthWest>(moves, boards, board, info, diag);

      // Castling
      if (Set > WKing)
      {
        board.template genCastle<Size, White>(moves, info, board.banMask);
      }
    }
  }

  template <U8 Size, U8 Set, U16 L, U16 T, bool White>
  _Compiletime void genRoyalMoves(std::vector<Move> &moves, Board<Set> (&boards)[L + 16][T + 32], Board<Set> board, const TimelineInfo &info, const U64 &pastCheckMask, const TMask &tMask)
  {
    constexpr U64 mask = Size == 1 ? 0x0000000000000001 : Size == 2 ? 0x0000000000000303
                                                      : Size == 3   ? 0x0000000000070707
                                                      : Size == 4   ? 0x000000000f0f0f0f
                                                      : Size == 5   ? 0x0000001f1f1f1f1f
                                                      : Size == 6   ? 0x00003f3f3f3f3f3f
                                                      : Size == 7   ? 0x007f7f7f7f7f7f7f
                                                                    : FULL;

    const U64 royalMovable = ~board.bitBoard(White, NoType) & board.banMask & pastCheckMask & mask;
    const U64 enemy = board.bitBoard(!White, NoType);
    if (Set > WKing)
      pieceMoves<King, false>(moves, info, board.bitBoard(White, King), royalMovable, board.board.occ, enemy, 0, tMask);
    if (Set > WRQueen)
      pieceMoves<RQueen, false>(moves, info, board.bitBoard(White, RQueen), royalMovable, board.board.occ, enemy, 0, tMask);
  }

  template <U8 Set, U8 Size, U16 L, U16 T>
  template <bool White>
  _Compiletime void Chess<Set, Size, L, T>::generateMoves(std::vector<Move> &moves, U16 timeline)
  {
    // Get the board and set up checkMasks, pinMasks, and banMask
    TimelineInfo info = timelineInfo[timeline];
    Board<Set> &brd = boards[timeline][info.turn]; // eventually needs to be done for every timeline or specify timeline in input

    if(brd.pastCheck==FULL) createMask<Set, T, White>(&brd); //Past Checks Generate if not done already

    const U64 pastCheckMask = brd.pastCheck | -(brd.pastCheck == 0);
    const U64 legalMask = brd.checkMask & pastCheckMask;

    if (brd.pastCheck == EMPTY)
    {
      TMask tmask = travelMasks<Set, L, T, White>(boards, timeline, info.turn); // generate travels masks

      if (legalMask == FULL)
      {
        genAllMoves<Size, Set, L, T, White, false>(moves, boards, brd, info, legalMask, pastCheckMask, tmask);
      }
      else if (legalMask)
      {
        genAllMoves<Size, Set, L, T, White, true>(moves, boards, brd, info, legalMask, pastCheckMask, tmask);
      }
      else
      {
        genRoyalMoves<Size, Set, L, T, White>(moves, boards, brd, info, pastCheckMask, tmask);
      }
    }
    else if ((brd.pastCheck & (brd.pastCheck - 1)) == EMPTY)
    {
      if (legalMask)
        brd.template genBitMoves<Size, White>(moves, info, legalMask, pastCheckMask);
      else
        brd.template genRoyalBitMoves<Size, White>(moves, info, pastCheckMask);
    }
  }

  template <U8 Set, U8 Size, U16 L, U16 T>
  template <bool White>
  _Compiletime void Chess<Set, Size, L, T>::makeMove(const Move &move)
  {
    Board<Set> &brd = boards[move.sTimeline][move.sTurn + 1];
    brd.board = boards[move.sTimeline][move.sTurn].board;

    switch (move.type)
    {
    case Normal:
      brd.template makeMove<White, Normal>(move);
      break;
    case Capture:
      brd.template makeMove<White, Capture>(move);
      break;
    case Push:
      brd.template makeMove<White, Push>(move);
      break;
    case Enpassant:
      brd.template makeMove<White, Enpassant>(move);
      break;
    case Promotion:
      brd.template makeMove<White, Promotion>(move);
      break;
    case PromoCapture:
      brd.template makeMove<White, PromoCapture>(move);
      break;
    case Castle:
      brd.template makeMove<White, Castle>(move);
      break;
    }
    // travel cases
    if (move.type >= Travel)
    {
      int newTimeline;
      // increment timelineNum//active/otherstuff
      if (timelineInfo[move.eTimeline].turn == move.eTurn)
      { // Checks if its a jump
        newTimeline = move.eTimeline;
        ++timelineInfo[newTimeline].turn;
      }
      else
      {
        brd.traveled = true;
        timelineNum[White]++;

        newTimeline = origIndex[White] + (White ? -1 : 1) * (timelineNum[White]);

        //Check if the opponent has more or the same amount of timelines before the travel happens.
        if (timelineNum[!White] >= timelineNum[White] - 1) 
        {
          activeNum[White]++;
          if (timelineNum[!White] > timelineNum[White]){ //If after the travel the opponent still has more timelines then that means we activated an inactive
            activeNum[!White]++;
          }
        }

        timelineInfo[newTimeline].tailIndex = move.eTurn + 1;
        timelineInfo[newTimeline].turn = move.eTurn + 1;
      }

      Board<Set> &brdTravel = boards[newTimeline][move.eTurn + 1];
      brdTravel.board = boards[move.eTimeline][move.eTurn].board;

      bool promotion = move.type >= TravelPromotion;

      const Piece piece = promotion ? Piece(move.special1) : brd.board.mailboxBoard[move.from];

      const U64 from = 1ull << move.from;
      brd.board.bitBoard[brd.board.mailboxBoard[move.from]] ^= from;
      brd.template bitBoard<White, NoType>() ^= from;
      brd.board.occ ^= from;
      brd.board.unmoved &= ~from;
      brd.board.epTarget = 0;
      brd.board.mailboxBoard[move.from] = NoPiece;

      if (move.type == TravelCapture || move.type == TravelPromoCapture)
      {
        brdTravel.template makeMoveTravel<White, true>(move, piece);
      }
      else
      {
        brdTravel.template makeMoveTravel<White, false>(move, piece);
      }

      brdTravel.template refresh<!White>(timelineInfo[newTimeline]);
      refreshMask<Set, !White>(&brdTravel);
    }
    ++timelineInfo[move.sTimeline].turn;

    brd.template refresh<!White>(timelineInfo[move.sTimeline]);
    refreshMask<Set, !White>(&brd);

    //Update Present TODO: Currently does calculation multiple times over
    for (int i = origIndex[1] - activeNum[1]; i <= origIndex[0] + activeNum[0]; ++i)
    {
      U8 turn = timelineInfo[i].turn;
      if (turn < present) present = turn;
    }
  }

  template <U8 Set, U8 Size, U16 L, U16 T>
  template <bool White>
  _Compiletime void Chess<Set, Size, L, T>::undoMove(const Move &move)
  { // if board saves the timeline it creates you could pass only a timeline index to it
    Board<Set> &brd = boards[move.sTimeline][move.sTurn + 1];
    brd.board.occ = FULL;
    brd.checkMask = EMPTY;
    if (Set > WKing)
      brd.template bitBoard<White, King>() = EMPTY;
    if (Set > WRQueen)
      brd.template bitBoard<White, RQueen>() = EMPTY;
    --timelineInfo[move.sTimeline].turn;

    if (move.type >= Travel)
    {
      int eTimelineReal;
      if (brd.traveled)
      {
        brd.traveled = false;
        eTimelineReal = origIndex[White] + (White ? -1 : 1) * (timelineNum[White]);

        timelineNum[White]--;
        if (timelineNum[!White] >= timelineNum[White])
        {
          activeNum[White]--;
          if (timelineNum[!White] > timelineNum[White] + 1)
            activeNum[!White]--;
        }

        timelineInfo[eTimelineReal].tailIndex = 0; // not really necessary
        timelineInfo[eTimelineReal].turn = 0;
      }
      else
      {
        eTimelineReal = move.eTimeline;
        --timelineInfo[eTimelineReal].turn;
      }

      Board<Set> &brdTo = boards[eTimelineReal][move.eTurn + 1];
      brdTo.board.occ = FULL;
      brdTo.checkMask = EMPTY;
      if (Set > WKing)
        brdTo.template bitBoard<White, King>() = EMPTY;
      if (Set > WRQueen)
        brdTo.template bitBoard<White, RQueen>() = EMPTY;

      // TODO: cleanup
      brdTo.pastMask.center = EMPTY;
      brdTo.pastMask.north = EMPTY;
      brdTo.pastMask.east = EMPTY;
      brdTo.pastMask.south = EMPTY;
      brdTo.pastMask.west = EMPTY;
      brdTo.pastMask.northeast = EMPTY;
      brdTo.pastMask.southeast = EMPTY;
      brdTo.pastMask.southwest = EMPTY;
      brdTo.pastMask.northwest = EMPTY;
      brdTo.pastCheck=FULL;
    }

    brd.pastMask.center = EMPTY;
    brd.pastMask.north = EMPTY;
    brd.pastMask.east = EMPTY;
    brd.pastMask.south = EMPTY;
    brd.pastMask.west = EMPTY;
    brd.pastMask.northeast = EMPTY;
    brd.pastMask.southeast = EMPTY;
    brd.pastMask.southwest = EMPTY;
    brd.pastMask.northwest = EMPTY;
    brd.pastCheck=FULL;

    //Update Present TODO: Currently does calculation multiple times over
    for (int i = origIndex[1] - activeNum[1]; i <= origIndex[0] + activeNum[0]; ++i)
    {
      U8 turn = timelineInfo[i].turn;
      if (turn < present) present = turn;
    }
  }  

  template <U8 Set, U8 Size, U16 L, U16 T>
  template <bool isWhite>
  _Compiletime Move Chess<Set, Size, L, T>::PGNtoMove(std::smatch matches)
  {

    Move move;
    int score = 0;
    if (matches[1].matched)
    {
      move.sTimeline = origIndex[1] - std::stoi(matches[1].str()); // two timelines
      move.sTurn = 2 * std::stoi(matches[2].str()) + (isWhite ? 16 : 17);
    }
    else
    {
      move.sTimeline = origIndex[1];
      move.sTurn = timelineInfo[move.sTimeline].turn;
    }

    if (matches[8].matched)
    {
      move.type = Travel;

      move.eTimeline = origIndex[1] - std::stoi(matches[10].str()); // two timelines
      move.eTurn = 2 * std::stoi(matches[11].str()) + (isWhite ? 16 : 17);
      move.from = (Size * (matches[7].str()[1] - '1')) + (matches[7].str()[0] - 'a');
      move.to = (Size * (matches[12].str()[1] - '1')) + (matches[12].str()[0] - 'a');
      U64 lastRank = 0xffull | 0xffull << (8 * Size - 8); // could be issues with having both last ranks
      if ((!matches[3].matched || matches[3].str() == "P" || matches[3].str() == "W") && (lastRank & (1ULL << move.to)))
      {
        move.special1 = charToPiece(matches[13].str()[0] + (isWhite ? 0 : 32));
        // charToPiece<isWhite>(matches[13].str()[0]);
        move.type = (boards[move.eTimeline][move.eTurn].board.mailboxBoard[move.to] != NoPiece) ? TravelPromoCapture : TravelPromotion;
      }
      else
      {
        move.type = (boards[move.eTimeline][move.eTurn].board.mailboxBoard[move.to] != NoPiece) ? TravelCapture : Travel;
      }
    }
    else
    {
      move.type = Normal;
      move.eTimeline = move.sTimeline;
      move.eTurn = move.sTurn;
      move.from = (Size * (matches[6].str()[0] - '1')) + (matches[5].str()[0] - 'a');
      move.to = (Size * (matches[7].str()[1] - '1')) + (matches[7].str()[0] - 'a');
      // Castle Check
      if ((matches[3].str() == "K" && abs(move.to - move.from) == 2))
      {
        int i = move.to;
        while (boards[move.eTimeline][move.eTurn].board.mailboxBoard[i] != toPiece(isWhite, Rook) && i < Size * ((move.to / Size) + 1) - 1 && i > Size * (move.to / Size))
        {
          if (move.to - move.from > 0)
          {
            i++;
          }
          else
          {
            i--;
          }
        }
        move.special1 = i;
        move.special2 = move.from + ((int)move.to - (int)move.from) / 2;
        move.type = Castle;
      }
      else if (!matches[3].matched || matches[3].str() == "P" || matches[3].str() == "W")
      {                                                     // En Passant/Pawn Push/
        U64 lastRank = 0xffull | 0xffull << (8 * Size - 8); // could be issues with having both last ranks
        if (pawnShift<isWhite, North>(boards[move.eTimeline][move.eTurn].board.epTarget) & (1ULL << move.to))
        { // directions could be off need to test
          move.special1 = pawnSquare<!isWhite, North>(move.to);
          move.type = Enpassant;
        }
        else if (lastRank & (1ULL << move.to))
        {
          move.special1 = charToPiece(matches[13].str()[0] + (isWhite ? 0 : 32));
          // charToPiece<isWhite>(matches[13].str()[0]);
          move.type = (boards[move.eTimeline][move.eTurn].board.mailboxBoard[move.to] != NoPiece) ? PromoCapture : Promotion;
        }
        else
        {
          if (boards[move.eTimeline][move.eTurn].board.mailboxBoard[move.to] != NoPiece)
          {
            move.type = Capture;
          }
          else
          {
            move.type = (abs(move.to - move.from) == 16) ? Push : Normal;
          }
        }
      }
      else if (boards[move.eTimeline][move.eTurn].board.mailboxBoard[move.to] != NoPiece)
      {
        move.type = Capture;
      }
      else
      {
        move.type = Normal;
      }
    }
    return move;
  }

  template <U8 Set, U8 Size, U16 L, U16 T>
  _Compiletime void Chess<Set, Size, L, T>::importPGN(std::string PGN)
  {
    const std::regex turnPat("(?:\\d+\\.\\s*)(?:\\{[^\\}]*\\}\\s*)?([^\\/]*[^\\/\\s])(?:\\s*[\\/]\\s*)*(?:\\{[^\\}]*\\}\\s*)?(.*[^\\s]|)");
    const std::regex movePat("(?:\\((-?\\d+)T(\\d+)\\))?([KQRBNPUDSWCY])?(([a-h])?([1-8])?)?x?([a-h][1-8])(([>][>]|[>])\\((-?\\d+)T(\\d+)\\)x?([a-h][1-8]))?(?:=([KQRBNPUDSWCY]))?");
    for (auto i = std::sregex_iterator(PGN.begin(), PGN.end(), turnPat); i != std::sregex_iterator(); ++i)
    {
      const std::string &white = (*i)[1].str();
      for (auto j = std::sregex_iterator(white.begin(), white.end(), movePat); j != std::sregex_iterator(); ++j)
      {
        makeMove<true>(PGNtoMove<true>(*j));
      }

      const std::string black = (*i)[2].str();
      for (auto j = std::sregex_iterator(black.begin(), black.end(), movePat); j != std::sregex_iterator(); ++j)
      {
        makeMove<false>(PGNtoMove<false>(*j));
      }
    }
  }

  // Fen must have boards in order from least recent to most recent along any given timeline.
  template <U8 Set, U8 Size, U16 L, U16 T>
  _Compiletime void Chess<Set, Size, L, T>::importFen(std::string fen) //technically should have ability for even timelines
  {
    std::regex boardPat("\\[([^\\/]+)\\/?([^\\/]+)?\\/?([^\\/]+)?\\/?([^\\/]+)?\\/?([^\\/]+)?\\/?([^\\/]+)?\\/?([^\\/]+)\\/?([^\\/]+)?:(-?\\d+):(-?\\d+):(w|b)\\]"); //should technically match a + sign
    std::regex rowPat("([pPnNbBrRqQkKsScCyYwWuUdD]|[1-8])\\s*(\\*)?");
    for (auto i = std::sregex_iterator(fen.begin(), fen.end(), boardPat); i != std::sregex_iterator(); ++i)
    {
      const std::smatch &board = *i;
      const bool white = board[11].str()[0] == 'w';
      const U8 brdL = -stoi(board[9].str()) + origIndex[1];
      const U8 brdT = 2 * stoi(board[10].str()) + (white ? 16 : 17);

      if (brdL < origIndex[1] - timelineNum[1])
        timelineNum[1] = origIndex[1] - brdL;
      if (brdL > origIndex[0] + timelineNum[0])
        timelineNum[0] = brdL - origIndex[0];

      Board<Set> &brd = boards[brdL][brdT];
      for (U8 i = 0; i < Size; ++i)
      {
        const std::string &row = board[i + 1].str();
        U8 sq = 56 - 8 * i;
        for (auto j = std::sregex_iterator(row.begin(), row.end(), rowPat); j != std::sregex_iterator(); ++j)
        {
          const char ch = j->str()[0];
          if ('1' <= ch && ch <= '8')
          {
            sq += ch - 48;
          }
          else
          {
            const Piece piece = charToPiece(ch);
            brd.board.mailboxBoard[sq] = piece;
            brd.board.unmoved |= U64(j->length() == 2) << sq;
            brd.board.bitBoard[piece] |= 1ull << sq++;
          }
        }
      }

      for (U8 p = WPawn; p < Set; p += 2)
        brd.board.white |= brd.board.bitBoard[p];
      for (U8 p = BPawn; p < Set; p += 2)
        brd.board.black |= brd.board.bitBoard[p];
      brd.board.occ = brd.board.white | brd.board.black;

      TimelineInfo &info = timelineInfo[brdL];
      info.turn = info.turn == 0 ? brdT : std::min(info.turn, brdT);
      info.tailIndex = std::max(info.tailIndex, brdT);

      if (white)
      {
        brd.template refresh<true>(info); // refresh
        refreshMask<Set, true>(&brd);
      }
      else
      {
        brd.template refresh<false>(info);
        refreshMask<Set, false>(&brd);
      }
    }
    activeNum[1] = std::min((int)timelineNum[1], timelineNum[0] + 1);
    activeNum[0] = std::min(timelineNum[1]+1, (int)timelineNum[0]);
    
    for (int i = origIndex[1] - activeNum[1]; i <= origIndex[0] + activeNum[0]; ++i) {
      U8 turn = timelineInfo[i].turn;
      if (turn < present) present = turn;
    }
  }

  template <U8 Set, U8 Size, U16 L, U16 T>
  _Compiletime void Chess<Set, Size, L, T>::printToFile(std::ofstream &file)
  {
    const U8 top = origIndex[0] + timelineNum[0];
    const U8 bot = origIndex[1] - timelineNum[1];

    U8 min = timelineInfo[bot].tailIndex;
    for (U8 i = bot; i <= top; ++i)
      min = std::min(min, timelineInfo[i].tailIndex);

    for (U8 i = top; i >= bot; --i)
    {
      const U8 tail = timelineInfo[i].tailIndex;
      const U8 head = timelineInfo[i].turn;

      for (U8 j = min; j < tail; ++j)
        file << "          ";
      for (U8 j = tail; j <= head; ++j)
      {
        std::string label = std::to_string(origIndex[1] - i) + "," + std::to_string((j - 16) / 2);
        file << "╔" + label + std::string("════════", 24 - 3 * label.length()) + "╗";
      }
      file << "\n";

      for (char j = 7; j >= 0; --j)
      {
        for (U8 k = min; k < tail; ++k)
          file << "          ";
        for (U8 k = tail; k <= head; ++k)
        {
          file << "║";
          for (U8 l = 0; l < 8; ++l)
            file << pieceToChar(boards[i][k].board.mailboxBoard[j * 8 + l]);
          file << "║";
        }
        file << "\n";
      }

      for (U8 j = min; j < tail; ++j)
        file << "          ";
      for (U8 j = tail; j <= head; ++j)
        file << "╚════════╝";
      file << "\n";
    }
  }
};