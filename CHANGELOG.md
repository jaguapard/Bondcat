# Changelog

## v0.3 Bee
Alpha-beta pruning, negamax, performance optimizations.

Changed promotion consideration to highest value first (usually only makes difference when promoted pawn is imeediately captured, previous versions chose Knight promotion in this case, can matter if opponent blunders and doesn't take the piece). Engine will still underpromote if it sees a stalemate threat.

Changed to move apply and rollback instead of full-state copies, but board is still copied for now

UCI:
Mate-in score display (buggy move count for now)
Mate evaluation now uses large value (~10000) instead of INFINITY
Search depth display

Bugfixes:
Fixed convention disagreement with Fathom on endgame tablebase lookups causing illegal moves output
Fixed endgame tablebase evaluation score outputting garbage values, now shows WDL score probed from tables
Fixed engine returning garbage or random moves if checkmate is unavoidable (previously this could happen no matter the team, meaning engine could drag out delivering mate-in-1 by hundreds of moves) 
GUI: fixed funky castling (previously could castle you wrong way), now can also castle by "moving" rook to king, not just king to rook