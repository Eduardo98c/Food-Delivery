Compilazione ed Esecuzione

# Server
# Vai nella cartella Server_Delivery e compila con:

gcc server.c -o server -lpthread queue.c llist.c wrapperSysCall.c abstract_user.c

# oppure usa make:
make

# Esegui il server con:

./server

# Ristorante
# Vai nella cartella ristorante e compila con:

gcc ristorante.c -o ristorante -lpthread queue.c llist.c wrapperSysCall.c abstract_user.c

# oppure usa make:

make

# Esegui il ristorante con:

./ristorante

# Client
# Vai nella cartella client e compila con:

gcc client.c -o client queue.c llist.c wrapperSysCall.c abstract_user.c

# oppure usa make:

make

# Esegui il client con:

./client

# Rider
# Vai nella cartella rider e compila con:

gcc rider.c -o rider queue.c llist.c wrapperSysCall.c abstract_user.c

# oppure usa make:

make

# Esegui il rider con:

./rider
