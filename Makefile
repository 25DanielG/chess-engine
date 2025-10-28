CC = gcc
CCD = $(CC) -DDEBUG -g -fsanitize=address

compile: board.c magic.c eval.c bot.c manager.c main.c
	$(CC) board.c magic.c eval.c bot.c manager.c main.c

debug: board.c magic.c eval.c bot.c manager.c main.c
	$(CCD) board.c magic.c eval.c bot.c manager.c main.c
	./a.out

run: compile
	./a.out

clean:
	@rm -rf a.out *.dSYM *~
