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

#define SERVER_IP "127.0.0.1"
#define SERVERPORT 8989
#define MAX_PORT 50
#define SERVER_BACKLOG 1000

#define SERVER_ID 0
#define RISTORANTE_ID 1
#define RIDER_ID 3

typedef struct sockaddr SA;          /* struttura sockaddr */
typedef struct sockaddr_in SA_IN;    /* struttura sockaddr_in */
typedef socklen_t SL;                /* struttura socklen_t */
typedef struct hostent HO;           /* struttura hostent */


//STRUTTURA RISTORANTE
struct ristorante{
   char nome[MAX_NOME_RISTORANTE];
   char posizione[MAX_POSIZIONE_RISTORANTE];
   char ip[NI_MAXHOST];
   short porta;
   int id;
   queue *queue_menu_ordinazioni;
};
typedef struct ristorante Ristorante;


//DATI CONDIVISI TRA I THREAD CHE GESTISCONO RICHIESTE DAI SERVER
struct shared_client0{
   int num_server;
   queue *queue_client0, *queue_menu_send;
   sem_t mutex_queue0, mutex_menu;  
}S0;

//DATI CONDIVISI TRA I THREAD CHE GESTISCONO RICHIESTE DAI RIDERS
struct shared_client3{
   int num_rider;
   queue *queue_client3;
   sem_t mutex_queue3;
}S3;

//DATI CONDIVISI GLOBALI TRA I THREADS
struct shared_glob{
   queue *queue_ordini;
   queue *queue_riders_connessi;
   queue *queue_client;
   sem_t mutex_queue_ordini, mutex_queue_riders, mutex_queue_client_id;
   sem_t sem_assegna_rider, sem_conferma, sem_attesa_id, sem_consegna_completata;
   
}SG; 

//FUNZIONI DI RETE
short* riempi_porte();                                                                 /* Riempie le porte disponibili */
int connetti_tcp(char *server_addr, short port);                                       /* Connette il client ristorante al server */
int setup_restaurant_server_tcp(short port, int backlog);                              /* Inizializzazione socket ristorante tcp */
int accept_new_connection(int server_restaurant_socket);                               /* Accetta una nuova connessione dal server */
void notifica_nuovo_client(int client_socket,int tipo_client);                         /* Notifica i nuovi client(server o client) pronti per essere serviti */
void close_server();                                                                   /* Chiusura pulita socket in caso di sigstop */

//FUNZIONI RISTORANTE
Ristorante * immetti_info_ristorante();                                                /* Vengono immesse le informazioni del ristorante */
Ristorante * carica_menu(Ristorante *ristoranteInput);                                 /* viene caricato il menu' del ristorante da file */
Ristorante * carica_menu_da_file(int file_menu,Ristorante *ristoranteInput);           /* Carica il menu' da file */ 

//FUNZIONI PER INVIARE O RICEVERE DATI

//@INVIA RISTORANTE
void invia_info_ristorante(int socket,Ristorante *ristoranteInput);                    /* Invia le informazioni del ristorante al server */

//FUNZIONI STAMPA RISTORANTE
void stampa_menu_ristorante(queue *menu);                                              /* Stampa il menu' del ristorante */
void stampa_info_ristorante(Ristorante *ristorante);                                   /* Stampa le informazioni del ristorante */

//FUNZIONI THREADS
void *thread_i_server(void *pclient);                                                  /* Funzione thread per la gestione dei server connessi */
void *thread_i_rider(void *pclient);                                                   /* Funzione thread per la gestione dei riders connessi */


/* Main */
int main(int argc, char *argv[]){

   /* inizializzazione variabili */
   int i;
   
   /* variabili server */
   int server_restaurant_socket = -1;    /* descrittore su cui aprire il socket server */
   int ristorante_socket;                /* descrittore per la connessione al server */
   int *client_socket_i;                 /* descrittore per gestire le connessioni da parte del 'server' o dai 'Riders' */         
   short *porte;                         /* lista di porte per aprire più ristoranti sullo stesso IP */
   
   /* Informazioni ristorante */
   Ristorante *nuovoRistorante;
 
   /* variabili threads */
   pthread_t *thread_i;
   
   /* Inizializzazione semafori(mutex) shared_client0 (Server)*/
   sem_init(&S0.mutex_queue0, 0, 1);            //Semaforo binario per l'accesso alla coda di descrittori (server socket)
   sem_init(&S0.mutex_menu, 0, 1);              //Semaforo binario per l'accesso al menu'
   /* Inizializzazione semafori(mutex) shared_client3 (Rider)*/
   sem_init(&S3.mutex_queue3, 0, 1);            //Semaforo binario per l'accesso alla coda di descrittori (riders socket)
   /* Inizializzazione semafori(mutex) shared_glob */
   sem_init(&SG.mutex_queue_ordini, 0, 1);      //Semaforo binario per l'accesso alla coda di ordini
   sem_init(&SG.mutex_queue_riders,0, 1);       //Semaforo binario per l'accesso alla coda di riders
   sem_init(&SG.mutex_queue_client_id, 0, 1);   //Semaforo binario per l'accesso alla coda di client
   
   /* Inizializzazione semafori(contatori) shared_glob */
   sem_init(&SG.sem_assegna_rider,0, 0);        //Semaforo contatore per lo sblocco dei rider quando un ordine e' disponibile
   sem_init(&SG.sem_conferma, 0, 0);            //Semaforo contatore per lo sblocco del thread che gestisce la connessione al server (conferma) 
   sem_init(&SG.sem_attesa_id, 0, 0);           //Semaforo contatore per lo sblocco del rider dopo aver ricevuto l'id del client ed aver inviato l'id rider al server
   sem_init(&SG.sem_consegna_completata, 0, 0); //Semaforo contatore per lo sblocco del thread che gestisce la connessione al server (ordine consegnato)

   /* Inizializzo le code per socket (creo una coda per ogni tipologia di client) */
   S0.queue_client0 = createQueue(sizeof(int *));           //Coda di server(client)
   S3.queue_client3 = createQueue(sizeof(int *));           //Coda di rider(client)
   
   SG.queue_ordini = createQueue(sizeof(Ordine *));         //Coda di ordini da far consegnare ai riders  
   SG.queue_client = createQueue(sizeof(Utente *));         //Coda di client_id da inviare ai riders
   SG.queue_riders_connessi = createQueue(sizeof(Rider *)); //Coda di riders connessi
   
   /* Inizializzo una coda globale(thread server) per inviare il menu */
   S0.queue_menu_send = createQueue(sizeof(Ordinazioni *));   //Coda di ordinazioni per il menu'

   
   srand(time(NULL));
     
   HO *data = detect_info_address(SERVER_IP);
   
   /* Riempie la lista di porte da 4000 fino a MAX_PORT */  
   porte = riempi_porte();
   short porta_temp;
  
   /* Caricamento dati ristorante */
   nuovoRistorante = immetti_info_ristorante();
   nuovoRistorante = carica_menu(nuovoRistorante); 
   strcpy(nuovoRistorante->ip, get_this_Info_host());
   
   
   /* Ciclo for che scorre le porte che vanno da 4000 fino a MAX_PORT così da permettere l'apertura di piu' ristoranti sullo stesso IP */
   for(i = 0; i < MAX_PORT && server_restaurant_socket == -1; i++){
      porta_temp = porte[i];
      
      /* Il ristorante diventa server e gli viene passata la porta libera appena trovata */
      server_restaurant_socket = setup_restaurant_server_tcp(porta_temp,SERVER_BACKLOG);
      
      if(server_restaurant_socket == -1){
         printf(" sulla porta %d\n",porta_temp);
      }
      else{
         nuovoRistorante->porta = porta_temp;
         /* stampa dati ristorante */
         stampa_info_ristorante(nuovoRistorante);
         
         /* Connette il ristorante al Server per la prima volta*/ 
         ristorante_socket = connetti_tcp(SERVER_IP,SERVERPORT);
         
         /* invia informazioni di connessione al server */
         invia_info_ristorante(ristorante_socket,nuovoRistorante);     
         
      }
   }
   if(ristorante_socket == -1){
      printf("Connessione al server fallita, nessuna porta disponibile, ritentare !\n");
      exit(0);
   }
   
   S0.queue_menu_send = copyQueue(nuovoRistorante->queue_menu_ordinazioni);
   while(1){
      printf("\n\nRistorante [%s] in attesa di connessioni sulla porta [%d] ...\n\n",nuovoRistorante->nome,porta_temp);  
      
      /* Accetta una nuova connessione */
      client_socket_i = (int *)malloc(sizeof(int));
      *client_socket_i = accept_new_connection(server_restaurant_socket); 
      int client_socket = *client_socket_i;
      free(client_socket_i);
           
      /* Riceve un intero che identifica la tipologia di client che si vuole connettere al ristorante */
      char ricevline_info_client[1];
      FullRead(client_socket,ricevline_info_client,sizeof(ricevline_info_client));
      int tipo_client = atoi(ricevline_info_client);
  
      /* Creo un nuovo thread per ogni connessione accettata */
      thread_i = (pthread_t *)malloc(sizeof(pthread_t));
      
      if(tipo_client == SERVER_ID){
         /* inserisco una nuova connessione nella coda di connessioni server */
         sem_wait(&S0.mutex_queue0);
         enqueue(S0.queue_client0,(int *) &client_socket);
         sem_post(&S0.mutex_queue0);
         
         
         /* creo e lancio un thread per gestire la connessione da parte del ristorante */
         pthread_create(thread_i, NULL,(void *)thread_i_server,&client_socket);
        
      }
      else if (tipo_client == RIDER_ID){
         /* inserisco nella coda dei Riders un nuovo Rider (client)*/
         sem_wait(&S3.mutex_queue3);
         enqueue(S3.queue_client3,(int *) &client_socket);
         sem_post(&S3.mutex_queue3);
         
         /* creo e lancio un thread per gestire la connessione da parte del client */
         pthread_create(thread_i, NULL,(void *)thread_i_rider,&client_socket);    
         
      } 
   } 
   
   /* Deallocazione semafori */
   sem_destroy(&S0.mutex_queue0);            
   sem_destroy(&S0.mutex_menu);              
   sem_destroy(&S3.mutex_queue3);            
   sem_destroy(&SG.mutex_queue_ordini);      
   sem_destroy(&SG.mutex_queue_riders);       
   sem_destroy(&SG.mutex_queue_client_id);   
   sem_destroy(&SG.sem_assegna_rider);        
   sem_destroy(&SG.sem_conferma);             
   sem_destroy(&SG.sem_attesa_id);           
   sem_destroy(&SG.sem_consegna_completata); 
   return 0;
   
}
//---------------------------------------------FUNZIONI DI RETE---------------------------------------------
short* riempi_porte(){
   
   short *porte_temp = (short *)malloc(MAX_PORT*sizeof(short));
   
   int i;
   for(i = 0; i < MAX_PORT; i++){
      porte_temp[i] = i + 4000;
   }  
   return porte_temp;
}

void close_server(){
   
   exit(0); 
}

int connetti_tcp(char *server_addr, short port){
   
   int ristorante_socket;
   SA_IN servaddr;
   char recevline_info_server[256];
   char sendline_ristorante[1] = "1";
   
   ristorante_socket = Socket(AF_INET, SOCK_STREAM, 0);
   
   servaddr.sin_family = AF_INET;
   servaddr.sin_port=htons(port);
	 
   Inet_pton(AF_INET,server_addr,&servaddr.sin_addr);
   
   Connect(ristorante_socket,(SA *) &servaddr,sizeof(servaddr));
   
   printf("\nserver: %s port: %d\n",server_addr,port);
   
   //Invia una stringa booleana per far capire al server che è un ristorante
   FullWrite(ristorante_socket,sendline_ristorante,sizeof(sendline_ristorante));   
   
   
   //Legge informazioni dal server
   FullRead(ristorante_socket, recevline_info_server, sizeof(recevline_info_server));
   printf("%s",recevline_info_server);
   
      
   return ristorante_socket;
}


int setup_restaurant_server_tcp(short port, int backlog){
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
   
   if(Bind(server_socket, (SA *) &servaddr, sizeof(servaddr)) != -1){
      Listen(server_socket, backlog);
   }
   else{
      printf("Bind error\n");
      return -1;
   }
   
   return server_socket;
}
void notifica_nuovo_client(int client_socket,int tipo_client){
   
   /* variabili informazioni da inviare*/
   char buffer_info_client[256], tipo_client_string[50];
   time_t timeval = time (NULL);
   int num_client;  
  
   if(tipo_client == SERVER_ID){
      strcpy(tipo_client_string,"server");
      printf ("\n[%s] n° [%d] pronto per essere servito\r\n\n",tipo_client_string,S0.num_server);
   
      snprintf(buffer_info_client, sizeof(buffer_info_client),"%.24s : Sei il [%s] numero: [%d]\r\n\n",ctime(&timeval),tipo_client_string,S0.num_server);
      FullWrite(client_socket, buffer_info_client, sizeof(buffer_info_client));
   } 
   else if(tipo_client == RIDER_ID){
      strcpy(tipo_client_string,"rider");
      
      printf ("\n[%s] n° [%d]  pronto per essere servito\r\n\n",tipo_client_string,S3.num_rider);
   
      snprintf(buffer_info_client, sizeof(buffer_info_client),"%.24s : Sei il [%s] numero: [%d]\r\n\n",ctime(&timeval),tipo_client_string,S3.num_rider);
      FullWrite(client_socket, buffer_info_client, sizeof(buffer_info_client));
   
   }   
}
int accept_new_connection(int server_restaurant_socket){
   
   /* variabili connessione client*/
   int client_socket;
   SA_IN client_addr;
   SL len = sizeof(SA_IN);   
 
   char buffer_info_client[256];
   
   
   client_socket = Accept(server_restaurant_socket,(SA *) &client_addr,&len);
  
   Inet_ntop(AF_INET, &client_addr.sin_addr, buffer_info_client, sizeof(buffer_info_client));
   printf("Richiesta dall'host: %s, porta: %d\n",buffer_info_client, ntohs(client_addr.sin_port));
   
   return client_socket;
}


//---------------------------------------------STAMPA INFORMAZIONI---------------------------------------------

void stampa_info_ristorante(Ristorante *ristorante){
    
   printf("------------------------------------ID: %d------------------------------------\nNome Ristorante: %s\nPosizione: %s\n\n",ristorante->id,ristorante->nome,ristorante->posizione);
       
   printf("MENU RISTORANTE\n\n");
   
   stampa_coda_ordinazioni(ristorante->queue_menu_ordinazioni);
   
}

//---------------------------------------------FUNZIONI CARICAMENTO INFORMAZIONI RISTORANTE---------------------------------------------


//@-------------------------------------------CARICAMENTO INFO RISTORANTE DA STANDARD INPUT-------------------------------------------
int generate_random_id(){
   
   srand(time(NULL));
   char numbers[]="0123456789";
   int id_restituito;
   char id[10];
   int index;
   for(int i = 0; i < 9; i++){

      index = rand()%9;
      id[i] = numbers[index];
   }
   if(strlen(id) > 10){
      id[10]=' ';
   }
   
   id_restituito = atoi(id);
   return id_restituito;
}
Ristorante * immetti_info_ristorante(){
   Ristorante *nuovoRistorante = (Ristorante *)malloc(sizeof(Ristorante));
   char nome[MAX_NOME_RISTORANTE]; 
   char posizione[MAX_POSIZIONE_RISTORANTE]; 
   
   printf("\n---------------INFORMAZIONI RISTORANTE---------------\n");
   
   /* Inserimento nome ristorante */
   do{    
      printf("Inserisci il nome del tuo ristorante: ");
      fflush(stdin);
      fgets(nome,MAX_NOME_RISTORANTE,stdin);    
        
      if(strlen(nome) <= 3){
         printf("\nInserire un nome di almeno 3 caratteri\n");
      }
      
      
   }while(isNumeric(nome) || nome == NULL  || strlen(nome) <= 3);
   
   remove_all_chars(nome,"\n");
   
   
   /* Inserimento posizione ristorante */
   do{
       
      printf("\nInserisci la posizione del tuo ristorante: ");
      fflush(stdin);
      fgets(posizione,MAX_POSIZIONE_RISTORANTE,stdin);    
        
      if(strlen(posizione) <= 6){
         printf("\nInserire una posizione di almeno 6 caratteri");
      }
      
   }while(isNumeric(posizione) || posizione == NULL  || strlen(posizione) <= 6);
   remove_all_chars(posizione,"\n");
   
   printf("\n***************FINE INFORMAZIONI RISTORANTE***************\n");
   
   /* copio le informazioni inserite da input + id nel ristorante creato */
   nuovoRistorante->id = generate_random_id(); 
   strcpy(nuovoRistorante->nome,nome);
   strcpy(nuovoRistorante->posizione,posizione);
   
    
   return nuovoRistorante;
}

//@-------------------------------------------CARICAMENTO MENU' DA FILE-------------------------------------------
Ristorante * carica_menu(Ristorante *ristoranteInput){
   
   printf("\n\n---------------MENU RISTORANTE---------------\n");
   
   int file_menu;
   char nome_fileMenu[30];
   char temp_clear;
   
   
   do{
      printf("\nInserisci il nome del file contenente il menu del ristorante: ");
      
      //scanf("%c",&temp_clear);  //variabile temporanea per resettare il buffer input
      fgets(nome_fileMenu,30,stdin);
      remove_all_chars(nome_fileMenu,"\n");
      file_menu = Open(nome_fileMenu,O_RDONLY,0777);
      
      if(file_menu != -1){
         printf("\nCarico menu da: %s\n",nome_fileMenu);
      } 
      
   }while(file_menu == -1);
   
   ristoranteInput = carica_menu_da_file(file_menu,ristoranteInput);
   
   printf("\n***************FINE MENU RISTORANTE***************\n");
   
   return ristoranteInput;
}

Ristorante * carica_menu_da_file(int file_menu, Ristorante *ristoranteInput){
   
   char *buffer_file;
   char **lista_ordinazioni;
   char **rowSplitted;              

   
   /* Viene inserito il contenuto del file in un buffer */
   buffer_file = leggi_da_file(file_menu);
   
   /* Rimuove eventuali \n dal buffer per permettere la corretta lettura */
   buffer_file = remove_all_chars(buffer_file,"\n");
   
   //printf("buffer:\n%s\n",buffer_file);
   
   /* Delimitiamo le righe del file(Ordinazioni) da ';' e le colonne da '@' (pietanza e costo)*/
   const char delim_row = ';'; 
   const char delim_col = '@';
   
   /* chiamo str_split per splittare il buffer_file per righe */
   lista_ordinazioni = str_split(buffer_file,delim_row);
   char **temp = lista_ordinazioni;   
   
   /* Inizializzo la coda di ordinazioni */
   ristoranteInput->queue_menu_ordinazioni = createQueue(sizeof(Ordinazioni *));
   
   //Per ogni riga splittata per ';', vengono splittate due stringhe per '@' (nome pietanza e costo) 
   while(*temp != NULL){
     
      rowSplitted = str_split(*temp,delim_col);    
       
      double temp_costo = atof(rowSplitted[1]);
      
      if(strcmp(rowSplitted[0],"") != 0 && temp_costo > 0){ 
      
         Ordinazioni *nuovaOrdinazione = (Ordinazioni *)malloc(sizeof(Ordinazioni));
         strcpy(nuovaOrdinazione->nome_cibo_o_bevanda,rowSplitted[0]);
         nuovaOrdinazione->costo = temp_costo;
         
         /* Inserisco la nuova ordinazione caricata da file in una coda del ristorante appena creato */
         enqueue(ristoranteInput->queue_menu_ordinazioni, (Ordinazioni *) &nuovaOrdinazione);
      }
      else{
         printf("piatto: %s costo: %.2f non puo' essere caricato\n", rowSplitted[0], temp_costo);
      }
      *temp++;
   }
   
   return ristoranteInput;
}

//---------------------------------------------FUNZIONI PER INVIARE O RICEVERE DATI---------------------------------------------

//@-------------------------------------------INVIA RISTORANTE/I-------------------------------------------
void invia_info_ristorante(int socket,Ristorante *ristoranteInput){
   char id_invio[10];
   char porta[128];    
   sprintf(id_invio,"%d",ristoranteInput->id);
    
   /* invio le informazioni del ristorante */
   FullWrite(socket,ristoranteInput->nome, sizeof(ristoranteInput->nome));
   FullWrite(socket,ristoranteInput->posizione, sizeof(ristoranteInput->posizione));
   FullWrite(socket,id_invio,sizeof(id_invio));
   FullWrite(socket,ristoranteInput->ip,sizeof(ristoranteInput->ip));
   
 
   snprintf(porta, 128, "%u", htons(ristoranteInput->porta));
   FullWrite(socket,porta,sizeof(porta));
  
}
    
//---------------------------------------------THREADS---------------------------------------------

//THREADS SERVER
void *thread_i_server(void *client){
//printf("-------------------------------------------------------------------------------\n");
   
   int id_server;
   int *client_thread = (int *)client;
 
   /* Alloco variabile per inserirci un nuovo socket */
   int *pclient = (int *)malloc(sizeof(int));
   
   /* Estrae un server(client) dalla coda in mutua esclusione */
   sem_wait(&S0.mutex_queue0);
   
   if(!isEmpty(S0.queue_client0)){
      dequeue(S0.queue_client0, pclient);
      S0.num_server++;
      id_server = S0.num_server;
      notifica_nuovo_client(*pclient, SERVER_ID); 
   }
   sem_post(&S0.mutex_queue0);
   
   if(pclient == NULL){
      pthread_exit(client_thread);
   } 
   
   /* Invia il menu al server così che possa a sua volta inviarlo al client */ 
   sem_wait(&S0.mutex_menu);
   invia_ordinazioni(*pclient, S0.queue_menu_send);
   sem_post(&S0.mutex_menu);
   printf(">> Menu' inviato al server\n\nIn attesa di un ordine utente dal server...\n");
   
   /* Riceve, appena disponibile, un nuovo ordine utente dal server */
   Ordine *nuovoOrdine_utente = ricevi_ordine(*pclient);
   printf(">> Nuovo ordine ricevuto, invio richiesta ai riders connessi...\n\n");
   
   /* Inserisce il nuovo ordine in una coda di ordini */
   sem_wait(&SG.mutex_queue_ordini);
   enqueue(SG.queue_ordini,(Ordine *) &nuovoOrdine_utente);
   
   /* Appena è disponibile un nuovo ordine ,sblocca il primo rider libero*/
   sem_post(&SG.sem_assegna_rider);
   sem_post(&SG.mutex_queue_ordini);
   
   /* Attende la conferma da parte del primo rider pronto ad effettuare la consegna */
   sem_wait(&SG.sem_conferma);
   
  
   /* Ottenuta la conferma, estrae dalla coda le informazioni del rider in mutua esclusione */
   Rider *rider_pronto;
   sem_wait(&SG.mutex_queue_riders);
   if(!isEmpty(SG.queue_riders_connessi)){
      dequeue(SG.queue_riders_connessi,&rider_pronto);
   }
   sem_post(&SG.mutex_queue_riders);
   
   
   /* Invia l' il rider con ID pronto per effettuare la consegna */
   invia_info_rider(*pclient,rider_pronto);
   printf(">> ID rider : [%d] inviato al server\n",rider_pronto->id);
   
 
   /* Riceve le informazioni del client con l'id dal server */
   Utente *nuovoUtente = ricevi_info_utente(*pclient);
   stampa_info_utente(nuovoUtente);
   printf(">> Il client [%d] sta per ricevere la sua ordinazione dal rider di ID: [%d] a [%s]\n\n",nuovoUtente->id,rider_pronto->id,nuovoUtente->posizione);
   
   /* Inserisco il client id in una coda in mutua esclusione */
   sem_wait(&SG.mutex_queue_client_id);
   enqueue(SG.queue_client,(Utente *) &nuovoUtente);
   sem_post(&SG.mutex_queue_client_id);
   
   /* Appena ottiene dal server l'id del client */
   sem_post(&SG.sem_attesa_id);   
   
   /* Attende che il rider notifichi il ristorante della consegna completata */
   sem_wait(&SG.sem_consegna_completata);
   
   /* Notifica il server della consegna completata */
   char consegna_effettuata[1];
   consegna_effettuata[0] = 's';
   FullWrite(*pclient,consegna_effettuata,sizeof(consegna_effettuata));
   
   /* Chiusura della connessione con il server (socket server) */
   close(*pclient);
   printf("\nserver %d servito\n",id_server);
   //printf("-------------------------------------------------------------------------------\n\n");
  
   pthread_exit(client_thread);
}

//THREADS RIDERS
void *thread_i_rider(void *client){
//printf("-------------------------------------------------------------------------------\n");
   
   int id_rider;
   int *client_thread = (int *)client;
   
   /* Alloco variabile per inserirci un nuovo socket */
   int *pclient = (int *)malloc(sizeof(int));
   
   /* Estrae un rider(client) dalla coda in mutua esclusione */
   sem_wait(&S3.mutex_queue3);
   /* Se la coda ha almeno un descrittore */
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
   /* Riceve le informazioni dal nuovo rider connesso */
   Rider *nuovoRider_connesso = ricevi_info_rider(*pclient);
   stampa_info_rider(nuovoRider_connesso);    
   printf("In attesa di una nuova richiesta...\n");
   
   /* Il thread rider rimane bloccato fin quando non viene attivato con una sem_post da thread_i_server (una nuova richiesta) */
   sem_wait(&SG.sem_assegna_rider);
   
   printf(">> Il rider %s effettuera' la consegna\n",nuovoRider_connesso->nome);
   
   Ordine *ordine_assegnato = (Ordine *)malloc(sizeof(Ordine)); 
   /* Estrae un ordine dalla coda in mutua esclusione e se ne prende carico */
   sem_wait(&SG.mutex_queue_ordini);
   if(!isEmpty(SG.queue_ordini)){
      dequeue(SG.queue_ordini,&ordine_assegnato);
   }
   sem_post(&SG.mutex_queue_ordini);
   

   /* Invia l' ordine estratto al rider(client) per poi consegnarlo */
   printf(">> Nuovo ordine disponibile per il rider [%s]\n",nuovoRider_connesso->nome);
   invia_ordine(*pclient,ordine_assegnato);
   
   /* Rende disponibili le proprie informazioni al ristorante */
   sem_wait(&SG.mutex_queue_riders);
   enqueue(SG.queue_riders_connessi,(Rider *)&nuovoRider_connesso);   
   sem_post(&SG.mutex_queue_riders);
   
   /* invia la conferma al ristorante */
   sem_post(&SG.sem_conferma);  
   printf(">> Conferma effettuata dal rider [%s]\n",nuovoRider_connesso->nome);
   /* log di conferma sul client rider */
   char conferma[1] = "s";
   FullWrite(*pclient,conferma,sizeof(conferma));
   
   /* Attende che gli arrivi il client id dal server */
   printf("\nIn attesa che il server invii l'id del client al rider...\n");
   sem_wait(&SG.sem_attesa_id);
   
   /* Estraggo l'utente con client ID (inviato dal server) da una coda in mutua esclusione */
   Utente *nuovoUtente;
   sem_wait(&SG.mutex_queue_client_id);
   dequeue(SG.queue_client, &nuovoUtente);
   sem_post(&SG.mutex_queue_client_id);
   
   /* Invia al client rider l' utente con client id che ha richiesto l'ordine */
   invia_info_utente(*pclient,nuovoUtente);
   printf(">> Il rider [%s] ha ricevuto il client id [%d]\n",nuovoRider_connesso->nome,nuovoUtente->id);
   
   /* Attende la segnalazione da parte del rider(client) che la consegna e' stata effettuata con successo */
   char consegna[1];
   FullRead(*pclient,consegna,sizeof(consegna)); 
   if(consegna[0] == 's'){
      printf(">> Consegna effettuata al client id: [%d] che risiede in [%s], arrivederci\n\n",nuovoUtente->id,nuovoUtente->posizione); 
      sem_post(&SG.sem_consegna_completata);
   }
   
   /* Chiusura della connessione con il rider (socket rider)*/
   close(*pclient);
   
   printf("\nrider %d servito\n",id_rider);
   //printf("-------------------------------------------------------------------------------\n\n");
  
   pthread_exit(client_thread);
}
