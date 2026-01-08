CC = gcc
CFLAGS = -Wall -lz -lssl -lcrypto -g -lm

include_dirs = -I./inc
src_files = ./src/init.c ./src/hash-object.c ./src/cat-file.c ./src/add.c ./src/write-tree.c

cli:
	$(CC) $(CFLAGS) ./testing/mainCLI.c $(src_files) $(include_dirs) -o tit

test: 
	$(CC) ./src/tit_test.c $(src_files) $(include_dirs) -g $(CFLAGS) -o test 

test_index:
	$(CC) ./testing/test_index.c $(src_files) $(include_dirs) -g $(CFLAGS) -o test 

test_tree:
	$(CC) ./testing/test_tree.c $(src_files) $(include_dirs) -g $(CFLAGS) -o test

clean:
	rm -f -r .tit/ *.o test tit
