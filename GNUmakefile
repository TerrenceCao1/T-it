CC = gcc
CFLAGS = -Wall -lssl -lcrypto

include_dirs = -I./inc
src_file = ./src/tit.c

cli:
	$(CC) $(CFLAGS) ./testing/mainCLI.c $(src_file) $(include_dirs) -o cli

test: 
	$(CC) ./testing/test.c $(src_file) $(include_dirs) -g $(CFLAGS) -o test 

clean:
	rm -f -r .tit/ *.o test cli
