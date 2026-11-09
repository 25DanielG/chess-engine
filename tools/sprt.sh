#!/bin/sh

NEW=${1:?usage: tools/sprt.sh new_engine base_engine [book.epd] [tc]}
BASE=${2:?usage: tools/sprt.sh new_engine base_engine [book.epd] [tc]}
BOOK=${3:-data/UHO_Lichess_4852_v1.epd}
TC=${4:-8+0.08}
CORES=$(sysctl -n hw.ncpu 2>/dev/null || nproc)

fastchess \
  -engine cmd="$NEW" name=new \
  -engine cmd="$BASE" name=base \
  -each tc="$TC" \
  -openings file="$BOOK" format=epd order=random \
  -games 2 -rounds 50000 -repeat \
  -concurrency "$CORES" \
  -sprt elo0=0 elo1=5 alpha=0.05 beta=0.05 \
  -recover \
  -pgnout file=build/sprt.pgn
