CC = gcc
CFLAGS = -Wall -lz -lssl -lcrypto

include_dirs = -I./inc
src_file = ./src/tit.c

cli:
	$(CC) $(CFLAGS) ./testing/mainCLI.c $(src_file) $(include_dirs) -o tit

test: 
	$(CC) ./testing/test.c $(src_file) $(include_dirs) -g $(CFLAGS) -o test 

clean:
	rm -f -r .tit/ *.o test tit
