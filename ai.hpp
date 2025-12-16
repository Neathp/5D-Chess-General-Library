#include <chrono>
#include <algorithm>
#include <memory>
#include "chess.hpp"
#include <string>
#include <unordered_map>
#include <cmath>
#include <stdexcept>
#include "tt.hpp"
#include "params.hpp"

#include <limits>

#include <cassert>

static U64 count = 0;
static U64 qCount = 0;
static U64 mates = 0;
static U64 collision = 0;
static U64 hitCount = 0;
static U64 NMHcut = 0;
static U64 maxPlyReached = 0;

static constexpr int CHECKMATE = 1000000000;

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
  static constexpr int MAX_PLY = 20;
  static constexpr int MAX_QPLY = 4;
  static constexpr int PLY = 20;
  static constexpr int NUM_PIECES = 12;
  static constexpr int BOARD_SIZE = 8;

  // Move lbrTable[NUM_PIECES * 2 + 1][BOARD_SIZE][BOARD_SIZE]; // should really not be +1 but travels exist

  CircularArray<Move, 8> killerTable;

  TranspositionTable tt(1000000);

  // Move mateKiller[PLY][10];

  enum NodeType : U8
  {
    NodeSpatial,
    NodeTravel,
    NodeQuiesce,
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
  _Compiletime int evaluate(Chess<Set, Size, L, T> &chess, int timeline, int ply)
  {
    double eval = 0;
    
    std::vector<Chess5D::Move> moves;
    moves.reserve(100);
    // Move Count
    chess.template generateMoves<White>(moves, timeline);
    if (moves.size() == 0)
    {
      ++mates;
      return -CHECKMATE + ply;
    }
    // else{ // For Testing purposes 
    //     return 0;
    // }

    U8 turn= chess.timelineInfo[timeline].turn;
    Board<Set>& brd = chess.boards[timeline][turn];
    Board<Set> before = chess.boards[timeline][turn];
    Move nullMove= Chess5D::Move(0, 0, 0, 0, Chess5D::NullMove, timeline, turn, timeline, turn+1);

    TimelineInfo infoCopy= chess.timelineInfo[timeline];
    chess.template makeMove<White>(nullMove);

    std::vector<Chess5D::Move> enemyMoves;
    enemyMoves.reserve(100);
    chess.template generateMoves<!White>(enemyMoves, timeline);

    eval += evalParams.moveVal * (moves.size()); //TODO: Calculate opponnent's move count and subtract it, this causes issues. Might need to make a pass move.
    eval -= evalParams.moveVal * (enemyMoves.size());
    
    // Softmate
    if (std::all_of(moves.begin(), moves.end(), [](const Move &move){ return move.type == Travel; })){
      eval += evalParams.softmate;
    }

    // Piece Count
    for (int i = 0; i < Set/2; ++i)
    {
      U64 pieceW = brd.bitBoard(White, static_cast<PieceType>(i));
      U64 pieceB = brd.bitBoard(!White, static_cast<PieceType>(i));

      eval += evalParams.pieceVal * evalParams.typeToVal[i] * (_popcnt64(pieceW) - _popcnt64(pieceB));
      
      // Castle Ability Bonus
      if (i == 5 || i == 3)
      {
        eval += evalParams.unmoved * (_popcnt64(pieceW & brd.board.unmoved) - _popcnt64(pieceB & brd.board.unmoved));
      }
    }

    // Piece Square Table
    for (int i = 0; i < 6; ++i) 
    {
      U64 pieceW = brd.bitBoard(White, static_cast<PieceType>(i));
      Bitloop (pieceW)
      {
        const U8 sq = SquareOf(pieceW);
        eval += evalParams.pieceSq[i][sq];
      }
      U64 pieceB = brd.bitBoard(!White, static_cast<PieceType>(i));
      Bitloop (pieceB)
      {
        U8 sq = SquareOf(pieceB);
        //Flip the board for black pieces
        sq^=56;
        
        eval -= evalParams.pieceSq[i][sq];
      }
    }

    // Timeline Count
    eval += evalParams.tlValue * ((chess.timelineNum[White]) - (chess.timelineNum[!White]));

    // Past Exposure    
    auto pastExposure = [&](const Board<Set> &b, bool isWhite) -> int {
      bool enemyHasQueen = (b.bitBoard(isWhite, Queen) | b.bitBoard(isWhite, RQueen) | b.bitBoard(isWhite, Unicorn) | b.bitBoard(isWhite, Dragon)) != 0;
      bool enemyHasBishop = enemyHasQueen || ((b.bitBoard(isWhite, Bishop) | b.bitBoard(isWhite, Princess)) != 0);    

      U64 combinedMask = b.pastMask.center;

      if(enemyHasBishop){
        combinedMask|=b.pastMask.north | b.pastMask.south; //ignore east and west usually not worth penalizing
        if(enemyHasQueen) combinedMask|=b.pastMask.northeast | b.pastMask.southeast | b.pastMask.southwest | b.pastMask.northwest;
      }
      
      return evalParams.kingExp * std::max(0, (int)_popcnt64(combinedMask) - evalParams.kingExpTol);
    };

    //Penalize exposure to past checks
    Board<Set> brdOpp = chess.boards[timeline][turn+1];
    eval += pastExposure(brdOpp, White); 
    eval -= pastExposure(brd, !White); 

    // // King Defense (not currently effective, because only checks for attacks, not defenses)
    // auto kingDefense = [&](const Board<Set> brd, bool isWhite) -> int {
    //   int result=0;
    //   U64 royal = brd.royalty(isWhite);
    //   Bitloop (royal)
    //   {
    //     const U8 sq = SquareOf(royal);
    //     result+=evalParams.kingDef * (int)_popcnt64((Lookup::KingAttacks[sq]|Lookup::Rings[sq][1])&brd.banMask);
    //   }
    //   return result;
    // };

    // //Incentivize king defense
    // eval -= kingDefense(brdOpp, !White); 
    // eval += kingDefense(brd, White); 

    chess.template undoMove<White>(nullMove);
    //debugCompare(infoCopy, chess.timelineInfo[timeline]);
    debugCompareBoard<Set>(before, chess.boards[timeline][turn]);

    return eval;
  };

  //Uses TT to backtrace best line
  template <U8 Set, U8 Size, U16 L, U16 T, bool White>
  void backtrace(Chess<Set, Size, L, T> &chess, int timeline, int depth)
  {
      TimelineInfo info = chess.timelineInfo[timeline];
      Board<Set> &brd = chess.boards[timeline][info.turn];

      U64 key = tt.computeHashKey<Set, White>(brd);
      TTEntry *ttEntry = tt.probe(key);

      if (ttEntry && ttEntry->key == key)
      {
          if(ttEntry->isQSearch) std::cout << "invalid\n";
          Move move = ttEntry->move;
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
  
  //Uses Result to backtrace best line
  template <U8 Set, U8 Size, U16 L, U16 T, bool White>
  void backtrace(Chess<Set, Size, L, T> &chess, int depth, const std::unique_ptr<Chess5D::Result> &res)
  {
      if (!res) return;

      std::string prefix;
      if (!White) {
          if (depth == 0) {
              prefix = "1... / ";
          } else {
              prefix = "/ ";
          }
      } else {
          prefix = std::to_string(depth / 2 + 1 + depth % 2) + ". ";
      }

      std::cout << prefix;
      for (std::size_t i = 0; i < res->moveset.size(); ++i) {
          std::cout << chess.template moveToPGN<White>(res->moveset[i]) << " {" << res->moveset[i].score << "} ";
          chess.template makeMove<White>(res->moveset[i]);
      }
      
      if(!White){
        std::cout << std::endl;
      }

      if (res->next) {
          backtrace<Set, Size, L, T, !White>(chess, depth + 1, res->next);
      }
      
      for (std::size_t i = res->moveset.size(); i-- > 0;) {
          chess.template undoMove<White>(res->moveset[i]);
      }
  }

  template <U8 Set, U8 Size, U16 L, U16 T, bool White>
_Compiletime void moveScore(Chess<Set, Size, L, T> &chess, int timeline, Move &move, Move &lastMove)
  {
    // should score with MVV-LVA, piece square table difference, captures/promotions, check caused
    TimelineInfo info = chess.timelineInfo[timeline];
    Board<Set> &brd = chess.boards[timeline][info.turn];

    PieceType pieceFrom = fromPiece(brd.board.mailboxBoard[move.from]);
    PieceType pieceTo = fromPiece(brd.board.mailboxBoard[move.to]);

    bool isTravel = move.type >= Travel;
    if (isTravel)
    {
      // Travel move penalties and bonuses
      if (chess.timelineNum[White] > chess.timelineNum[!White])
      {
          move.score -= moveParams.inactiveTravelPenalty;
          move.E -= extParams.inactiveTravelReduct;
      }
      if (move.type == Travel)
      {
          move.score -= moveParams.travelBasePenalty;
          move.E -= extParams.travelReduct;
      }
      move.score -= moveParams.travelPenalty;

      Board<Set> &brdTravel = chess.boards[move.eTimeline][move.eTurn];
      pieceTo = fromPiece(brdTravel.board.mailboxBoard[move.to]);
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

      // minimal benefits when set to 0 eval??? //Hashmap instead?
      for (Move &killMove : killerTable.array)
      { // Killer
        if (killMove == move)
        {
          move.score += moveParams.killerBonus;
          break;
        }
      }
    }

    // Capture and quiet move scoring
    if (move.type == Capture || move.type == PromoCapture || move.type == TravelCapture || move.type == TravelPromoCapture)
    {
        move.score += moveParams.typeToVal[pieceTo] * moveParams.captureMVVMultiplier - moveParams.typeToVal[pieceFrom] * moveParams.captureLVAMalus + moveParams.captureBaseBonus;
        move.score +=evalParams.pieceSq[pieceTo][move.to]; //Captured piece loses value of square
    }
    else
    {
      move.score -= moveParams.typeToVal[pieceFrom]* moveParams.quietMovePenaltyMultiplier; // probably needs fixing but works well ¯\_(ツ)_/¯
    }
    move.score+=evalParams.pieceSq[pieceFrom][move.to]-evalParams.pieceSq[pieceFrom][move.from]; //Value of moving squares

    // Check bonuses
    chess.template makeMove<White>(move);
    Board<Set> &newBrd = chess.boards[timeline][info.turn + 1];
    if(newBrd.pastCheck==FULL) createMask<Set, T, White>(&newBrd); 

    if(newBrd.pastCheck!=EMPTY && newBrd.pastCheck!=FULL){
      move.score += moveParams.pastCheckBonus;
      move.E += extParams.pastCheckExt;
    }
    else if (newBrd.checkMask != FULL)
    {
      move.score += moveParams.checkBonus; // Check incentive
      move.E += extParams.checkExt;
    }

    if (isTravel) //TODO: add past improvements
    {
      int newTimeline = chess.origIndex[White] + (White ? -1 : 1) * (chess.timelineNum[White]);
      Board<Set> &newBrdTravel = chess.boards[newTimeline][move.eTurn + 1];
      if (newBrdTravel.checkMask != FULL)
      {
        move.score += moveParams.travelCheckBonus; // Travel Check incentive
        move.E += extParams.travelCheckExt;
      }
    }

    chess.template undoMove<White>(move);
}

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

  inline std::unique_ptr<Result> mateDistancePrune(int& alpha, int& beta, int ply) {
    const int upper =  CHECKMATE - ply;   // best possible mate
    const int lower = -CHECKMATE + ply;   // worst possible mate

    // Fail-low: even the best possible score is <= alpha
    if (upper <= alpha){
      return std::make_unique<Result>(upper, std::vector<Chess5D::Move>{}, nullptr);
    }

    // Fail-high: even the worst possible score is >= beta
    if (lower >= beta){
      return std::make_unique<Result>(lower, std::vector<Chess5D::Move>{}, nullptr);
    }

    // Otherwise tighten window
    alpha = std::max(alpha, lower);
    beta  = std::min(beta, upper);

    return nullptr;
  }

  inline int packMate(int score, int ply)
  {
      if (score > CHECKMATE - 1000) return score + ply;
      if (score < -CHECKMATE + 1000) return score - ply;
      return score;
  }

  inline int unpackMate(int score, int ply)
  {
      if (score > CHECKMATE - 1000) return score - ply;
      if (score < -CHECKMATE + 1000) return score + ply;
      return score;
  }

  template <U8 Set, U8 Size, U16 L, U16 T, bool White>
  _Compiletime std::unique_ptr<Result> negaMaxIDDFS(Chess<Set, Size, L, T> &chess, int maxDepth, std::chrono::milliseconds timeLimit)
  {
    auto startTime = std::chrono::high_resolution_clock::now();
    std::unique_ptr<Result> bestRes;

    int alpha = -CHECKMATE;
    int beta = CHECKMATE;
    // Perform iterative deepening within the time limit
    for (int depth = 1; depth <= maxDepth; ++depth)
    {
      auto elapsedTime = duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime);

      if (elapsedTime >= timeLimit)
      {
        std::cout <<"Timeout Depth: " << depth << std::endl;
        break;
      }

      bestRes = negaMax<Set, Size, L, T, White, true>(chess, alpha, beta, depth, 0, std::vector<Chess5D::Move>());

      if (bestRes->value >= CHECKMATE - MAX_PLY)
      {
        //  alpha=bestRes.value-1;
      }

      if (bestRes->value <= -CHECKMATE + MAX_PLY)
      {
        //  beta=bestRes.value+1;
      }
    }

    return bestRes;
  }

  struct MCTSNode {
    double value;
    int samples;
    
    const Chess5D::Move move {};

    MCTSNode *const parent {};
    std::vector<MCTSNode> actions {};

    MCTSNode(MCTSNode *p, Chess5D::Move m) : value(0), samples(0), parent(p), move(m) {}

    _Compiletime void add_actions(std::vector<Chess5D::Move> moves) {
      for (Chess5D::Move move : moves) actions.emplace_back(this, move);
    }
  };

  struct MCTSRollout {
    std::shared_ptr<MCTSRollout> prev {};
    std::shared_ptr<MCTSRollout> next {};
    Chess5D::Move move {};

    MCTSRollout(std::shared_ptr<MCTSRollout> p) : prev(std::move(p)) {}
  };

  template <double C>
  _Compiletime MCTSNode *selectAction(MCTSNode *state) {
    double bestValue = 0;
    MCTSNode *bestAction = nullptr;

    // find the best child
    for (MCTSNode &action : state->actions) {
      double value = action.samples > 0 ? action.value + C * std::sqrt(std::log(state->samples)/action.samples) : std::numeric_limits<double>::infinity();

      if (value > bestValue) {
        bestValue = value;
        bestAction = &action;
      }
    }

    state->samples++;
    bestAction->samples++;

    return bestAction;
  }

  template <U8 Set, U8 Size, U16 L, U16 T, bool White>
  _Compiletime std::unique_ptr<Result> MCTS(Chess<Set, Size, L, T> &chess, int maxSimuls, int maxDepth) {
    int timelinenum = chess.origIndex[0];
    int seed = 0;
    srand(time(NULL));

    // Generate root node with children
    MCTSNode root = MCTSNode(nullptr, Chess5D::Move());
    std::vector<Chess5D::Move> moves;
    moves.reserve(100);
    chess.template generateMoves<White>(moves, timelinenum);
    root.add_actions(moves);

    std::unique_ptr<Result> bestResult = std::make_unique<Result>(-CHECKMATE, std::vector<Chess5D::Move>{}, nullptr);

    bool isWhite = White; // temp solution to get around templating
    for (int i = 0; i < maxSimuls; i++) {
      int curDepth = 0;
    
      MCTSNode *curNode = &root;
      while (curNode->actions.size() > 0) {
        curNode = selectAction<std::sqrt(2)>(curNode);
        if (curDepth % 2 == 0) {
          chess.template makeMove<true>(curNode->move);
        } else {
          chess.template makeMove<false>(curNode->move);
        }
        curDepth++;
      }

      // "add" curNode to the tree (make it non-terminal)
      int depthMarker = curDepth;
      moves.clear();
      if (curDepth % 2 == 0) {
        chess.template generateMoves<true>(moves, timelinenum);
      } else {
        chess.template generateMoves<false>(moves, timelinenum);
      }
      curNode->add_actions(moves);

      // play a random game starting from our current state (chess object is already updated to that position)
      std::shared_ptr<MCTSRollout> curRolloutNode = std::make_shared<MCTSRollout>(nullptr);
      while (moves.size() > 0 && curDepth <= maxDepth) {
        Chess5D::Move move = moves[rand() % moves.size()];
        if (curDepth % 2 == 0) {
          chess.template makeMove<true>(move);
        } else {
          chess.template makeMove<false>(move);
        }
        curRolloutNode->move = move;
        curRolloutNode->next = std::make_shared<MCTSRollout>(curRolloutNode);
        curRolloutNode = curRolloutNode->next;
        curDepth++;

        moves.clear();
        if (curDepth % 2 == 0) {
          chess.template generateMoves<true>(moves, timelinenum);
        } else {
          chess.template generateMoves<false>(moves, timelinenum);
        }
      }

      int value;
      if (curDepth % 2 == 0) {
        value = evaluate<Set, Size, L, T, true>(chess, timelinenum, curDepth); // call evaluation function on final state
      } else {
        value = evaluate<Set, Size, L, T, false>(chess, timelinenum, curDepth); // call evaluation function on final state
      }
      value = (curDepth - depthMarker) % 2 == 0 ? value : -value;

      // reverse back up the random game until we hit the tree (signified by depthMarker)
      while (curDepth > depthMarker){
        curDepth--;

        curRolloutNode = curRolloutNode->prev;
        Chess5D::Move move = curRolloutNode->move;
        if (curDepth % 2 == 0) {
          chess.template undoMove<true>(move);
        } else {
          chess.template undoMove<false>(move);
        }
      }

      // assign curNode the evaluation
      curNode->value = value;

      std::unique_ptr<Result> curResult = std::make_unique<Result>(value, std::vector<Chess5D::Move>{}, nullptr); // for the library
      // start backpropagating the values
      while (curDepth > 0) {
        curDepth--;

        Chess5D::Move move = curNode->move;
        if (curDepth % 2 == 0) {
          chess.template undoMove<true>(move);
        } else {
          chess.template undoMove<false>(move);
        }
        curNode = curNode->parent;

        // backpropagate the emperical average
        double sum = 0;
        int count = 0;
        for (MCTSNode &action : curNode->actions) {
          if (action.actions.size() > 0) {
            sum -= action.value;
            count++;
          }
        }
        
        curNode->value = sum/count;
        curResult = std::make_unique<Result>(curNode->value, std::vector<Chess5D::Move>{}, nullptr);
        curResult->moveset.push_back(move);
      }

      curResult->value = curNode->value;
      if (curResult->value > bestResult->value) bestResult.swap(curResult); // if better, update result

      std::cout << "Rollout " << i << " Complete\n";
    }

    return bestResult; 
  }

  template <U8 Set, U8 Size, U16 L, U16 T, bool White, bool PV>
  _Compiletime std::unique_ptr<Result> negaMax(Chess<Set, Size, L, T> &chess, int alpha, int beta, int depth, int ply, std::vector<Move> lastMoves)
  {
    //Single Timeline NegaMax
    if (chess.timelineNum[0] + chess.timelineNum[1] == 0 && chess.origIndex[0] == chess.origIndex[1])
    {
      Move tempNullMove = Move(0, 0, 0, 0, NullMove, 0, 0, 0, 0);
      return negaMax1<Set, Size, L, T, White, PV, NodeSpatial>(chess, alpha, beta, depth, 0, chess.origIndex[0], tempNullMove);
    }

    // Eval/Quiesce
    if (depth <= 0 || ply > MAX_PLY)
    {
      std::unique_ptr<Result> res = std::make_unique<Result>(0, std::vector<Chess5D::Move>{}, nullptr);
      for (int timeline = chess.origIndex[1] - chess.activeNum[1]; timeline <= chess.origIndex[0] + chess.activeNum[0]; ++timeline)
      {
        Chess5D::Move lastMove = Move(0, 0, 0, 0, NullMove, 0, 0, 0, 0);
        for (const Chess5D::Move &move : lastMoves) {
          if (move.sTimeline == timeline)
            lastMove = move;
        }
        //res->value += quiesce<Set, Size, L, T, White, PV>(chess, alpha, beta, -1, ply, timeline, lastMove);
      }

      return res;
    }
    std::vector<std::vector<Chess5D::Move>> movesAll;
    int cnt = 0;
    //For each timeline, generate moves and score them based on single timeline negaMax
    for (int i = chess.origIndex[1] - chess.activeNum[1]; (i <= chess.origIndex[0] + chess.activeNum[0]); ++i)
    {
      if(chess.timelineInfo[i].turn != chess.present)
      {
        continue; // skip timelines that are not present
      }

      std::vector<Chess5D::Move> moves;
      chess.template generateMoves<White>(moves, i);

      // Score
      for (Move &move : moves)
      {
        Move tempNullMove = Move(0, 0, 0, 0, NullMove, 0, 0, 0, 0);
        moveScore<Set, Size, L, T, White>(chess, i, move, tempNullMove);
      }

      // Sort
      std::sort(moves.begin(), moves.end(), [](const Move &a, const Move &b)
                { return a.score > b.score; });

      // Search
      for (Move &move : moves)
      { // could maybe add alpha beta cutoffs at this depth
        chess.template makeMove<White>(move); //Set PV for first move
        std::unique_ptr<Result> res = negaMax1<Set, Size, L, T, !White, false, NodeSpatial>(chess, -beta, -alpha, depth - 1, ply+1, i, move);
        chess.template undoMove<White>(move);

        move.score = -res->value;
      }

      // Resort
      std::sort(moves.begin(), moves.end(), [](const Move &a, const Move &b)
                { return a.score > b.score; });

      movesAll.push_back(moves);
      cnt++;
    }

    // naively attempt to play all the best moves of every present timeline and only search a handful of those
    std::unique_ptr<Result> bestRes = std::make_unique<Result>(-CHECKMATE-1, std::vector<Chess5D::Move>(), nullptr);
    std::vector<std::vector<Move>> moveList = combinations(movesAll, 2); //TODO: should probably be 30 moves divided by timelines^2 or something like that
    for (int i = 0; i < moveList.size(); ++i)
    {
      std::vector<Move> &moves=moveList[i];
      chess.template makeMoveset<White>(moves);
      std::unique_ptr<Result> res = negaMax<Set, Size, L, T, !White, PV>(chess, -beta, -alpha, depth - 1, ply+1, moves);
      chess.template undoMoveset<White>(moves);
      
      if (-res->value > bestRes->value)
      {
        bestRes->value = -res->value;
        bestRes->moveset = moves;
        bestRes->next = std::move(res);;
      }
      if (bestRes->value > alpha)
        alpha = bestRes->value;

      if (alpha >= beta)
      {
        break;
      }
    }
    if(bestRes->moveset.size() == 0 && moveList.size() > 0)//some reason checkmate is getting returned and if moves tie checkmate no move is saved.
    {
      // If no moves were found, return the first move of the first timeline
      bestRes->moveset = moveList[0];
    }

    return bestRes;
  };

  template <bool White, NodeType Node>
  _Compiletime TTEntry* ttLookup(U64 key, int &alpha, int &beta, int depth, int ply, std::unique_ptr<Result> &resOut) {
      TTEntry* ttEntry = tt.probe(key);
      if (!ttEntry || ttEntry->key != key ||(Node != NodeQuiesce && (ttEntry->isQSearch || ttEntry->depth < depth)))
          return nullptr;

      if (ttEntry->isWhite != White) collision++;
      hitCount++;

      int realValue = unpackMate(ttEntry->value, ply);

      if (ttEntry->flag == TTEntry::LOWERBOUND) alpha = std::max(alpha, realValue);
      else if (ttEntry->flag == TTEntry::UPPERBOUND) beta = std::min(beta, realValue);

      if (ttEntry->flag == TTEntry::EXACT || alpha >= beta) 
          resOut = std::make_unique<Result>(realValue, std::vector<Move>{ttEntry->move}, nullptr);

      return ttEntry;
  }

  template <bool White, NodeType Node>
  _Compiletime void ttStore(Move bestMove, int value, int depth, int alphaOrig, int betaOrig, U64 key, int ply){
    TTEntry entry; // local temporary
    entry.move      = bestMove;
    entry.value     = packMate(value, ply);
    entry.key       = key;
    entry.isWhite   = White;
    entry.isQSearch = (Node == NodeQuiesce);
    if (value <= alphaOrig) {
        entry.flag = TTEntry::UPPERBOUND;
    } else if (value >= betaOrig) {
        entry.flag = TTEntry::LOWERBOUND;
    } else {
        entry.flag = TTEntry::EXACT;
    }
    entry.depth = depth;

    tt.store(entry);
  }

    template <U8 Set, U8 Size, U16 L, U16 T, bool White>
  _Compiletime std::unique_ptr<Result> qSearch(Chess<Set, Size, L, T> &chess, int alpha, int beta, int ply, int timeline, Move lastMove)
  {
    int alphaOrig = alpha;
    int betaOrig = beta;
    TimelineInfo &info = chess.timelineInfo[timeline];
    Board<Set> &brd = chess.boards[timeline][info.turn];

    if (auto res = mateDistancePrune(alpha, beta, ply)) return res;

    // Transposition Table Lookup
    U64 key = tt.computeHashKey<Set, White>(brd);
    std::unique_ptr<Result> ttRes;
    TTEntry* ttEntry = ttLookup<White, NodeQuiesce>(key, alpha, beta, 0, ply, ttRes);
    if (ttRes) return ttRes;

    //In Check?
    createMask<Set, T, White>(&brd); //Maybe i should just be checking for past check already as well.
    bool inCheck = brd.pastCheck != EMPTY || brd.checkMask != FULL;
    int eval=0;
    if(!inCheck){
      //Stand Pat
      eval = evaluate<Set, Size, L, T, White>(chess, timeline, ply); //TODO: Needs to gen moves anyways... might as well store and pass
      if (ply > MAX_QPLY){
        qCount++;
        return std::make_unique<Result>(eval, std::vector<Chess5D::Move>{}, nullptr);
      }
      else if (eval >= beta){
        qCount++;
        return std::make_unique<Result>(beta, std::vector<Chess5D::Move>{}, nullptr);
      }
      else if (eval > alpha){
        alpha = eval;
      }
    }

    std::vector<Move> moves;
    moves.reserve(100);
    chess.template generateMoves<White>(moves, timeline); //TODO: For QSearch, add flags to movegen for only captures/checks/travels/no travels

    if (moves.size() == 0){
      qCount++; mates++;
      return std::make_unique<Result>(-CHECKMATE + ply, std::vector<Chess5D::Move>{}, nullptr);
    }

    std::vector<Move> loudMoves;
    loudMoves.reserve(80);
    for (auto it = moves.begin(); it != moves.end(); ++it) {
      if(it->type >= Travel) continue; //Ignore Travels in Quiescence(could maybe be changed?)

      bool isLoud=(inCheck||it->type == Capture || it->type == Enpassant || it->type == Promotion ||it->type == PromoCapture);
      if(!isLoud){
          chess.template makeMove<White>(*it);
          Board<Set>& newBrd = chess.boards[timeline][info.turn];
          createMask<Set, T, !White>(&newBrd);
          isLoud = (newBrd.pastCheck != EMPTY || newBrd.checkMask != FULL);
          chess.template undoMove<White>(*it);
      }
      if (isLoud){
        loudMoves.push_back(*it);
        moveScore<Set, Size, L, T, White>(chess, timeline, *it, lastMove);
      }
    }

    /*
            // Step 6. Pruning
        if (!is_loss(bestValue))
        {
            // Futility pruning and moveCount pruning
            if (!givesCheck && move.to_sq() != prevSq && !is_loss(futilityBase)
                && move.type_of() != PROMOTION)
            {
                if (moveCount > 2)
                    continue;

                Value futilityValue = futilityBase + PieceValue[pos.piece_on(move.to_sq())];

                // If static eval + value of piece we are going to capture is
                // much lower than alpha, we can prune this move.
                if (futilityValue <= alpha)
                {
                    bestValue = std::max(bestValue, futilityValue);
                    continue;
                }

                // If static exchange evaluation is low enough
                // we can prune this move.
                if (!pos.see_ge(move, alpha - futilityBase))
                {
                    bestValue = std::min(alpha, futilityBase);
                    continue;
                }
            }

            // Continuation history based pruning
            if (!capture
                && (*contHist[0])[pos.moved_piece(move)][move.to_sq()]
                       + pawnHistory[pawn_history_index(pos)][pos.moved_piece(move)][move.to_sq()]
                     <= 5475)
                continue;

            // Do not search moves with bad enough SEE values
            if (!pos.see_ge(move, -78))
                continue;
        }
        */

    if (loudMoves.size() == 0){
      qCount++;
      return std::make_unique<Result>(eval, std::vector<Chess5D::Move>{}, nullptr);
    }

    std::sort(loudMoves.begin(), loudMoves.end(), [](const Move &a, const Move &b){ return a.score > b.score; });
 
    std::unique_ptr<Result> best = std::make_unique<Result>(inCheck?-CHECKMATE+ply:eval, std::vector<Move>{loudMoves[0]}, nullptr); 
    std::unique_ptr<Result> res;
    for (int i = 0; i < loudMoves.size(); ++i){
      Move move = loudMoves[i];

      chess.template makeMove<White>(move);
      res = qSearch<Set, Size, L, T, !White>(chess, -beta, -alpha, ply + 1, timeline, move);
      chess.template undoMove<White>(move);

      if (-res->value > best->value){
        best->value = -res->value;
        best->moveset = std::vector<Move>{move};
        best->next = std::move(res);;
      
        if (best->value > alpha) alpha = best->value;
        if (alpha >= beta){
          killerTable.addElement(move);
          break;
        }
      }
    }
    
    ttStore<White, NodeQuiesce>(best->moveset[0], best->value, 0, alphaOrig, betaOrig, key, ply);
    
    return best;
  };

  template <U8 Set, U8 Size, U16 L, U16 T, bool White, bool PV, NodeType Node>
  _Compiletime std::unique_ptr<Result> negaMax1(Chess<Set, Size, L, T> &chess, int alpha, int beta, int depth, int ply, int timeline, Move lastMove)
  {
    int alphaOrig = alpha;
    int betaOrig = beta;
    U8 turn = chess.timelineInfo[timeline].turn;
    Board<Set> &brd = chess.boards[timeline][turn];

    if (auto res = mateDistancePrune(alpha, beta, ply)) return res;

    // Transposition Table Lookup
    std::unique_ptr<Result> ttRes;
    U64 key = tt.computeHashKey<Set, White>(brd);
    TTEntry* ttEntry = ttLookup<White, Node>(key, alpha, beta, depth, ply, ttRes);
    if (ttRes) return ttRes;

    if (depth <= 0 || ply > MAX_PLY){ //TODO: If mate returned continue to run search deeper? Prevents incorrect reporting. Yes currently getting incorrect reporting.
      //QSearch
      if(ply>maxPlyReached){
        maxPlyReached=ply;
      }
      ++count;
      //return qSearch<Set, Size, L, T, White>(chess, -beta, -alpha, ply, timeline, lastMove); 
      return std::make_unique<Result>(evaluate<Set, Size, L, T, White>(chess, timeline, ply), std::vector<Chess5D::Move>{}, nullptr); //For having no QSearch
    }

    //NMH
    createMask<Set, T, White>(&brd);
    bool inCheck = brd.pastCheck != EMPTY || brd.checkMask != FULL;
    std::unique_ptr<Result> res;
    if (depth>3 && !inCheck && !PV && !(lastMove.type==Chess5D::NullMove && lastMove.eTurn!=0)){ //TODO:Make sure NMH is tuned so that it doesn't cut too much.
      Move nullMove= Chess5D::Move(0, 0, 0, 0, Chess5D::NullMove, timeline, turn, timeline, turn+1);
      chess.template makeMove<White>(nullMove);
      int R = extParams.NMHReduct + depth / 6;
      res = negaMax1<Set, Size, L, T, !White, false, Node>(chess, -beta, -beta+1, depth-1-R, ply + 1, timeline, nullMove);
      chess.template undoMove<White>(nullMove);
      if (-res->value >= beta) {
        NMHcut++;
        return std::make_unique<Result>(-res->value, std::vector<Move>{}, nullptr); 
      }
    }
    
    std::vector<Move> moves;
    moves.reserve(100);
    chess.template generateMoves<White>(moves, timeline); //TODO: For QSearch, add flags to movegen for only captures/checks/travels/no travels
    if(ply==0) std::cout<<moves.size()<<std::endl;

    if constexpr (Node==NodeTravel){// Remove Travels completely if we already travelled //TODO: Maybe change to if the travel would be inactive
      moves.erase(std::remove_if(moves.begin(), moves.end(), [&](Move &move) { return move.type >= Travel; }),moves.end());
    }

    int E = 0; // Extension Value
    if (moves.size() <= extParams.onlyReplyThreshold){
      if (moves.size() == 0){
        count++;
        mates++;
        return std::make_unique<Result>(-CHECKMATE + ply, std::vector<Chess5D::Move>{}, nullptr);
      }
      E += extParams.onlyReply; // Only Reply
    }

    // Move Ordering
    bool PVMove = false;
    for (auto it = moves.begin(); it != moves.end(); ++it) {
      moveScore<Set, Size, L, T, White>(chess, timeline, *it, lastMove);
      if (PV && ttEntry &&*it == ttEntry->move)
      {
        PVMove = true;
        it->E+= extParams.PVExt;
        std::rotate(moves.begin(), it, moves.end());
      }       
    }

    std::sort(moves.begin() + (int)PVMove, moves.end(), [](const Move &a, const Move &b) { return a.score > b.score; });

    std::unique_ptr<Result> best = std::make_unique<Result>(-CHECKMATE + ply, std::vector<Move>{moves[0]}, nullptr);
    for (int i = 0; i < moves.size(); ++i)
    {
      Move move = moves[i];

      if (move.type >= Travel) continue; //Ignore Travels for now

      //LMR
      if (depth >= extParams.LMRDepth && !inCheck && i>extParams.LMRStart && best->value > -CHECKMATE + ply && move.E<=0)
        move.E -= extParams.LMR; //int(log(i - LMRStart+2)); // LMR, should be more efficient

      chess.template makeMove<White>(move);
      if (move.type >= Travel)
      {
        U8 newTimeline = chess.origIndex[White] + (White ? -1 : 1) * (chess.timelineNum[White]);
        int newDepth = std::min(depth, move.sTurn - move.eTurn) - 1 + E + move.E; //TODO: Maybe change to be more flexible in depth searched
        (PV && i == 0) ?
          res = negaMax1<Set, Size, L, T, !White, true, NodeTravel>(chess, -beta, -alpha, newDepth, ply + 1, newTimeline, move):
          res = negaMax1<Set, Size, L, T, !White, false, NodeTravel>(chess, -beta, -alpha, newDepth, ply + 1, newTimeline, move);
      }
      else
      {
        int newDepth = depth - 1 + E + move.E; //TODO: Add extensions/reductions
        //int newDepth = depth-1; to turn off extensions/reductions
        (PV && i == 0) ?
          res = negaMax1<Set, Size, L, T, !White, true, Node>(chess, -beta, -alpha, newDepth, ply + 1, timeline, move):
          res = negaMax1<Set, Size, L, T, !White, false, Node>(chess, -beta, -alpha, newDepth, ply + 1, timeline, move);
      }
      chess.template undoMove<White>(move);

      //std::cout << depth << ": "<<chess.template moveToPGN<White>(move) <<" {" << move.score << "}"<< " {" << -res->value << "}" << std::endl;
      
      if (-res->value > best->value)
      {
        best->value = -res->value;
        best->moveset = std::vector<Move>{move};
        best->next = std::move(res);
        if (best->value > alpha) alpha = best->value;
        if (alpha >= beta)
        {
          killerTable.addElement(move);
          break;
        }
      }
    }
    
    ttStore<White, Node>(best->moveset[0], best->value, depth, alphaOrig, betaOrig, key, ply);
    
    return best;
  };

  template <U8 Set, U8 Size, U16 L, U16 T, bool White>
  _Compiletime std::unique_ptr<Result> negaMax_Test(Chess<Set, Size, L, T> &chess, int depth, int timeline) {
    
    TimelineInfo &info = chess.timelineInfo[timeline];
    Board<Set> &brd = chess.boards[timeline][info.turn];

    assert(brd.board.occ == (brd.board.white | brd.board.black));

    U64 pieceOcc = EMPTY;
    U64 pieceWhite = EMPTY;
    U64 pieceBlack = EMPTY;
    for(int i=0; i<Set; i++){
        pieceOcc |= brd.board.bitBoard[i];
        for(int j = 0; j<64; j++){
          assert(((brd.board.bitBoard[i] & (1ULL << j)) != 0) == (brd.board.mailboxBoard[j] == i));
        }
        if(i % 2) {
            pieceWhite |= brd.board.bitBoard[i];
        } else {
            pieceBlack |= brd.board.bitBoard[i];
        }
    }

    assert((brd.board.epTarget & ~(RANKMASK[3] | RANKMASK[4])) == 0);
    assert((brd.board.epTarget&~pieceOcc)==0);
    assert((brd.board.unmoved&~pieceOcc)==0);
    assert(brd.board.occ   == pieceOcc);
    assert(brd.board.white == pieceWhite);
    assert(brd.board.black == pieceBlack);
    assert((brd.board.white & brd.board.black) == 0);

    std::vector<Move> moves;
    moves.reserve(100);
    chess.template generateMoves<White>(moves, timeline);
    if( moves.size() == 0) {
      count++;
      mates++;
      return std::make_unique<Result>(-CHECKMATE, std::vector<Chess5D::Move>{}, nullptr);
    }
    if (depth == 0) {
      count++;
      return std::make_unique<Result>(0, std::vector<Chess5D::Move>{}, nullptr);
    }

    auto bestRes = std::make_unique<Result>(-CHECKMATE, std::vector<Chess5D::Move>{}, nullptr);

    for (int i = 0; i < moves.size(); ++i) {
      Move move = moves[i];

      TimelineInfo infoBefore = info;
      Board<Set> brdBefore = brd;
      chess.template makeMove<White>(move);
      auto res = negaMax_Test<Set, Size, L, T, !White>(chess, depth - 1, timeline);
      chess.template undoMove<White>(move);
      debugCompare(infoBefore, info);
      debugCompareBoard<Set>(brdBefore, brd);
      int score = -res->value;
      if (score > bestRes->value) {
          bestRes->value = score;
          bestRes->moveset = std::vector<Chess5D::Move>{move};
          bestRes->next = std::move(res);
      }
    }

    return bestRes;
};
};