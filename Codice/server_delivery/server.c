#include<stdio.h>	 
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include<signal.h>
#include<string.h>
#include<time.h>
#include<errno.h> 
#include<syslog.h>      
#include<sys/types.h>
#include<sys/wait.h>         
#include<arpa/inet.h> 
#include<netinet/in.h>  
#include<sys/socket.h>
#include<pthread.h>
#include<semaphore.h>
#include"wrapperSysCall.h"
#include"abstract_user.h"
#include"wrapperSocket.h"
#include"wrapperString.h"
#include"queue.h"
#include"llist.h"


#define SERVERPORT 8989
#define SERVER_BACKLOG 1000

#define SERVER_ID 0
#define RISTORANTE_ID 1
#define CLIENT_ID 2
#define RIDER_ID 3


//strutture
typedef struct sockaddr SA;          /* struttura sockaddr */
typedef struct sockaddr_in SA_IN;    /* struttura sockaddr_in */
typedef socklen_t SL;                /* struttura socklen_t */


//STRUTTURA RISTORANTE
struct ristorante{
   char nome[MAX_NOME_RISTORANTE];
   char posizione[MAX_POSIZIONE_RISTORANTE];
   int id;
   char ip[NI_MAXHOST];
   short porta;
   queue *queue_menu_ordinazioni;
};
typedef struct ristorante Ristorante;

//DATI CONDIVISI TRA I THREAD CHE GESTISCONO RICHIESTE DAI RISTORANTI
struct shared_client1{
   int num_restaurant;
   queue *queue_client1;
   sem_t mutex_queue1;  
}S1;

//DATI CONDIVISI TRA I THREAD CHE GESTISCONO RICHIESTE DAI CLIENT
struct shared_client2{
   int num_client;
   queue *queue_client2;
   sem_t mutex_queue2;
}S2;

//DATI CONDIVISI TRA I THREAD CHE GESTISCONO RICHIESTE DAI RIDERS
struct shared_client3{
   int num_rider;
   queue *queue_client3;
   sem_t mutex_queue3;  
}S3;

//DATI CONDIVISI GLOBALI TRA I THREADS
struct shared_glob{
   llist *list_restaurant;
   sem_t mutex_list_restaurant;
}SG;

//FUNZIONI DI RETE
void close_server();                                                           /* Chiusura pulita socket in caso di sigstop */
int setup_server_tcp(short port, int backlog);                                 /* Inizializzazione socket tcp */
int accept_new_connection(int server_socket);                                  /* Accetta una nuova connessione client */
int connetti_tcp(char *server_addr, short port);                               /* Connette il server ad un ristorante */
void notifica_nuovo_client(int client_socket,int tipo_client);                 /* Notifica i nuovi client(ristoranti o client) pronti per essere serviti */

//FUNZIONI PER INVIARE/RICEVERE DATI

//@INVIA RISTORANTE
void invia_info_ristorante(int socket,Ristorante *ristoranteInput, int typeClient); /* Invia le informazioni dei ristoranti ai client(client o rider) */
void invia_lista_ristoranti(int socket,llist *list_restaurant,int typeClient); /* Usa sizeList volte'invia_info_ristorante_a_tipo per inviare la lista di ristoranti */

//@RICEVI RISTORANTE
Ristorante *ricevi_info_ristorante(int socket,int typeClient);                 /* Funzione per ricevere le informazioni ristorante da un client */

//FUNZIONI DI STAMPA
void stampa_info_ristorante(Ristorante *ristorante);                           /* Stampa le informazioni di un ristorante in input */

//FUNZIONI CONTROLLO SCELTA UTENTE
Ristorante *controlla_presenza_ristorante_scelto(Ristorante *ristorante_scelto,llist *list_restaurant);  /*Controlla che il ristorante scelto sia nella lista */

//FUNZIONI THREADS
void *thread_i_rider(void *client);                                            /* Funzione thread per la gestione dei riders connessi */
void *thread_i_restaurant(void *client);                                       /* Funzione thread per la gestione dei ristoranti connessi */
void *thread_i_client(void *client);                                           /* Funzione thread per la gestione dei client connessi */

void numprint(void *num);

/* Main */
int main(int argc, char *argv[]){
   
   /* inizializzazione variabili */
   int i, current;
   
   /* variabili server */
   int server_socket, *client_socket_i;
  
   /* variabili threads */
   pthread_t *thread_i;

   /* inizializzazione semafori(mutex) shared_client1 (Ristoranti)*/
   sem_init(&S1.mutex_queue1, 0, 1);
   /* inizializzazione semafori(mutex) shared_client2 (Client)*/
   sem_init(&S2.mutex_queue2, 0, 1);
    /* inizializzazione semafori(mutex) shared_client2 (Riders)*/
   sem_init(&S3.mutex_queue3, 0, 1);
   /* inizializzazione semafori(mutex) shared_glob (Globali)*/
   sem_init(&SG.mutex_list_restaurant, 0, 1);
    
   /* inizializzo la lista per le informazioni dei ristoranti */
   SG.list_restaurant = llist_create(NULL);
    
   S2.num_client = 0;
   S1.num_restaurant = 0;
   /* inizializzo le Code per socket (creo una coda per ogni tipologia di client) */
   S1.queue_client1 = createQueue(sizeof(int *));   //Coda di ristoranti(client)
   S2.queue_client2 = createQueue(sizeof(int *));   //Coda di client 
   S3.queue_client3 = createQueue(sizeof(int *));   //Coda di riders(client)
  
   /* inizializza server tcp */
   server_socket = setup_server_tcp(SERVERPORT,SERVER_BACKLOG);
   
   while(1){
       printf("Server Delivery food in attesa di connessioni...\n\n");  
      /* Accetta una nuova connessione */
      client_socket_i = (int *)malloc(sizeof(int));
      *client_socket_i = accept_new_connection(server_socket); 
      int client_socket = *client_socket_i;
      free(client_socket_i);

      /* Riceve un intero che identifica la tipologia di client che si vuole connettere al server */
      char ricevline_info_client[1];
      FullRead(client_socket,ricevline_info_client,sizeof(ricevline_info_client));
      int tipo_client = atoi(ricevline_info_client);
    
      /* Creo un nuovo thread per ogni connessione accettata */
      thread_i = (pthread_t *)malloc(sizeof(pthread_t));
      
      if(tipo_client == RISTORANTE_ID){
         /* inserisco nella coda di ristoranti un nuovo ristorante */
         sem_wait(&S1.mutex_queue1);
         enqueue(S1.queue_client1,(int *) &client_socket);
         sem_post(&S1.mutex_queue1);
         
         /* creo e lancio un thread per gestire la connessione da parte del ristorante */
         pthread_create(thread_i, NULL,(void *)thread_i_restaurant, &client_socket);
      }
      else if (tipo_client == CLIENT_ID){
         /* inserisco nella coda dei client un nuovo client*/
         sem_wait(&S2.mutex_queue2);
         enqueue(S2.queue_client2,(int *) &client_socket);
         sem_post(&S2.mutex_queue2);
        
         /* creo e lancio un thread per gestire la connessione da parte del client */
         pthread_create(thread_i, NULL,(void *)thread_i_client, &client_socket);    
      }
       else if (tipo_client == RIDER_ID){
         /* inserisco nella coda dei client un nuovo client*/
         sem_wait(&S3.mutex_queue3);
         enqueue(S3.queue_client3,(int *) &client_socket);
         sem_post(&S3.mutex_queue3);
        
         /* creo e lancio un thread per gestire la connessione da parte del client */
         pthread_create(thread_i, NULL,(void *)thread_i_rider, &client_socket);    
         
      }
      
   } 
   /* Deallocazione semafori */
   sem_destroy(&S1.mutex_queue1);
   sem_destroy(&S2.mutex_queue2);
   sem_destroy(&S3.mutex_queue3);
   sem_destroy(&SG.mutex_list_restaurant);
   
   return 0;
} 

//---------------------------------------------FUNZIONI SERVER---------------------------------------------
void close_server(){
   exit(0); 
}

int setup_server_tcp(short port, int backlog){
   int server_socket;
   int enable = 1;
   SA_IN servaddr;
   
   server_socket = Socket(AF_INET, SOCK_STREAM, 0);
   
   //inizializzazione indirizzo server
   bzero(&servaddr, sizeof(servaddr));
   servaddr.sin_family = AF_INET;
   servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
   servaddr.sin_port = htons(port);
   
   signal(SIGTSTP,close_server);
   
   Setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
   
   Bind(server_socket, (SA *) &servaddr, sizeof(servaddr));
   
   Listen(server_socket, backlog);
   
   return server_socket;
}

int accept_new_connection(int server_socket){
   
   /* variabili connessione client*/
   int client_socket;
   SA_IN client_addr;
   SL len = sizeof(SA_IN);
      
   char buffer_info_client[256];
   
   client_socket = Accept(server_socket,(SA *) &client_addr,&len);
  
   Inet_ntop(AF_INET, &client_addr.sin_addr, buffer_info_client, sizeof(buffer_info_client));
   printf("Richiesta dall'host: %s, porta: %d\n",buffer_info_client, ntohs(client_addr.sin_port));
   
   return client_socket;
}

int connetti_tcp(char *server_addr, short port){
   
   int server_socket;
   SA_IN servaddr;
   char recevline_info_server[256];
   
   char sendline_server[1] = "0";
   
   server_socket = Socket(AF_INET, SOCK_STREAM, 0);
   
   servaddr.sin_family = AF_INET;
   servaddr.sin_port=htons(port);
	 
   Inet_pton(AF_INET,server_addr,&servaddr.sin_addr);
   
   int conn_value = Connect(server_socket,(SA *) &servaddr,sizeof(servaddr));
   if(conn_value == -1){
      return -1;
   }
   
   printf("\nclient/ristorante: %s port: %d\n",server_addr,port);
    
   //Invia una stringa booleana per far capire ad un ristorante che sta comunicando con un server 
   FullWrite(server_socket,sendline_server,sizeof(sendline_server));   
    
   //Legge informazioni dal server
   FullRead(server_socket, recevline_info_server, sizeof(recevline_info_server));
   printf("%s",recevline_info_server);
   
   return server_socket;
}
void notifica_nuovo_client(int client_socket,int tipo_client){
   
   /* variabili informazioni da inviare*/
   char buffer_info_client[256], tipo_client_string[50];
   time_t timeval = time (NULL);
   int num_client;  
  
   if(tipo_client == RISTORANTE_ID){
      strcpy(tipo_client_string,"ristorante");
      printf ("\n[%s] n° [%d]  da servire \r\n\n",tipo_client_string,S1.num_restaurant);
   
      snprintf(buffer_info_client, sizeof(buffer_info_client),"%.24s : Sei il [%s] numero: [%d]\r\n\n",ctime(&timeval),tipo_client_string,S1.num_restaurant);
      FullWrite(client_socket, buffer_info_client, sizeof(buffer_info_client));
   } 
   else if (tipo_client == CLIENT_ID){
      strcpy(tipo_client_string,"client");
      
      printf ("\n[%s] n° [%d]  pronto per essere servito\r\n\n",tipo_client_string,S2.num_client);
   
      snprintf(buffer_info_client, sizeof(buffer_info_client),"%.24s : Sei il [%s] numero: [%d]\r\n\n",ctime(&timeval),tipo_client_string,S2.num_client);
      FullWrite(client_socket, buffer_info_client, sizeof(buffer_info_client));
   
   }
   else if (tipo_client == RIDER_ID){
      strcpy(tipo_client_string,"rider");
      
      printf ("\n[%s] n° [%d]  pronto per essere servito\r\n\n",tipo_client_string,S3.num_rider);
   
      snprintf(buffer_info_client, sizeof(buffer_info_client),"%.24s : Sei il [%s] numero: [%d]\r\n\n",ctime(&timeval),tipo_client_string,S3.num_rider);
      FullWrite(client_socket, buffer_info_client, sizeof(buffer_info_client));
   
   }      
}
//---------------------------------------------STAMPA INFORMAZIONI---------------------------------------------

void stampa_info_ristorante(Ristorante *ristorante){
   
   printf("------------------------------------ID RISTORANTE: %d------------------------------------\nNome Ristorante: %s\nPosizione: %s\nIP: %s\nPorta: %d\n\n^\n|",ristorante->id,ristorante->nome,ristorante->posizione,ristorante->ip,ristorante->porta); 
}

void numprint(void *num){
   Ristorante *pnum = (Ristorante *)num;
   if (num == NULL) 
      return;
   stampa_info_ristorante(pnum);
}

//---------------------------------------------FUNZIONI CONTROLLO DATI INVIATI DA CLIENT---------------------------------------------

Ristorante *controlla_presenza_ristorante_scelto(Ristorante *ristorante_scelto,llist *list_restaurant){
   
   struct node *curr = *list_restaurant;
   
   while(curr != NULL){
      
      if(curr->data != NULL){  
         
         Ristorante *ristorante = curr->data;
         if(ristorante->id == ristorante_scelto->id && strcmp(ristorante->nome,ristorante_scelto->nome) == 0 && strcmp(ristorante->posizione,ristorante_scelto->posizione) == 0){
            return ristorante;
         }
      }
      curr = curr->next;
   }
   
   return NULL;  
}
//---------------------------------------------FUNZIONI PER INVIARE O RICEVERE DATI---------------------------------------------

//@-------------------------------------------INVIA RISTORANTE/I-------------------------------------------

void invia_info_ristorante(int socket,Ristorante *ristoranteInput, int typeClient){
   char id_invio[10];
   char porta[128];    
   sprintf(id_invio,"%d",ristoranteInput->id);
    
   /* invio le informazioni del ristorante */
   FullWrite(socket,ristoranteInput->nome, sizeof(ristoranteInput->nome));
   FullWrite(socket,ristoranteInput->posizione, sizeof(ristoranteInput->posizione));
   FullWrite(socket,id_invio,sizeof(id_invio));
   
   /* Se il client e' un rider, gli invia anche IP e Porta del ristorante */
   if(typeClient == RIDER_ID){
      FullWrite(socket,ristoranteInput->ip,sizeof(ristoranteInput->ip));
      
      snprintf(porta, 128, "%u", htons(ristoranteInput->porta));
      FullWrite(socket,porta,sizeof(porta));
   }
}
void invia_lista_ristoranti(int socket,llist *list_restaurant,int typeClient){
   
   char send_size[100];
   int sizeList = llist_getSize(list_restaurant);

   sprintf(send_size,"%d",sizeList);
   
   /* Invia la dimensione della lista da inviare così che il client sappia il numero di byte da leggere */
   FullWrite(socket,send_size,sizeof(send_size));
   
   if(sizeList != 0){
      /* In base alla dimensione della lista verranno inviati sizeList ristoranti (es: sizeList = 2 = 2 chiamate a "invia_info_ristorante_a_client" )*/
      struct node *curr = *list_restaurant;
      
      for(int i = 0; i < sizeList || curr != NULL; i++){
         invia_info_ristorante(socket, curr->data, typeClient);
         curr = curr->next;
      }
   } 
}
//@-------------------------------------------RICEVI RISTORANTE/I-------------------------------------------
Ristorante *ricevi_info_ristorante(int socket, int typeClient){
   
   /* Alloco un nuovo ristorante */
   Ristorante *ristorante_ricevuto = (Ristorante *)malloc(sizeof(Ristorante));
     
   char nome_ristorante[MAX_NOME_RISTORANTE];
   char posizione_ristorante[MAX_POSIZIONE_RISTORANTE];
   char id_lettura[10];
   char porta[128]; 
   
   /* Riceve informazioni dal ristorante da memorizzare nella lista */
   FullRead(socket,nome_ristorante, sizeof(nome_ristorante));
   FullRead(socket,posizione_ristorante, sizeof(posizione_ristorante));
   FullRead(socket,id_lettura,sizeof(id_lettura));
   
   if(typeClient == RISTORANTE_ID){
      FullRead(socket,ristorante_ricevuto->ip,sizeof(ristorante_ricevuto->ip));
      FullRead(socket,porta,sizeof(porta));
      
      ristorante_ricevuto->porta =ntohs(atoi(porta));
   }
   
   strcpy(ristorante_ricevuto->nome,nome_ristorante);
   strcpy(ristorante_ricevuto->posizione,posizione_ristorante); 
   ristorante_ricevuto->id = atoi(id_lettura);

   
   return ristorante_ricevuto;
}

//---------------------------------------------THREADS---------------------------------------------


//THREADS RIDERS
void *thread_i_rider(void *client){
   
   printf("-------------------------------------------------------------------------------\n");
   int id_rider;
   int *client_thread = (int *)client;

   /* Alloco variabile per inserirci un nuovo socket */
   int *pclient = (int *)malloc(sizeof(int));
   
   /* Estrae un rider(client) dalla coda in mutua esclusione */
   sem_wait(&S3.mutex_queue3);
   if(!isEmpty(S3.queue_client3)){
      dequeue(S3.queue_client3, pclient);
      S3.num_rider++;
      id_rider = S3.num_rider;
      notifica_nuovo_client(*pclient, RIDER_ID); 
   }
   sem_post(&S3.mutex_queue3);
   
   if(pclient == NULL){
      pthread_exit(client_thread);
   }
     
   
   /* Invia la lista di ristoranti attuale al rider in mutua esclusione */
   sem_wait(&SG.mutex_list_restaurant);
   invia_lista_ristoranti(*pclient, SG.list_restaurant,RIDER_ID);
   sem_post(&SG.mutex_list_restaurant);
   printf(">> Lista di ristoranti inviata al rider [%d]\n\n",id_rider);
  
   printf("rider %d servito\n",id_rider);
   printf("-------------------------------------------------------------------------------\n\n");
   
   /* Il thread rider termina */
   pthread_exit(client_thread);
}

//THREADS RISTORANTI
void *thread_i_restaurant(void *client){
   
   //printf("-------------------------------------------------------------------------------\n");
   int id_restaurant;
   int *client_thread = (int *)client;

   /* Alloco variabile per inserirci un nuovo socket */
   int *pclient = (int *)malloc(sizeof(int));
   
   /* Estrae un ristorante(client) dalla coda in mutua esclusione */
   sem_wait(&S1.mutex_queue1);
   if(!isEmpty(S1.queue_client1)){
      dequeue(S1.queue_client1, pclient);
      S1.num_restaurant++;
      id_restaurant = S1.num_restaurant;
      notifica_nuovo_client(*pclient, RISTORANTE_ID);  
   }
   sem_post(&S1.mutex_queue1);
   
   if(pclient == NULL){
      pthread_exit(client_thread);
   }
     
   /* Ricevo le informazioni dal ristorante che ha contattato il server */
   Ristorante *nuovoRistorante = ricevi_info_ristorante(*pclient,RISTORANTE_ID);
   
   //Inserisco le informazioni del nuovo ristorante nella lista in mutua esclusione per evitare letture sporche dai client
   sem_wait(&SG.mutex_list_restaurant);
   llist_push(SG.list_restaurant, nuovoRistorante);
   //llist_print(SG.list_restaurant,numprint);
   sem_post(&SG.mutex_list_restaurant);
   printf(">> Ristorante [%s] di posizione [%s] aggiunto al server\n\n",nuovoRistorante->nome,nuovoRistorante->posizione);
   
  
   printf("ristorante %d servito\n",id_restaurant);
   //printf("-------------------------------------------------------------------------------\n\n");
   
   /* Il thread ristorante termina */
   pthread_exit(client_thread);
}

//THREAD CLIENT
void *thread_i_client(void *client){
   
   int id_client;
   int *client_thread = (int *)client;

   /* Alloco variabile per inserirci un nuovo socket */
   int *pclient = (int *)malloc(sizeof(int));
   /* Estrae un client dalla coda in mutua esclusione */
   sem_wait(&S2.mutex_queue2);
   if(!isEmpty(S2.queue_client2)){
      dequeue(S2.queue_client2, pclient);
      S2.num_client++;
      id_client = S2.num_client;
   }
   
   sem_post(&S2.mutex_queue2);
    
   if(pclient == NULL){
      pthread_exit(client_thread);
   }
   notifica_nuovo_client(*pclient, CLIENT_ID); 
   
   /* Invia la lista di ristoranti attuale al client in mutua esclusione */
   sem_wait(&SG.mutex_list_restaurant);
   invia_lista_ristoranti(*pclient, SG.list_restaurant,CLIENT_ID);
   sem_post(&SG.mutex_list_restaurant);
   
   printf("\nIn attesa che il client scelga un ristorante...\n");
   
   /* Riceve le informazioni del ristorante scelto dal client */   
   Ristorante *ristorante_scelto = ricevi_info_ristorante(*pclient,CLIENT_ID); 
  
   /* Controlla che il ristorante scelto dall'utente sia presente nella lista dei ristoranti disponibili in mutua esclusione */
   sem_wait(&SG.mutex_list_restaurant);
   Ristorante *match_ristorante = controlla_presenza_ristorante_scelto(ristorante_scelto, SG.list_restaurant);
   sem_post(&SG.mutex_list_restaurant);  
   
   
   /* Se il ristorante non è più presente nella lista (non più attivo per ricevere richieste), invio al client l'esito negativo (n) */
   char esito_richiesta[1];
   if(match_ristorante == NULL){
      printf("nessun ristorante disponibile con questi dati\n\n");
      esito_richiesta[0] = 'n';
      FullWrite(*pclient,esito_richiesta,sizeof(esito_richiesta));
      close(*pclient);
      printf("client %d servito\n",id_client);
      pthread_exit(client_thread);
   }

   esito_richiesta[0] = 's';
   FullWrite(*pclient,esito_richiesta,sizeof(esito_richiesta));
   
   stampa_info_ristorante(match_ristorante);
   printf("\nMi sto connettendo a: %s all'indirizzo %s e porta %d...\n",match_ristorante->nome,match_ristorante->ip,match_ristorante->porta);
    
   /* Il server si connette al ristorante richiesto dall'utente se disponibile */
   int socket_ristorante = connetti_tcp(match_ristorante->ip,match_ristorante->porta);

   /* Se il ristorante non è piu' disponibile perchè chiuso, invia un esito negativo al client e chiude la connessione attuale */
   if(socket_ristorante == -1){
       printf("Connessione con [%s] fallita\n",match_ristorante->nome);
       esito_richiesta[0] = 'n';
       FullWrite(*pclient,esito_richiesta, sizeof(esito_richiesta));
       /* Chiusura della connessione con il client (socket client) */
       close(*pclient);
       pthread_exit(client_thread);
   }
   esito_richiesta[0] = 's';
   
   FullWrite(*pclient,esito_richiesta, sizeof(esito_richiesta));
    
   /* Riceve il menu' dal ristorante contattato */ 
   printf("\nIn attesa che il ristorante invii il menu'...\n");
   queue *menu_ristorante_ricevuto = ricevi_ordinazioni(socket_ristorante);
   
   /* Controllo per verificare che non ci siano errori nel menu' ricevuto */
   if(menu_ristorante_ricevuto == NULL){
      printf("Menu' non pervenuto\n\n");
      /* Chiusura della connessione con il client (socket client) */
      close(*pclient);
      /* Chiusura della connessione con il ristorante (socket ristorante) */
      close(socket_ristorante);
      printf("client [%d] servito\n\n",id_client);
      pthread_exit(client_thread);
   }
   printf(">> Menu ricevuto da [%s]\n",match_ristorante->nome);
   
   /* Invia il menu' appena ricevuto dal ristorante al client */
   printf(">> Menu' inviato al client\n");
   invia_ordinazioni(*pclient,menu_ristorante_ricevuto);
  
   /* Attende la creazione e l' invio del nuovo ordine da parte dell'utente */
   printf("\nIn attesa che l' utente invii l'ordine...\n");
   Ordine *nuovoOrdine_utente = ricevi_ordine(*pclient);
   printf(">> Nuovo ordine ricevuto\n");
   /* Stampa l'ordine ricevuto */
   stampa_ordine_utente(nuovoOrdine_utente);
   
   /* Riceve le informazioni del client con l'id */
   Utente *nuovoUtente = ricevi_info_utente(*pclient);
   printf(">> Informazioni utente ricevute :\n");
   stampa_info_utente(nuovoUtente);
   
   /* Invia l'ordine appena ricevuto dal client al ristorante contattato */
   printf(">> Inoltro l'ordine al ristorante contattato\n");
   invia_ordine(socket_ristorante,nuovoOrdine_utente);
   
   /* Riceve l' ID del rider dal ristorante */
   Rider *rider_incaricato = ricevi_info_rider(socket_ristorante);
   printf(">> Il rider con ID: [%d] ,ricevuto dal ristorante [%s], e' pronto per la consegna\n",rider_incaricato->id,match_ristorante->nome);
   
   /* Invia l'ID del rider al client */
   invia_info_rider(*pclient,rider_incaricato);
   printf(">> informazioni rider [%d] inoltrate al client [%d]\n",rider_incaricato->id,nuovoUtente->id);
   
   /* Invia le informazioni del client con l'ID al ristorante contattato cosi' che lo possa inoltrare al rider */
   invia_info_utente(socket_ristorante, nuovoUtente);
   printf(">> Informazioni del client di ID: [%d] e Nome:[%s] inviate al ristorante [%s]\n",nuovoUtente->id,nuovoUtente->nome,match_ristorante->nome);
   
   /* Il server viene notificato della consegna completata dal ristorante */
   char consegna_effettuata[1];
   FullRead(socket_ristorante,consegna_effettuata,sizeof(consegna_effettuata));
   if(consegna_effettuata[0] == 's'){
      printf(">> Consegna effettuata al client di ID: [%d] e nome: [%s] all'indirizzo [%s]\ndal rider di ID: [%d] e nome: [%s]\n",nuovoUtente->id,nuovoUtente->nome,nuovoUtente->posizione,rider_incaricato->id,rider_incaricato->nome);
      consegna_effettuata[0] = 's';
      /* Invia un semplice notifica al client per concludere l'ordine */
      FullWrite(*pclient,consegna_effettuata,sizeof(consegna_effettuata));
   }
   
   /* Chiusura della connessione con il client (socket client) */
   close(*pclient);
   /* Chiusura della connessione con il ristorante (socket ristorante) */
   close(socket_ristorante);
   
   printf("client [%d] servito\n\n",id_client);
   //printf("-------------------------------------------------------------------------------\n\n");
   
   /* Il thread client termina */
   pthread_exit(client_thread);
}
