#include <chrono>
#include <algorithm>
#include <memory>
#include "chess.hpp"
#include <string>
#include <unordered_map>
#include <cmath>
#include <stdexcept>
#include "tt.hpp"

static U64 count = 0;
static U64 mates = 0;
static U64 collision = 0;
static U64 hitCount = 0;
static U64 maxDepth = 0;

static constexpr int CHECKMATE = 10000000;

template <typename T, size_t Size>
class CircularArray
{
private:
  size_t currentIndex;

public:
  T array[Size];

  CircularArray() : currentIndex(0) {}

  void addElement(const T &element)
  {
    array[currentIndex] = element;
    currentIndex = (currentIndex + 1) % Size;
  }

  void empty()
  {
    currentIndex = 0;
    std::fill(std::begin(array), std::end(array), T());
  }
};

namespace Chess5D
{
  static constexpr int MAX_PLY = 30;
  static constexpr int PLY = 20;
  static constexpr int NUM_PIECES = 12;
  static constexpr int BOARD_SIZE = 8;

  int typeToVal[NUM_PIECES] = {1, 5, 5, 3, 15, 0, 9, 4, 0, 3, 2, .25};

  // Move lbrTable[NUM_PIECES * 2 + 1][BOARD_SIZE][BOARD_SIZE]; // should really not be +1 but travels exist

  CircularArray<Move, 8> killerTable;

  TranspositionTable tt(1000000);

  // Move mateKiller[PLY][10];

  enum NodeType : U8
  {
    NodeSpatial,
    NodeTravel
  };

  struct Result
  {
    int value;
    std::vector<Chess5D::Move> moveset;
    std::unique_ptr<Result> next;

    Result(int v, std::vector<Chess5D::Move> m, std::unique_ptr<Result> n) : value(v), moveset(m), next(std::move(n)) {}

    Result() : value(0), moveset(), next(nullptr) {}
  };

  template <U8 Set, U8 Size, U16 L, U16 T, bool White>
  _Compiletime int evaluate(Chess<Set, Size, L, T> &chess, int timeline, int depth)
  {
    struct Weight
    {
      int softmate = -10000;
      double pieceVal = 5;
      double moveVal = 1;
      double kingExp = 1;
      int tlValue = -2000;
      int unmoved = 5;
    } w;

    ++count;
    std::vector<Chess5D::Move> moves;
    moves.reserve(100);
    chess.template generateMoves<White>(moves, timeline);
    if (moves.size() == 0)
    {
      ++mates;
      return -CHECKMATE + depth;
    }

    TimelineInfo info = chess.timelineInfo[timeline];
    Board<Set> brd = chess.boards[timeline][info.turn];

    double eval = 0;

    // Move Count
     eval += w.moveVal * (moves.size());

    // Softmate
    if (std::all_of(moves.begin(), moves.end(), [](const Move &move)
                    { return move.type == Travel; }))
    {
      eval += w.softmate;
    }

    // Piece Count
    for (int i = 0; i < 12; ++i)
    {

      U64 pieceW = brd.bitBoard(White, static_cast<PieceType>(i));
      U64 pieceB = brd.bitBoard(!White, static_cast<PieceType>(i));
      eval += w.pieceVal * typeToVal[i] * (_popcnt64(pieceW) - _popcnt64(pieceB));

      // Castle Ability Bonus
      if (i == 5 || i == 3)
      {
        eval += w.unmoved * (_popcnt64(pieceW & brd.board.unmoved) - _popcnt64(pieceB & brd.board.unmoved));
      }
    }

    bool enemyHasQueen = (brd.bitBoard(!White, Queen) | brd.bitBoard(!White, RQueen) | brd.bitBoard(!White, Unicorn) | brd.bitBoard(!White, Dragon)) > 0;
    bool enemyHasBishop = enemyHasQueen || ((brd.bitBoard(!White, Bishop) | brd.bitBoard(!White, Princess)) > 0);

    bool meHasQueen = (brd.bitBoard(White, Queen) | brd.bitBoard(White, RQueen) | brd.bitBoard(White, Unicorn) | brd.bitBoard(White, Dragon)) > 0;
    bool meHasBishop = meHasQueen || ((brd.bitBoard(White, Bishop) | brd.bitBoard(White, Princess)) > 0);

    // Timeline Count
    eval += w.tlValue * ((chess.timelineNum[White]) - (chess.timelineNum[!White]));

    Board<Set> brd2 = chess.boards[timeline][info.turn - 1];

    U64 combinedMask = brd.pastMask.center |
                               (enemyHasBishop)
                           ? (brd.pastMask.north | brd.pastMask.south |
                                      (enemyHasQueen)
                                  ? (brd.pastMask.northeast | brd.pastMask.southeast | brd.pastMask.southwest | brd.pastMask.northwest)
                                  : 0)
                           : 0;

    U64 combinedMask2 = brd2.pastMask.center |
                                (meHasBishop)
                            ? (brd2.pastMask.north | brd2.pastMask.south |
                                       (meHasQueen)
                                   ? (brd2.pastMask.northeast | brd2.pastMask.southeast | brd2.pastMask.southwest | brd2.pastMask.northwest)
                                   : 0)
                            : 0;

    // KingExposure
    eval += w.kingExp * (std::max(0, (int)_popcnt64(combinedMask2) - 4) - std::max(0, (int)_popcnt64(combinedMask) - 4)); // not really working as intended

    return eval;
  };

  template <U8 Set, U8 Size, U16 L, U16 T, bool White>
  _Compiletime void moveScore(Chess<Set, Size, L, T> &chess, int depth, int timeline, Move &move, Move &lastMove)
  {
    // should score with MVV-LVA, piece square table difference, captures/promotions, check caused
    TimelineInfo info = chess.timelineInfo[timeline];
    Board<Set> &brd = chess.boards[timeline][info.turn];

    Piece pieceFrom = brd.board.mailboxBoard[move.from];
    Piece pieceTo = brd.board.mailboxBoard[move.to];

    bool isTravel = move.type >= Travel;
    if (isTravel)
    {
      if (chess.timelineNum[White] > chess.timelineNum[!White])
      {
        move.score -= 1000;
        move.E -= 1;
      }
      if (move.type == Travel)
      {
        move.score -= 200;
        move.E -= 1;
      }
      move.score -= 200; // travel penalty

      Board<Set> &brdTravel = chess.boards[move.eTimeline][move.eTurn];
      pieceTo = brdTravel.board.mailboxBoard[move.to];
    }
    else
    {
      /*
      if (lastMove.type != NullMove) { // LBR
        Piece lastPieceFrom = brd.board.mailboxBoard[lastMove.to];
        if (lbrTable[lastPieceFrom][lastMove.from][lastMove.to] == move) {
          move.score += 20;
        };
      }
      */

      // minimal benefits when set to 0 eval???
      for (Move &killMove : killerTable.array)
      { // Killer
        if (killMove == move)
        {
          move.score += 1000;
          break;
        }
      }
    }

    if (move.type == Capture || move.type == PromoCapture || move.type == TravelCapture || move.type == TravelPromoCapture)
    {
      move.score += typeToVal[pieceTo] * 2 - typeToVal[pieceFrom] + 10; // MVV-LVA //TODO: Improve
    }
    else
    {
      move.score -= typeToVal[pieceFrom]; // probably needs fixing but works well ¯\_(ツ)_/¯
    }

    chess.template makeMove<White>(move);
    Board<Set> &newBrd = chess.boards[timeline][info.turn + 1];
    if (newBrd.checkMask != FULL)
    {
      move.score += 100; // Check incentive
      move.E += 1;
    }
    if (isTravel)
    {
      int newTimeline = chess.origIndex[White] + (White ? -1 : 1) * (chess.timelineNum[White]);
      Board<Set> &newBrdTravel = chess.boards[newTimeline][move.eTurn + 1];
      if (newBrdTravel.checkMask != FULL)
      {
        move.score += 400; // Travel Check incentive
        move.E += 2;
      }
    }

    chess.template undoMove<White>(move);
  };

  void generateCombinations(const std::vector<std::vector<Move>> &moveList, int depth, std::vector<Move> &currentCombination, int x, std::vector<std::vector<Move>> &resList)
  {
    if (depth == moveList.size())
    {
      // Add the current combination to allCombinations
      resList.push_back(currentCombination);
      return;
    }

    for (int i = 0; i < x && i < moveList[depth].size(); ++i)
    {
      currentCombination.push_back(moveList[depth][i]);
      generateCombinations(moveList, depth + 1, currentCombination, x, resList);
      currentCombination.pop_back();
    }
  }

  std::vector<std::vector<Move>> combinations(const std::vector<std::vector<Move>> &moveList, int x)
  {
    std::vector<std::vector<Move>> resList;
    std::vector<Move> currentCombination;
    generateCombinations(moveList, 0, currentCombination, x, resList);
    return resList;
  }

  template <U8 Set, U8 Size, U16 L, U16 T, bool White>
  _Compiletime Result negaMaxIDDFS(Chess<Set, Size, L, T> &chess, int maxDepth, std::chrono::milliseconds timeLimit)
  {
    auto startTime = std::chrono::high_resolution_clock::now();
    Result bestRes;

    int alpha = -CHECKMATE;
    int beta = CHECKMATE;
    // Perform iterative deepening within the time limit
    for (int depth = 1; depth <= maxDepth; ++depth)
    {
      auto elapsedTime = duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime);

      if (elapsedTime >= timeLimit)
      {
        std::cout << depth << std::endl;
        break;
      }

      bestRes = negaMax<Set, Size, L, T, White, true>(chess, alpha, beta, depth, 0, std::vector<Chess5D::Move>());

      if (bestRes.value >= CHECKMATE - MAX_PLY)
      {
        //  alpha=bestRes.value-1;
      }

      if (bestRes.value <= -CHECKMATE + MAX_PLY)
      {
        //  beta=bestRes.value+1;
      }
    }

    return bestRes;
  }

  template <U8 Set, U8 Size, U16 L, U16 T, bool White, bool PV>
  _Compiletime Result negaMax(Chess<Set, Size, L, T> &chess, int alpha, int beta, int depth, int ply, std::vector<Chess5D::Move> lastMoves)
  {
    if (chess.timelineNum[0] + chess.timelineNum[1] == 0 && chess.origIndex[0] == chess.origIndex[1])
    {
      Move tempNullMove = Move(0, 0, 0, 0, NullMove, 0, 0, 0, 0);
      int val = negaMax1<Set, Size, L, T, White, true, NodeSpatial>(chess, alpha, beta, depth, 0, chess.origIndex[0], tempNullMove);
      Result res(val, std::vector<Chess5D::Move>{}, nullptr);
      return res;
    }

    if (depth <= 0 || ply > MAX_PLY)
    {
      Result res(0, std::vector<Chess5D::Move>{}, nullptr);
      for (int timeline = chess.origIndex[1] - chess.activeNum[1]; timeline <= chess.origIndex[0] + chess.activeNum[0]; ++timeline)
      {
        res.value += quiesce<Set, Size, L, T, White, PV>(chess, alpha, beta, 10, ply, timeline, lastMoves[0]);
      }
      return res;
    }

    std::vector<std::vector<Chess5D::Move>> movesAll;
    int cnt = 0;
    for (int i = chess.origIndex[1] - chess.activeNum[1]; (i <= chess.origIndex[0] + chess.activeNum[0])&&(chess.timelineInfo[i].turn != chess.present); ++i)
    {
      std::vector<Chess5D::Move> moves;
      chess.template generateMoves<White>(moves, i);

      // Score
      for (Move &move : moves)
      {
        Move tempNullMove = Move(0, 0, 0, 0, NullMove, 0, 0, 0, 0);
        moveScore<Set, Size, L, T, White>(chess, depth, i, move, tempNullMove);
      }

      // Sort
      std::sort(moves.begin(), moves.end(), [](const Move &a, const Move &b)
                { return a.score > b.score; });

      // Search
      for (Move &move : moves)
      { // could maybe add alpha beta cutoffs at this depth
        chess.template makeMove<White>(move);
        int res = negaMax1<Set, Size, L, T, !White, false, NodeSpatial>(chess, -beta, -alpha, depth - 1, ply+1, i, move);
        chess.template undoMove<White>(move);

        move.score = -res;
      }

      // Resort
      std::sort(moves.begin(), moves.end(), [](const Move &a, const Move &b)
                { return a.score > b.score; });

      movesAll.push_back(moves);
      cnt++;
    }

    // naively attempt to play all the best moves of every timeline and only search a handful of those
    Result bestRes(-CHECKMATE, std::vector<Chess5D::Move>(), nullptr);
    std::vector<std::vector<Move>> moveList = combinations(movesAll, 3);
    for (int i = 0; i < moveList.size(); ++i)
    {
      std::vector<Move> moves = moveList[i];

      for (Move &move : moves)
      {
        chess.template makeMove<White>(move);
      }
      Result res = negaMax<Set, Size, L, T, !White, PV>(chess, -beta, -alpha, depth - 1, ply+1, moves);
      for (Move &move : moves)
      {
        chess.template undoMove<White>(move);
      }

      if (-res.value > bestRes.value)
      {
        bestRes.value = -res.value;
        bestRes.moveset = moves;
        bestRes.next = std::make_unique<Result>(res.value, res.moveset, std::move(res.next));
      }
      if (bestRes.value > alpha)
        alpha = bestRes.value;

      if (alpha >= beta)
      {
        break;
      }
    }

    return bestRes;
  };

  //Should result be int or double?
  template <U8 Set, U8 Size, U16 L, U16 T, bool White, bool PV, NodeType Node>
  _Compiletime int negaMax1(Chess<Set, Size, L, T> &chess, int alpha, int beta, int depth, int ply, int timeline, Move lastMove)
  {
    int alphaOrig = alpha;
    TimelineInfo &info = chess.timelineInfo[timeline];
    Board<Set> &brd = chess.boards[timeline][info.turn];

    if (PV && ply == 0)
      brd.template refresh<White>(info); // TODO: Add to actual function

    // Transposition Table Lookup
    U64 key = tt.computeHashKey<Set, White>(brd);
    TTEntry &ttEntry = tt.probe(key);
    if (ttEntry.key == key && ttEntry.depth >= depth && !ttEntry.isQSearch)
    {
      if (ttEntry.isWhite != White)
      {
        collision++;
      }
      hitCount++;
      if (ttEntry.flag == TTEntry::EXACT)
      {
        return ttEntry.value;
      }
      else if (ttEntry.flag == TTEntry::LOWERBOUND)
      {
        alpha = std::max(alpha, ttEntry.value);
      }
      else if (ttEntry.flag == TTEntry::UPPERBOUND)
      {
        beta = std::min(beta, ttEntry.value);
      }

      if (alpha >= beta)
      {
        return ttEntry.value;
      }
    }

    // Mate Distance Pruning
    int mating_value = CHECKMATE - ply;
    if (mating_value < beta)
    {
      beta = mating_value;
      if (alpha >= mating_value)
      {
        return mating_value;
      }
    }

    mating_value = -CHECKMATE + ply;
    if (mating_value > alpha)
    {
      alpha = mating_value;
      if (beta <= mating_value)
      {
        return mating_value;
      }
    }

    // Quiescence Search
    if (depth <= 0 || ply > MAX_PLY)
    {
      return quiesce<Set, Size, L, T, White, PV>(chess, alpha, beta, -1, ply, timeline, lastMove);
    }

    std::vector<Move> moves;
    moves.reserve(100);
    chess.template generateMoves<White>(moves, timeline);

    int E = 0; // Extension Value
    if (moves.size() <= 1)
    {
      if (moves.size() == 0)
      {
        return -CHECKMATE + ply;
      }
      E += 1; // Only Reply
    }

    bool inCheck = brd.pastCheck != EMPTY && brd.checkMask == FULL;

    bool PVMove = false;
    // Move Ordering
    for (auto it = moves.begin(); it != moves.end(); ++it)
    {
      if (PV && *it == ttEntry.move)
      {
        PVMove = true;
        std::rotate(moves.begin(), it, moves.end());
        continue;
      }
      moveScore<Set, Size, L, T, White>(chess, depth, timeline, *it, lastMove);
    }

    std::sort(moves.begin() + (int)PVMove, moves.end(), [](const Move &a, const Move &b)
              { return a.score > b.score; });

    int bestRes = -CHECKMATE+ply;
    Move bestMove=moves[0];
    int res;
    
    for (int i = 0; i < moves.size(); ++i)
    {
      Move move = moves[i];

      if (Node == NodeTravel && move.type >= Travel)
        continue;

      if (depth >= 3 && !inCheck)
        move.E -= int(log(i + 2));

      chess.template makeMove<White>(move);
      if (move.type >= Travel)
      {
        U8 newTimeline = chess.origIndex[White] + (White ? -1 : 1) * (chess.timelineNum[White]);
        int newDepth = std::min(depth, move.sTurn - move.eTurn) - 1 + E + move.E;
        if (PV && i == 0)
        {
          res = negaMax1<Set, Size, L, T, !White, true, NodeTravel>(chess, -beta, -alpha, newDepth, ply + 1, newTimeline, move);
        }
        else
        {
          res = negaMax1<Set, Size, L, T, !White, false, NodeTravel>(chess, -beta, -alpha, newDepth, ply + 1, newTimeline, move);
        }
      }
      else
      {
        int newDepth = depth - 1 + E + move.E;
        if (PV && i == 0)
        {
          res = negaMax1<Set, Size, L, T, !White, true, Node>(chess, -beta, -alpha, newDepth, ply + 1, timeline, move);
        }
        else
        {
          res = negaMax1<Set, Size, L, T, !White, false, Node>(chess, -beta, -alpha, newDepth, ply + 1, timeline, move);
        }
      }
      chess.template undoMove<White>(move);

      if (-res > bestRes)
      {
        bestRes = -res;
        bestMove = move;
      }
      if (bestRes > alpha)
        alpha = bestRes;

      if (alpha >= beta)
      {
        killerTable.addElement(move);
        break;
      }
    }

    // Transposition Table Store
    ttEntry.move = bestMove;
    ttEntry.value = bestRes;
    ttEntry.key = key;
    ttEntry.isWhite = White;
    ttEntry.isQSearch = false;
    if (bestRes <= alphaOrig)
    {
      ttEntry.flag = TTEntry::UPPERBOUND;
    }
    else if (bestRes >= beta)
    {
      ttEntry.flag = TTEntry::LOWERBOUND;
    }
    else
    {
      ttEntry.flag = TTEntry::EXACT;
    }
    ttEntry.depth = depth;
    tt.store(ttEntry);

    return bestRes;
  };

  template <U8 Set, U8 Size, U16 L, U16 T, bool White, bool PV>
  _Compiletime int quiesce(Chess<Set, Size, L, T> &chess, int alpha, int beta, int depth, int ply, int timeline, Move lastMove)
  {
    int alphaOrig = alpha;
    TimelineInfo info = chess.timelineInfo[timeline];
    Board<Set> &brd = chess.boards[timeline][info.turn];

    // Transposition Table Lookup
    U64 key = tt.computeHashKey<Set, White>(brd);
    TTEntry &ttEntry = tt.probe(key);
    if (ttEntry.key == key && ttEntry.depth >= depth)
    {
      if (ttEntry.isWhite != White)
      {
        collision++;
      }
      hitCount++;
      if (ttEntry.flag == TTEntry::EXACT)
      {
        return ttEntry.value;
      }
      else if (ttEntry.flag == TTEntry::LOWERBOUND)
      {
        alpha = std::max(alpha, ttEntry.value);
      }
      else if (ttEntry.flag == TTEntry::UPPERBOUND)
      {
        beta = std::min(beta, ttEntry.value);
      }

      if (alpha >= beta)
      {
        return ttEntry.value;
      }
    }

    // Mate Distance Pruning
    int mating_value = CHECKMATE - ply;
    if (mating_value < beta)
    {
      beta = mating_value;
      if (alpha >= mating_value)
        return mating_value;
    }

    mating_value = -CHECKMATE + ply;
    if (mating_value > alpha)
    {
      alpha = mating_value;
      if (beta <= mating_value)
        return mating_value;
    }

    int val = evaluate<Set, Size, L, T, White>(chess, timeline, ply);
    if (depth <= 0 || ply > MAX_PLY)
    {
      return val;
    }
    else if (val >= beta)
    {
      return beta;
    }
    else if (val > alpha)
    {
      alpha = val;
    }

    std::vector<Move> moves;
    moves.reserve(100);
    chess.template generateMoves<White>(moves, timeline); // should generate only captures/checks efficiently

    // Remove Travels completely
    moves.erase(std::remove_if(moves.begin(), moves.end(), [&](Move &move) { return move.type >= Travel; }),moves.end());

    int E = 0; // Extension Value
    if (moves.size() < 2)
    {
      if (moves.size() == 0)
      {
        return -CHECKMATE + ply;
      }
      E += 1; // Only Reply
    }

    std::vector<Move> quietMoves;
    quietMoves.reserve(50);

    // Remove Quiet Moves and only search for mate check
    if (brd.checkMask == FULL && brd.pastCheck == EMPTY)
    {
      std::partition_copy(moves.begin(), moves.end(), std::back_inserter(quietMoves), std::back_inserter(moves), [&](Move &move){
            if (move.type == Capture || move.type == PromoCapture) {
                return true;
            }

            chess.template makeMove<White>(move);
            Board<Set>& newBrd = chess.boards[timeline][info.turn + 1];
            createMask<Set, T, !White>(&newBrd);

            bool checkCaused = (newBrd.pastCheck != EMPTY || newBrd.checkMask != FULL);
            chess.template undoMove<White>(move);

            return checkCaused;});
    }

    bool inCheck = brd.pastCheck != EMPTY && brd.checkMask == FULL;

    bool PVMove = false;
    // Move Ordering
    for (auto it = moves.begin(); it != moves.end(); ++it)
    {
      if (PV && *it == ttEntry.move)
      {
        PVMove = true;
        std::rotate(moves.begin(), it, moves.end());
        continue;
      }
      moveScore<Set, Size, L, T, White>(chess, depth, timeline, *it, lastMove);
    }

    std::sort(moves.begin() + (int)PVMove, moves.end(), [](const Move &a, const Move &b)
              { return a.score > b.score; });
    
    int bestRes = -CHECKMATE + ply;
    int res;
    Move bestMove;
    if(moves.size()!=0) bestMove=moves[0];
    for (int i = 0; i < moves.size(); ++i)
    {
      Move move = moves[i];

      if (depth >= 3 && !inCheck)
        move.E -= int(log(i + 2)); // LMR

      chess.template makeMove<White>(move);
      if (PV && i == 0)
      {
        res = quiesce<Set, Size, L, T, !White, true>(chess, -beta, -alpha, depth - 1 + E + move.E, ply + 1, timeline, move);
      }
      else{
        res = quiesce<Set, Size, L, T, !White, false>(chess, -beta, -alpha, depth - 1 + E + move.E, ply + 1, timeline, move);
      }
      chess.template undoMove<White>(move);

      if (-res > bestRes)
      {
        bestRes = -res;
        bestMove = move;
      }
      if (bestRes > alpha)
        alpha = bestRes;

      if (alpha >= beta)
      {
        killerTable.addElement(move);
        break;
      }
    }
    if (bestRes <= -CHECKMATE + MAX_PLY && quietMoves.size()!=0)
    {
      for (Move &move : quietMoves) moveScore<Set, Size, L, T, White>(chess, depth, timeline, move, lastMove);
      std::sort(quietMoves.begin(), quietMoves.end(), [](const Move &a, const Move &b){return a.score > b.score;});

      for (int i = 0; i < quietMoves.size(); ++i)
      {
        Move move = quietMoves[i];

        if (depth >= 3 && !inCheck)
          move.E -= int(log(i + 2)); // LMR

        chess.template makeMove<White>(move);
        res = quiesce<Set, Size, L, T, !White, false>(chess, -beta, -alpha, depth - 1 + E + move.E, ply + 1, timeline, move);
        chess.template undoMove<White>(move);

        if (-res > bestRes)
        {
          bestRes = -res;
          bestMove=move;
        }
        if(bestRes > -CHECKMATE + MAX_PLY ) break;
      }
    }

    // Transposition Table Store
    ttEntry.move = bestMove;
    ttEntry.value = bestRes;
    ttEntry.key = key;
    ttEntry.isWhite = White;
    ttEntry.isQSearch = true;
    if (bestRes <= alphaOrig)
    {
      ttEntry.flag = TTEntry::UPPERBOUND;
    }
    else if (bestRes >= beta)
    {
      ttEntry.flag = TTEntry::LOWERBOUND;
    }
    else
    {
      ttEntry.flag = TTEntry::EXACT;
    }
    ttEntry.depth = depth;
    tt.store(ttEntry);

    return bestRes;
  };

  template <U8 Set, U8 Size, U16 L, U16 T, bool White>
  _Compiletime Result negaMax_Test(Chess<Set, Size, L, T> &chess, int depth, int timeline)
  {
    if (depth == 0)
    {
      Result res = Result(evaluate<Set, Size, L, T, White>(chess, timeline, depth), Move(0, 0, 0, 0, NullMove, 0, 0, 0, 0), nullptr);

      return res;
    }

    std::vector<Move> moves;
    moves.reserve(100);
    chess.template generateMoves<White>(moves, timeline);

    Result bestRes = Result(-CHECKMATE, Move(0, 0, 0, 0, NullMove, 0, 0, 0, 0), nullptr);

    for (int i = 0; i < moves.size(); ++i)
    {
      Move move = moves[i];

      chess.template makeMove<White>(move);
      Result res = negaMax_Test<Set, Size, L, T, !White>(chess, depth - 1, timeline);
      chess.template undoMove<White>(move);

      if (-res.value > bestRes.value)
      {
        bestRes.value = -res.value;
        bestRes.moveset = move;
        bestRes.next = std::make_unique<Result>(res.value, res.moveset, std::move(res.next));
      }
    }

    return bestRes;
  };
};