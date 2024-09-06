CC=gcc
CFLAGS=-I.
DEPS = queue.h llist.h wrapperSysCall.h abstract_user.h
OBJ = rider.c queue.c llist.c wrapperSysCall.c abstract_user.c

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

rider: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)
