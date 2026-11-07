CC = gcc
CCD = $(CC) -DDEBUG -g -fsanitize=address
SDL = `pkg-config --cflags --libs sdl2 SDL2_image`
FILES = board.c utils.c magic.c eval.c bot.c opening.c manager.c ui_sdl.c tt.c see.c

TEST_DEPTH ?= 6
TEST_LIMIT ?= 5
TEST_PLIES ?= 150

compile: $(FILES) main.c
	$(CC) $(FILES) main.c $(SDL)

debug: $(FILES) main.c
	$(CCD) $(FILES) main.c $(SDL)
	./a.out

run: compile
	./a.out

test-bin: $(FILES) main.c
	$(CC) \
	  -DGRAPHICS=0 \
	  -DWHITE_BOT=1 \
	  -DBLACK_BOT=1 \
	  -DWHITE_DEPTH=$(TEST_DEPTH) \
	  -DBLACK_DEPTH=$(TEST_DEPTH) \
	  -DWHITE_LIMIT=$(TEST_LIMIT) \
	  -DBLACK_LIMIT=$(TEST_LIMIT) \
	  -DMAX_GAME_PLIES=$(TEST_PLIES) \
	  -DOPENING_BOOK=0 \
	  $(FILES) main.c $(SDL) -o test_engine

clean:
	@rm -rf a.out test_engine *.dSYM *~