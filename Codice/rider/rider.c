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
#include"wrapperSysCall.h"
#include"abstract_user.h"
#include"wrapperSocket.h"
#include"wrapperString.h"
#include"queue.h"
#include"llist.h"

#define SERVER_IP "127.0.0.1"
#define SERVERPORT 8989

#define SERVER_ID 0
#define RISTORANTE_ID 1
#define RIDER_ID 3


typedef struct sockaddr SA;          /* struttura sockaddr */
typedef struct sockaddr_in SA_IN;    /* struttura sockaddr_in */
typedef struct socklen_t SL;         /* struttura socklen_t */
typedef struct hostent HO;           /* struttura hostent */


//STRUTTURA RISTORANTE
struct ristorante{
   int id_scelta;
   int id;
   char nome[MAX_NOME_RISTORANTE];
   char posizione[MAX_POSIZIONE_RISTORANTE];
   char ip[NI_MAXHOST];
   short porta;
};
typedef struct ristorante Ristorante;

//FUNZIONI DI RETE
HO *detect_info_address(char *server_addr);                                      /* rileva informazioni sul server */
int connetti_tcp(char *server_addr, short port);                                 /* Connette il client al server */
void close_rider();                                                              /* Chiusura pulita socket in caso di sigstop */

//FUNZIONI PER INVIARE/RICEVERE DATI

//@INVIA RISTORANTE
void invia_info_ristorante_scelto(int socket, Ristorante *ristorante_scelto);    /* Invia le informazioni del ristorante scelto dall'utente */

//@RICEVI RISTORANTE
Ristorante *ricevi_info_ristorante_da_server(int socket);                        /* Chiamato sizeList volte dalla funzione 'ricevi_lista_ristoranti' */
llist *ricevi_lista_ristoranti(int socket);                                      /* Riceve la lista di ristoranti dal server per effettuarne la scelta */

//FUNZIONI MENU'
Rider * immetti_info_rider();                                                    /* Immetti le informazioni del rider */
Ristorante* controlla_scelta(int scelta_utente,llist *list_restaurant);          /* Controlla che l'id scelto dall'utente sia corretto */
Ristorante *scegli_ristorante(llist *list_restaurant);                           /* Gestisce il menu' per la scelta del ristorante */

//FUNZIONI DI STAMPA 
void stampa_info_ristorante(Ristorante *ristorante);                             /* Stampa le informazioni dei ristoranti ricevuti dal server */
void stampa_menu_ristorante(queue *menu);                                        /* Stampa il menu' del ristorante ricevuto dal server */

/* main */
int main (int argc, char *argv[]){
   
   /* Inizializzazione variabili */
   int i, isConnesso = 1;
   
   /* Variabili client */
   int client_socket;
   
   /* Ristorante da scegliere */
   Ristorante *ristorante_scelto;
   
   /* Inizializzo la lista dei ristoranti da far riempire al server */
   llist *list_restaurant = llist_create(NULL);
   
   srand(time(NULL));
   
   
   HO *data = detect_info_address(SERVER_IP);
   
   
   /* Immette le informazioni del rider da standard input */
   Rider *nuovoRider = immetti_info_rider();
   signal(SIGTSTP,close_rider);
   /* Stampa le informazioni del rider */
   stampa_info_rider(nuovoRider);
   
   while(isConnesso){
      
      /* Connette il client al server */
      client_socket = connetti_tcp(SERVER_IP,SERVERPORT); 
      
      /* Riceve la lista di ristoranti dal server */
      list_restaurant = ricevi_lista_ristoranti(client_socket);
   
      /* Se la lista di ristoranti non è vuota */
      if(list_restaurant != NULL && llist_getSize(list_restaurant) != 0){
         /* Fa scegliere un ristorante all'utente */
         ristorante_scelto = scegli_ristorante(list_restaurant);
         /* Se la scelta è non nulla */
         if(ristorante_scelto == NULL){
            printf("Ristorante non piu' disponibile\n\n");
            //exit(0);
         }
         int socket_rider = connetti_tcp(ristorante_scelto->ip,ristorante_scelto->porta);   
         if(socket_rider == -1){
            printf("Connessione con [%s] fallita\n",ristorante_scelto->nome);
            //exit(1);
         }
         invia_info_rider(socket_rider,nuovoRider);
      
         /* Riceve l'ordine da consegnare */
         Ordine *nuovoOrdine = ricevi_ordine(socket_rider);
         
         if(nuovoOrdine == NULL){
            printf("Ordine non valido\n\n");
            //exit(0);
            break;
         }
         /* Stampa l'ordine da consegnare */
         stampa_ordine_utente(nuovoOrdine);  
         printf("Ordine ricevuto\nIn attesa di confermare un nuovo ordine\n");
         char conferma[1];
         FullRead(socket_rider,conferma,sizeof(conferma));
         
         if(conferma[0] == 's'){
         
            printf("Ordine confermato, attendo il client ID che ha richiesto l'ordine...\n");
            /* Riceve le informazioni dell'utente con l'id  che ha richiesto il nuovo ordine */
            Utente *nuovoUtente = ricevi_info_utente(socket_rider);
            printf("Riepilogo ordine\n\n");
            stampa_info_utente(nuovoUtente);
            stampa_ordine_utente(nuovoOrdine);
            printf("Consegna in corso al client id: [%d] che risiede in [%s]...\n\n",nuovoUtente->id,nuovoUtente->posizione);
               
            /* for per simulare il tempo di consegna del rider */
            char consegna_effettuata = ' ';
            //int tempo_impiegato = rand()%5; 
            //sleep(tempo_impiegato);
            while(consegna_effettuata != 's'){
               printf("Premi 's' per confermare la consegna: ");
               scanf(" %c",&consegna_effettuata);
               if(consegna_effettuata == 's'){
                  printf("Consegna effettuata al client id: [%d] che risiede in [%s], arrivederci\n\n",nuovoUtente->id,nuovoUtente->posizione);
                  char consegna[1];
                  consegna[0] = 's';
                  FullWrite(socket_rider,consegna,sizeof(consegna));
               }
            }
            consegna_effettuata=' ';
         }
        
         close(socket_rider);    
      }
      else{
         printf("Nessun ristorante disponibile\n");
         //exit(0);
      }
      /* Opzione per far effettuare al rider un' altra consegna */
      printf("Rimanere connessi ? s/n\n");
      char nuova_consegna;
      fflush(stdin);
      scanf(" %c",&nuova_consegna);
      if(nuova_consegna != 's'){
         isConnesso = 0;
      }
   }
   
   return 0;
} 

//---------------------------------------------FUNZIONI RETE---------------------------------------------
int connetti_tcp(char *server_addr, short port){
   
   int client_socket;
   SA_IN servaddr;
   char recevline_info_server[256];
   char sendline_client[1] = "3";
   
   client_socket = Socket(AF_INET, SOCK_STREAM, 0);
   
   servaddr.sin_family = AF_INET;
   servaddr.sin_port=htons(port);
	 
   Inet_pton(AF_INET,server_addr,&servaddr.sin_addr);
   
   Connect(client_socket,(SA *) &servaddr,sizeof(servaddr));
   
   printf("\nserver: %s port: %d\n",server_addr,port);
   
   //Invia una stringa booleana per far capire al server che è un client
   FullWrite(client_socket,sendline_client,sizeof(sendline_client)); 
   
   //Legge informazioni dal server
   FullRead(client_socket, recevline_info_server, sizeof(recevline_info_server));
   printf("%s",recevline_info_server);
   
    
   return client_socket;
}

void close_rider(){
   exit(0); 
}

//---------------------------------------------STAMPA INFORMAZIONI RISTORANTI---------------------------------------------

void stampa_info_ristorante(Ristorante *ristorante){
    
   printf("%d) ID: %d - Nome Ristorante: %s - Posizione: %s\n",ristorante->id_scelta, ristorante->id, ristorante->nome, ristorante->posizione);
  
}

//---------------------------------------------FUNZIONI CARICAMENTO INFORMAZIONI RIDER---------------------------------------------

//@-------------------------------------------CARICAMENTO INFORMAZIONI RIDER DA STANDARD INPUT-------------------------------------------
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
Rider * immetti_info_rider(){

   Rider *nuovoRider = (Rider *)malloc(sizeof(Rider));   
   char nome[MAX_NOME_RIDER]; 
   char cognome[MAX_COGNOME_RIDER]; 
   
   printf("\n---------------INFORMAZIONI RIDER---------------\n");
   
   /* Inserimento nome rider */
   do{    
      printf("Inserisci il tuo nome: ");
      fflush(stdin);
      fgets(nome,MAX_NOME_RIDER,stdin);    
        
      if(strlen(nome) <= 3){
         printf("\nInserire un nome di almeno 3 caratteri\n");
      }
      
      
   }while(isNumeric(nome) || nome == NULL  || strlen(nome) <= 3);
   
   remove_all_chars(nome,"\n");
   
   /* Inserimento cognome rider */
   do{
       
      printf("\nInserisci il tuo cognome: ");
      fflush(stdin);
      fgets(cognome,MAX_COGNOME_RIDER,stdin);    
        
      if(strlen(cognome) <= 3){
         printf("\nInserire un cognome di almeno 3 caratteri\n");
      }
      
   }while(isNumeric(cognome) || cognome == NULL  || strlen(cognome) <= 3);
   remove_all_chars(cognome,"\n");
   
   printf("\n***************FINE INFORMAZIONI RIDER***************\n");
   
   /* copio le informazioni inserite da input + id nel rider creato */
   nuovoRider->id = generate_random_id();
   strcpy(nuovoRider->nome,nome);
   strcpy(nuovoRider->cognome,cognome);
   
    
   return nuovoRider;
}
//---------------------------------------------FUNZIONI PER INVIARE O RICEVERE DATI---------------------------------------------
               
//@-------------------------------------------RICEVI RISTORANTE/I-------------------------------------------
Ristorante *ricevi_info_ristorante_da_server(int socket){
   
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
   FullRead(socket,ristorante_ricevuto->ip,sizeof(ristorante_ricevuto->ip));
   FullRead(socket,porta,sizeof(porta));
   
   strcpy(ristorante_ricevuto->nome,nome_ristorante);
   strcpy(ristorante_ricevuto->posizione,posizione_ristorante); 
   
   ristorante_ricevuto->id = atoi(id_lettura);
   
   ristorante_ricevuto->porta =ntohs(atoi(porta));
   
   return ristorante_ricevuto;
}

llist *ricevi_lista_ristoranti(int socket){
   char sizeList[100];
   
   FullRead(socket,sizeList,sizeof(sizeList));
   int size_list = atoi(sizeList);
   
   if(size_list == 0){
      printf("Nessun ristorante disponibile\n\n");
   }
   else{
      llist *list_restaurant = llist_create(NULL);
      /* in base alla dimensione della lista verranno ricevuti sizeList ristoranti (es: sizeList = 2 = 2 chiamate a "ricevi_info_ristorante") */
      for(int i = 0; i < size_list; i++){
         Ristorante *nuovoRistorante_caricato = ricevi_info_ristorante_da_server(socket);
         nuovoRistorante_caricato->id_scelta = size_list-i;
         llist_push(list_restaurant, nuovoRistorante_caricato);
      }
      return list_restaurant;
   }
   
   return NULL;
}


//---------------------------------------------GESTIIONE CONNESSIONE AI RISTORANTI---------------------------------------------

//@-------------------------------------------SCELTA RISTORANTE-------------------------------------------
Ristorante* controlla_scelta(int scelta_utente,llist *list_restaurant){
   
   struct node *curr = *list_restaurant;
   
   while(curr != NULL){
   
      if(curr->data != NULL){ 
         Ristorante *ristorante = curr->data;
         if( ristorante->id_scelta == scelta_utente){
           
            return ristorante;
         }
      }
      curr = curr->next;
   }
   return NULL;   
}
Ristorante *scegli_ristorante(llist *list_restaurant){
   
   int scelta;
   char repeat;
   
   Ristorante *ristorante_scelto = NULL;
   
   while(ristorante_scelto == NULL){
      
      /* Scorro e stampo la lista di ristoranti per mostrarla all'utente */
      struct node *curr = *list_restaurant;
      while(curr != NULL){
         if(curr->data != NULL){
            stampa_info_ristorante(curr->data);
         }
         curr = curr->next;
      }
      printf("\n");
      
      /* Ciclo che continua fino a quando la funzione "controlla_scelta" non restituisce un ristorante */
      while(ristorante_scelto == NULL){
         printf("Scegli l'id del ristorante con cui vuoi connetterti come rider: ");
         scanf("%d",&scelta);
         ristorante_scelto = controlla_scelta(scelta,list_restaurant);
         
         if(ristorante_scelto == NULL){
            printf("\nVuoi rivedere i ristoranti disponibili ? [s/n]\n");
            fflush(stdin);
            scanf("%c",&repeat);
            if(repeat == 's'){
               printf("\n");   
               break;
            }
            printf("\n");
         }
      }
      
   }
   return ristorante_scelto;
}

