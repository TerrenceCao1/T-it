CC = gcc
CFLAGS = -Wall -lz -lssl -lcrypto -g

include_dirs = -I./inc
src_files = ./src/init.c ./src/hash-object.c ./src/cat-file.c 

cli:
	$(CC) $(CFLAGS) ./testing/mainCLI.c $(src_files) $(include_dirs) -o tit

test: 
	$(CC) ./src/tit_test.c $(src_files) $(include_dirs) -g $(CFLAGS) -o test 

test_index:
	$(CC) ./testing/test.c $(src_files) $(include_dirs) -g $(CFLAGS) -o test 

clean:
	rm -f -r .tit/ *.o test tit
