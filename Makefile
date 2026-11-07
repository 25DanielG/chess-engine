CC = gcc
CFLAGS = -I.
CCD = $(CC) -DDEBUG -g -fsanitize=address
SDL = `pkg-config --cflags --libs sdl2 SDL2_image`

CORE_FILES = src/core/board.c src/core/utils.c src/core/magic.c src/core/see.c
ENGINE_FILES = src/engine/eval.c src/engine/bot.c src/engine/opening.c src/engine/tt.c
APP_FILES = src/app/manager.c src/app/ui_sdl.c
FILES = $(CORE_FILES) $(ENGINE_FILES) $(APP_FILES)

MAIN = src/app/main.c
BUILD = build

TEST_DEPTH ?= 6
TEST_LIMIT ?= 5
TEST_PLIES ?= 150

compile: $(FILES) $(MAIN)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $(FILES) $(MAIN) $(SDL) -o $(BUILD)/a.out

debug: $(FILES) $(MAIN)
	@mkdir -p $(BUILD)
	$(CCD) $(CFLAGS) $(FILES) $(MAIN) $(SDL) -o $(BUILD)/a.out
	./$(BUILD)/a.out

run: compile
	./$(BUILD)/a.out

test-bin: $(FILES) $(MAIN)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) \
	  -DGRAPHICS=0 \
	  -DWHITE_BOT=1 \
	  -DBLACK_BOT=1 \
	  -DWHITE_DEPTH=$(TEST_DEPTH) \
	  -DBLACK_DEPTH=$(TEST_DEPTH) \
	  -DWHITE_LIMIT=$(TEST_LIMIT) \
	  -DBLACK_LIMIT=$(TEST_LIMIT) \
	  -DMAX_GAME_PLIES=$(TEST_PLIES) \
	  -DOPENING_BOOK=0 \
	  $(FILES) $(MAIN) $(SDL) -o $(BUILD)/test_engine

clean:
	@rm -rf $(BUILD) a.out test_engine *.dSYM *~