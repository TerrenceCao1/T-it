CC = gcc
CFLAGS = -Wall

include_dirs = -I./inc -I./deps/sha1
src_file = ./src/tit.c

cli:
	$(CC) $(CFLAGS) ./testing/mainCLI.c $(src_file) $(include_dirs) -o cli

test: 
	gcc ./testing/test.c $(src_file) $(include_dirs) -o test 

clean:
	rm -f -r .tit/ *.o test cli
