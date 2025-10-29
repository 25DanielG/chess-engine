CC = gcc
CCD = $(CC) -DDEBUG -g -fsanitize=address
SDL = `pkg-config --cflags --libs sdl2 SDL2_image`

compile: board.c magic.c eval.c bot.c manager.c ui_sdl.c main.c
	$(CC) board.c magic.c eval.c bot.c manager.c ui_sdl.c main.c $(SDL)

debug: board.c magic.c eval.c bot.c manager.c ui_sdl.c main.c
	$(CCD) board.c magic.c eval.c bot.c manager.c ui_sdl.c main.c $(SDL)
	./a.out

run: compile
	./a.out

clean:
	@rm -rf a.out *.dSYM *~