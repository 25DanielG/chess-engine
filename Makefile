CC = gcc
CCD = $(CC) -DDEBUG -g -fsanitize=address
SDL = `pkg-config --cflags --libs sdl2 SDL2_image`
FILES = board.c utils.c magic.c eval.c bot.c opening.c manager.c ui_sdl.c

compile: $(FILES) main.c
	$(CC) $(FILES) main.c $(SDL)

debug: $(FILES) main.c
	$(CCD) $(FILES) main.c $(SDL)
	./a.out

run: compile
	./a.out

clean:
	@rm -rf a.out *.dSYM *~