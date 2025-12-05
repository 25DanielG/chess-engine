## Chess Engine in C

## Features
### Graphics
- SDL chess board graphics

### Engine
- Minimax with alpha–beta pruning (`minimax`)
- Quiescence search on captures (`quiesce`)
- Iterative deepening with hard time cut (`find_move`)
- Move ordering: MVV-LVA, two killer moves per ply, and history heuristic (`score_moves`)
- Late move reduction for quiet and late moves
- Quiescence capture pruning
- Null-move pruning
- Pseudo-legal movegen + legality checks + checkmate/stalemate scoring
- Snapshot-based + undoable bitboard for move execution
- Magic bitboard for fast sliding move generation
- Phase evaluation (`blended_eval`, `phase`, `scale`)


### Running
Using CMake, run `make run` for general running or `make debug` for added debug statements. Use `make compile` to only compile the engine.