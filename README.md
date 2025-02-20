# 5D-Chess-General-Library
For simulating or manipulating 5D chess games/boards.

## Terminology:
Terms are subject to change, say so if you find them confusing or want to change them to something else.
**5DGame**: *Class* - A game or subgame of 5D chess. Has methods for verification and movement, but does not inherently guarantee legality.

**Metaboard**: *1D Array* - An array in 5DGame, with each int index referencing a unique timeline, turn, rank, and file position.

**Board**: *int* - A single chess board with pieces in specific positions. Addressed internally as an int.

**Headboard**: *General Term* - A board that is the farthest right on its own timeline (it can be played.) 

**Piece**: *Const int/char* - Can be used as a general term, but is technically an int associated with a piece type, possibly with different values for color/moved variants.

**(Local) Position**: *General Term* - Refers to the position of a piece from its board's perspective.

**(Global) Location**: *General Term* - Refers to the position of something from the perspective of the metaboard.

**Turn**: *General Term* - Two successive boards/moves, the first being white and the second being black. Can be numbered and abbreviated as T1, T2, etc.

**Half-Turn**: *General Term* - A single side's board or move. Can be numbered and abbreviated as T1-W, T1-B, etc.

**Fullboard**: *General Term* - The Metaboard's internal 1D array representation of a game, including timelines, turns, and individual boards.

## Technical Details

### Metaboard and Piece Representation
The Metaboard is a 1D array, with indices referencing a single 4D location. Indices work on a base system, with:
* Files addressed from 0 to 8, 
* Ranks addressed from 9 to 90 in multiples of 9,
* Turns addressed from 99 to (99 * maxTurns) in multiples of 99, 
* And timelines addressed from 99( maxTurns + 1 ) to maxTimelines(99(maxTurns + 1)) in multiples of 99(maxTurns + 1). 


## Paths of Development:

## Common Code Practice:


## Possible Additions:
