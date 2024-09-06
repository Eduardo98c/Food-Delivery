CC=gcc
CFLAGS=-I.
DEPS = queue.h llist.h wrapperSysCall.h abstract_user.h
OBJ = client.c queue.c llist.c  wrapperSysCall.c abstract_user.c

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

client: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)
