## Chess Engine in C

## Features
### Graphics
- SDL chess board graphics

### ELO
ELO referee system in `tools/referee.py`. Must specify two binary paths and call the file like `python3 tools/referee.py build/uci /opt/homebrew/bin/stockfish --games 100 --movetime 100 --opponent-elo 2000`.
Current results:
- 6/5/9 WDL against Stockfish 18 2500 ELO
- Estimated 2450 ELO

### Benchmarking
- Bench command, grading nodes and nodes per second.
  - 2,171,603 nodes
  - 1,089,541 nps
- SPRT, engine comparison
  - Copy engine `cp build/uci build/uci_base`
  - Build new engine `make uci`
  - Run SPRT `tools/sprt.sh build/uci build/uci_base` with 100000 games (new vs. base)
- go nodes N and info lines
 - Example: `info depth 20 score cp 40 nodes 40451861 nps 1126384 time 35913 pv e2e4 e7e5 g1f3 d7d5 d2d4 d5e4 f3e5 f8b4 c2c3 b4d6 d1e2 c7c6 b1d2 d6e5 d4e5 g8e7 h2h3 e8g8 d2e4 c8e6 c1d2`

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