## Chess Engine in C

## Features
### Graphics
- SDL chess board graphics

### ELO
ELO referee system in `tools/referee.py`. Must specify two binary paths and call the file like `python3 tools/referee.py build/uci /opt/homebrew/bin/stockfish --games 100 --movetime 100 --opponent-elo 2000`.
Current results:
- 7/1/2 WDL against Stockfish 18 2000 ELO
- 4/0/6 WDL against Stockfish 18 2500 ELO
- Projected 2430 ELO

### Engine
- Minimax with alpha–beta pruning (`minimax`)
- Quiescence search on captures (`quiesce`)
- Iterative deepening with hard time cut (`find_move`)
- Move ordering: MVV-LVA, two killer moves per ply, and history heuristic (`score_moves`)
- Static exchange evaluation
- Late-move reduction for quiet and late moves
- Late-move pruning
- Quiescence capture pruning
- Null-move pruning
- Check extensions
- Transposition table
- Principle variation search / Negascout
- Futility pruning: node futility at shallow depth, move-based futility for quiet late moves, delta-like futility in quiescence
- Razoring (forward pruning, search at reduced depth before searching at a full depth)
- Pseudo-legal movegen + legality checks + checkmate/stalemate scoring
- Snapshot-based + undoable bitboard for move execution
- Magic bitboard for fast sliding move generation
- Phase evaluation (`blended_eval`, `phase`, `scale`)


### Running
Using CMake, run `make run` for general running or `make debug` for added debug statements. Use `make compile` to only compile the engine.