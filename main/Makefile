CFLAGS= -fsanitize=address -g -Wall -ldl -lm -lpthread

.PHONY: compile test clean checkstyle format


compile: game

test: game_test
	./game_test

clean:
	rm -f *.o game game_test ../tui/*.o

checkstyle:
	clang-tidy --quiet $(wildcard *.c) $(wildcard *.h) --

format:
	clang-format -i $(wildcard *.c) [[$(wildcard *.h) != miniaudio.h]]


game: game.o game_lib.o ../tui/tui_matrix.o ../tui/tui_io.o ../tui/ansi_codes.o ../tui/tui.o vec.o
	gcc $(CFLAGS) game.o game_lib.o ../tui/tui_matrix.o ../tui/tui_io.o ../tui/ansi_codes.o ../tui/tui.o vec.o -o game

game.o: game.c ../tui/tui.h ../tui/tui_io.h ../tui/tui_matrix.h ../tui/ansi_codes.h
	gcc $(CFLAGS) -c game.c -o game.o

game_lib.o: game_lib.c game_lib.h ../tui/tui.h ../tui/tui_io.h ../tui/tui_matrix.h ../tui/ansi_codes.h
	gcc $(CFLAGS) -c game_lib.c -o game_lib.o

vec.o: vec.c vec.h
	gcc -fsanitize=address -g -c vec.c -o vec.o

game_test: game_test.o game_lib.o ../tui/tui_matrix.o ../tui/tui.o ../tui/tui_io.o ../tui/ansi_codes.o vec.o ../unity/unity.o
	gcc $(CFLAGS) game_test.o game_lib.o ../tui/tui_matrix.o ../tui/tui.o ../tui/tui_io.o ../tui/ansi_codes.o vec.o ../unity/unity.o -o game_test

game_test.o: game_test.c game_lib.h ../unity/unity.h ../tui/tui_matrix.h ../tui/ansi_codes.h
	gcc $(CFLAGS) -c game_test.c -o game_test.o


../tui/tui.o: ../tui/tui.c ../tui/tui.h ../tui/tui_matrix.h ../tui/ansi_codes.h
	gcc $(CFLAGS) -c ../tui/tui.c -o ../tui/tui.o

../tui/tui_matrix.o: ../tui/tui_matrix.c ../tui/tui_matrix.h ../tui/ansi_codes.h
	gcc $(CFLAGS) -c ../tui/tui_matrix.c -o ../tui/tui_matrix.o

../tui/tui_io.o: ../tui/tui_io.c ../tui/tui_io.h
	gcc $(CFLAGS) -c ../tui/tui_io.c -o ../tui/tui_io.o

../tui/ansi_codes.o: ../tui/ansi_codes.c ../tui/ansi_codes.h
	gcc $(CFLAGS) -c ../tui/ansi_codes.c -o ../tui/ansi_codes.o


../unity/unity.o: ../unity/unity.c ../unity/unity.h ../unity/unity_internals.h
	gcc -fsanitize=address -g -c ../unity/unity.c -o ../unity/unity.o
