CC=gcc
CFLAGS=-I.
DEPS = wrapperSysCall.h queue.h llist.h abstract_user.h 
OBJ = server.c -lpthread wrapperSysCall.c queue.c llist.c abstract_user.c

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

server: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)
