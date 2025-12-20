CC = gcc
CFLAGS = -Wall

include_dir = ./inc
src_file = ./src/tit.c

cli:
	$(CC) $(CFLAGS) ./testing/mainCLI.c $(src_file) -I$(include_dir) -o cli

test: 
	gcc ./testing/test.c $(src_file) -I$(include_dir) -o test 

clean:
	rm -f -r .tit/ *.o test cli
