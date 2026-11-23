# Changelog

## v0.4: Kestrel
### General
- Internal architecture changed to bitboards and bit ray casting for obstruction checks resulting in ~35-40x speedup at same depth compared to v0.3.
- Fixed en passant and castling rights not being concerned for 3-fold repetition
- Fixed search not terminating on encountering 50 move rule draw
- Default search depth changed to 5 in both UCI and GUI modes
- Added FEN mode, imports position from a FEN string and disables the engine

### UCI
- Added output of search time to info string
- Outputs more details when inputted invalid move

### GUI
- Added full game output in algebraic notation after each move, importable to many other chess tools
- Press H to show squares currently attacked by the enemy
- Engine can now play as any team (T toggles the team) and be disabled mid-game (E)

## v0.3: Bee

Small, but hard-working and achieving decent results.
### Features
#### General
- Alpha-beta pruning, negamax, performance optimizations allowing depths 4 and 5 to be feasible.
- Default depth for GUI/UCI changed from 3/3 to 4/5
- Changed promotion consideration to highest value first (usually only makes difference when promoted pawn is imeediately captured, previous versions chose Knight promotion in this case, can matter if opponent blunders and doesn't capture the promoted pawn). Engine will still underpromote if it sees a stalemate threat
- Changed to move applications and rollbacks instead of full-state copies, but board is still copied for now
- Mate evaluation now uses large value (~10000 pawns, +- ply count factor) instead of INFINITY (engine now tries to mate faster or drag out being mated, also fixes various with moves not recording due to INF vs INF comparison returning false) 
- Search now properly stops on encountering checkmate or draw (such branches' results return to parents immediately instead of searching deeper fruitlessly)
- Project name and identity changed to Bondcat

#### UCI
- Mate-in score display
- Search depth display

### Bugfixes
#### General
- Fixed convention disagreement with Fathom on endgame tablebase lookups causing illegal moves output
- Fixed endgame tablebase evaluation score outputting garbage values, now shows WDL score probed from tables
- Fixed engine returning garbage or random moves if checkmate is unavoidable (previously this could happen no matter the team, meaning engine could drag out delivering mate-in-1 by hundreds of moves)
#### UCI
- Fixed FEN string input failing due to bug
#### GUI
- Fixed funky castling (previously could castle you wrong way), now can also castle by "moving" rook to king, not just king to rook
- Fixed game state not being checked after black's move

## v0.2: Ant
### Features
#### General
- Endgame tablebase probing (optional, define `BONDCAT_ENDGAME_TABLES_PATH` to Syzygy tablebases folder, both DTZ and WDL required). If path is inaccessible or tablebases not present, this feature is disabled.
- Added 3-fold and 50-move rule draw detection
- Evaluation function now also increases by 0.02 for every legal move in position
#### UCI
- Added search stats display
#### Bugfixes
- Fixed wrong checkmate detection
- Fixed king sometimes moving illegally
- Fixed crashing on UCI castling

## v0.1: Newborn
Defenseless, but aspiring. Initial release, knows basic rules, can perform in UCI tournaments, performs primitive searches, but plagued with crashes and outputting illegal moves.