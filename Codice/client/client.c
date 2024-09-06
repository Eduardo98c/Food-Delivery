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
#define CLIENT_ID 2

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
   queue *menu;
};
typedef struct ristorante Ristorante;

//FUNZIONI DI RETE
HO *detect_info_address(char *server_addr);                                      /* rileva informazioni sul server */
int connetti_tcp(char *server_addr, short port);                                 /* Connette il client al server */
void close_client();                                                             /* Chiusura pulita socket in caso di sigstop */

//FUNZIONI PER INVIARE/RICEVERE DATI

//@INVIA RISTORANTE SCELTO
void invia_info_ristorante_scelto(int socket, Ristorante *ristorante_scelto);    /* Invia le informazioni del ristorante scelto dall'utente */

//@RICEVI RISTORANTI
Ristorante *ricevi_info_ristorante(int socket);                                  /* Chiamato sizeList volte dalla funzione 'ricevi_lista_ristoranti' */
llist *ricevi_lista_ristoranti(int socket);                                      /* Riceve la lista di ristoranti dal server per effettuarne la scelta */

//FUNZIONI DI STAMPA 
void stampa_info_ristorante(Ristorante *ristorante);                             /* Stampa le informazioni dei ristoranti ricevuti dal server */
void stampa_menu_ristorante(queue *menu);                                        /* Stampa il menu' del ristorante ricevuto dal server */

//FUNZIONI MENU'

//@SCEGLI RISTORANTE
Ristorante* controlla_scelta(int scelta_utente,llist *list_restaurant);          /* Controlla che l'id scelto dall'utente sia corretto */
Ristorante *scegli_ristorante(llist *list_restaurant);                           /* Gestisce il menu' per la scelta del ristorante */

//@SCEGLI ORDINAZIONI(PIETANZE)
Ordinazioni *controlla_scelta_pietanza(int scelta_pietanza,queue *menu);         /* Controlla che l'ID della pietanza scelta sia valido */
Ordine *effettua_ordine(queue *menu);                                            /* Gestisce il menu' per la scelta delle pietanze e la creazione dell' ordine */

//FUNZIONI IMMISSIONE DATI DA INPUT
Utente * immetti_info_utente();                                                  /* Immetti le informazioni dell'utente */

/* main */
int main (int argc, char *argv[]){
   
   /* Inizializzazione variabili */
   int i,isConnesso = 1;
   
   /* Variabili client */
   int client_socket;
   
   /* Ristorante da scegliere */
   Ristorante *ristorante_scelto;
   
   /* Inizializzo la lista dei ristoranti da far riempire al server */
   llist *list_restaurant = llist_create(NULL);
   
   srand(time(NULL));
   
   /* Permette l'immissione di informazioni dell'utente da standard input */
   Utente *nuovoUtente = immetti_info_utente();
   signal(SIGTSTP,close_client);
   
   /* Stampa le informazioni dell'utente */
   stampa_info_utente(nuovoUtente);
   /* Rileva informazioni sull'ip del server da contattare */
   HO *data = detect_info_address(SERVER_IP);
   
   
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
         if(ristorante_scelto != NULL){
            /* Invia al server la scelta(ristorante) effettuata dall'utente */
            invia_info_ristorante_scelto(client_socket,ristorante_scelto);
         }
         else{
            //exit(0);
         }       
         /* Attende l'esito esito dal server (s = ristorante contattato disponibile; n = ristorante contattato non più presente nella lista)*/   
         char esito_richiesta_ricevuto[1];
         FullRead(client_socket,esito_richiesta_ricevuto, sizeof(esito_richiesta_ricevuto));
   
         if(esito_richiesta_ricevuto[0] == 'n'){
            printf("Il ristorante %s non è piu' disponibile, contattare un altro ristorante\n",ristorante_scelto->nome);
            continue;
            //exit(0);
         }

         printf("Menu' in arrivo dal ristorante '%s'...\n",ristorante_scelto->nome);
         FullRead(client_socket,esito_richiesta_ricevuto, sizeof(esito_richiesta_ricevuto));
   
         /* Attende l'esito esito dal server (s = ristorante contattato disponibile; n = ristorante contattato non disponibile(chiuso))*/   
         if(esito_richiesta_ricevuto[0] == 'n'){
            printf("\nIl ristorante [%s] non e' piu' disponibile, ricontattarlo in un secondo momento o contattare un altro ristorante\n\n",ristorante_scelto->nome);
            continue;
            //exit(0);
         }
         ristorante_scelto->menu = createQueue(sizeof(Ordinazioni *));
  
         /* Riceve il menu' dal server */
         queue *menu_ricevuto = ricevi_ordinazioni(client_socket);
         ristorante_scelto->menu = menu_ricevuto;
      
         /* Permette all'utente di creare un nuovo ordine (ID ordine, coda di pietanze scelte , costi, conto)*/
         Ordine *nuovoOrdine;
         int flag_ordine = 0;
      
         if(menu_ricevuto != NULL){
            printf("Menu' ricevuto, puoi effettuare l' ordine\n\n");
            while(!flag_ordine){
         
               /* Effettua l'ordine */
               nuovoOrdine = effettua_ordine(menu_ricevuto);
               if(nuovoOrdine->conto > nuovoUtente->saldo){
                  printf("Non hai denaro sufficiente, creare un ordine valido\n\n");
               }
               else{
                  /* Ordine completato */
                  flag_ordine = 1;
                  nuovoUtente->saldo = nuovoUtente->saldo-nuovoOrdine->conto;
                  printf("Ordine creato correttamente,ecco il tuo ID ordine: [%d] ed il tuo nuovo saldo: [%.2f]\n\n",nuovoOrdine->id_ordine,nuovoUtente->saldo);
               }
            }
            flag_ordine = 0;
         }
         if(nuovoOrdine == NULL){
            printf("Errore, ordine non esistente\n");
            continue;
            //exit(0);
         }
         /* Invia l'ordine ultimato al server */
         invia_ordine(client_socket,nuovoOrdine);
   
         /* Invia informazioni sull' utente */
         invia_info_utente(client_socket,nuovoUtente);
         printf("Informazioni dell' utente inviate al server\n");
         printf("Ordine [%d] inviato al server, in attesa di un rider per la consegna...\n\n", nuovoOrdine->id_ordine);
   
         /* Appena il rider ha inviato la conferma al ristorante, le sue informazioni vengono inoltrate al server e infine qui sul client */
         Rider *rider_incaricato = ricevi_info_rider(client_socket);
         printf("Il rider di ID: [%d] e nome: [%s] sta effettuando la tua consegna (ID ordine : [%d]) presso il tuo domicilio in: [%s]...\n\n",rider_incaricato->id,rider_incaricato->nome,nuovoOrdine->id_ordine, nuovoUtente->posizione);
   
         /* Attende la notifica del server per il completamento dell' ordine */
         char consegna_effettuata[1];
         FullRead(client_socket,consegna_effettuata,sizeof(consegna_effettuata));
         if(consegna_effettuata[0] == 's'){
            printf("Il rider di ID: [%d] e nome: [%s] ha completato la tua consegna (ID ordine : [%d]), arrivederci\n\n",rider_incaricato->id,rider_incaricato->nome,nuovoOrdine->id_ordine);
         }
      }
      /* Opzione per far effettuare al client un' altra consegna */
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
   char sendline_client[1] = "2";
   
   client_socket = Socket(AF_INET, SOCK_STREAM, 0);
   
   servaddr.sin_family = AF_INET;
   servaddr.sin_addr.s_addr = inet_addr(server_addr);
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
void close_client(){
   exit(0); 
}

//---------------------------------------------STAMPA INFORMAZIONI RISTORANTI---------------------------------------------

void stampa_menu_ristorante(queue *menu){
   
   queue *temp = copyQueue(menu);
   
   while(!isEmpty(temp)){
      Ordinazioni *temp_ord;
      dequeue(temp,&temp_ord);
      printf("%d) Pietanza: %s - Costo: %.2f\n",temp_ord->id_scelta,temp_ord->nome_cibo_o_bevanda,temp_ord->costo);  
   }
   printf("\n\n");
}

void stampa_info_ristorante(Ristorante *ristorante){
    
   printf("%d) ID: %d - Nome Ristorante: %s - Posizione: %s\n",ristorante->id_scelta, ristorante->id, ristorante->nome, ristorante->posizione);
  
}
//---------------------------------------------FUNZIONI CARICAMENTO INFORMAZIONI UTENTE---------------------------------------------

//@-------------------------------------------CARICAMENTO INFORMAZIONI UTENTE DA STANDARD INPUT-------------------------------------------
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

Utente * immetti_info_utente(){
   Utente *nuovoUtente = (Utente *)malloc(sizeof(Utente));
   
   char nome[MAX_NOME_UTENTE]; 
   char cognome[MAX_COGNOME_UTENTE];
   char posizione[MAX_POSIZIONE_UTENTE];
    
   double saldo;
   
   printf("\n---------------INFORMAZIONI UTENTE---------------\n");
   
   /* Inserimento nome utente */
   do{    
      printf("Inserisci il tuo nome: ");
      fflush(stdin);
      fgets(nome,MAX_NOME_UTENTE,stdin);    
        
      if(strlen(nome) <= 3){
         printf("\nInserire un nome di almeno 3 caratteri\n");
      }
      
      
   }while(isNumeric(nome) || nome == NULL  || strlen(nome) <= 3);
   
   remove_all_chars(nome,"\n");
   
   /* Inserimento cognome utente */
   do{
       
      printf("\nInserisci il tuo cognome: ");
      fflush(stdin);
      fgets(cognome,MAX_COGNOME_UTENTE,stdin);    
        
      if(strlen(cognome) <= 3){
         printf("\nInserire un cognome di almeno 3 caratteri\n");
      }
      
   }while(isNumeric(cognome) || cognome == NULL  || strlen(cognome) <= 3);
   remove_all_chars(cognome,"\n");
   
   /* Inserimento posizione utente */
   do{
       
      printf("\nInserisci la tua posizione: ");
      fflush(stdin);
      fgets(posizione,MAX_POSIZIONE_UTENTE,stdin);    
        
      if(strlen(posizione) <= 6){
         printf("\nInserire una posizione di almeno 6 caratteri\n");
      }
      
   }while(isNumeric(posizione) || posizione == NULL  || strlen(posizione) <= 6);
   remove_all_chars(posizione,"\n");
   
   /* Inserimento saldo utente */
   do{    
      printf("\nInserisci il tuo saldo: ");
      fflush(stdin);
      scanf("%lf",&saldo);    
      
      if(saldo <= 0.0){
         printf("Inseirire un saldo maggiore di 0\n");
      }    
          
   }while(saldo <= 0);
   
   
   printf("\n***************FINE INFORMAZIONI UTENTE***************\n");
   
   /* copio le informazioni inserite da input + id nell' utente creato */
   nuovoUtente->id = generate_random_id();
   strcpy(nuovoUtente->nome,nome);
   strcpy(nuovoUtente->cognome,cognome);
   strcpy(nuovoUtente->posizione,posizione);
   nuovoUtente->saldo = saldo;   
 
   return nuovoUtente;
}
//---------------------------------------------FUNZIONI PER INVIARE O RICEVERE DATI---------------------------------------------


//@-------------------------------------------INVIA RISTORANTE/I-------------------------------------------
void invia_info_ristorante_scelto(int socket, Ristorante *ristorante_scelto){
   
   char id_invio[10];
   sprintf(id_invio,"%d",ristorante_scelto->id);
  
   FullWrite(socket,ristorante_scelto->nome, sizeof(ristorante_scelto->nome));
   FullWrite(socket,ristorante_scelto->posizione, sizeof(ristorante_scelto->posizione));
   FullWrite(socket,id_invio,sizeof(id_invio));
}

//@-------------------------------------------RICEVI RISTORANTE/I-------------------------------------------
Ristorante *ricevi_info_ristorante(int socket){
   
   /* Alloco un nuovo ristorante */
   Ristorante *ristorante_ricevuto = (Ristorante *)malloc(sizeof(Ristorante));
 
   char nome_ristorante[MAX_NOME_RISTORANTE];
   char posizione_ristorante[MAX_POSIZIONE_RISTORANTE];
   char id_lettura[10];
    
   /* Riceve informazioni dal ristorante da memorizzare nella lista */
   FullRead(socket,nome_ristorante, sizeof(nome_ristorante));
   FullRead(socket,posizione_ristorante, sizeof(posizione_ristorante));
   FullRead(socket,id_lettura,sizeof(id_lettura));
  
   
   strcpy(ristorante_ricevuto->nome,nome_ristorante);
   strcpy(ristorante_ricevuto->posizione,posizione_ristorante); 
   
   ristorante_ricevuto->id = atoi(id_lettura);
  
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
         Ristorante *nuovoRistorante_caricato = ricevi_info_ristorante(socket);
         nuovoRistorante_caricato->id_scelta = size_list-i;
         llist_push(list_restaurant, nuovoRistorante_caricato);
      }
      return list_restaurant;
   }
   
   return NULL;
}

//---------------------------------------------GESTIIONE SCELTE UTENTE---------------------------------------------

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
         printf("Scegli l'id del ristorante da cui vuoi ordinare: ");
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

//@-------------------------------------------SCELTA PIETANZE DA MENU'-------------------------------------------
Ordinazioni *controlla_scelta_pietanza(int scelta_pietanza,queue *menu){
   
   queue *temp = copyQueue(menu);
  
   while(!isEmpty(temp)){
      Ordinazioni *temp_ord;
      dequeue(temp,&temp_ord);
      if(temp_ord->id_scelta == scelta_pietanza){
         return temp_ord;
      }
   }  
}

Ordine *effettua_ordine(queue *menu){
   
   int scelta,scelta_pietanza;
   double conto = 0;
   Ordine *nuovoOrdine = NULL;
   
   /* coda (carrello) per inserire le pietanze nel carrello */
   queue *carrello = createQueue(sizeof(Ordinazioni *));
   
   /* Fin quando il nuovo ordine non viene creato */
   while(nuovoOrdine == NULL){
      printf("1) Stampa Menu'\n2) Aggiungi le pietanze al carrello per l'ordine inserendone l'ID\n3) Completa l'ordine e invia\n\n");      
      char controllo_scelta;
      int scelta;
      fflush(stdin);
      scanf(" %c",&controllo_scelta);
      if(isNumeric((char *) &controllo_scelta)){
         scelta = 0;
      }
      else{
         scelta = atoi((char *) &controllo_scelta);   
      }
      switch(scelta){
         
         case 1: /* Stampa a video il menu' con i relativi id */
                 stampa_menu_ristorante(menu);
                 break;
                 
         case 2: /*L'utente inserira' l'ID della pietanza e se esiste, verrà aggiunta all'ordine */
                 printf("Id pietanza: ");
                 fflush(stdin);
                 scanf("%d",&scelta_pietanza);
                 Ordinazioni *pietanza_ordinata = controlla_scelta_pietanza(scelta_pietanza, menu);
                 
                 if(pietanza_ordinata != NULL){
                    printf("\n[pietanza: %s - Costo: %.2f] Aggiunta al carrello\n\n",pietanza_ordinata->nome_cibo_o_bevanda,pietanza_ordinata->costo);
                    enqueue(carrello,(Ordinazioni *) &pietanza_ordinata);
                    conto += pietanza_ordinata->costo;
                 }
                 else{
                    printf("Nessuna pietanza valida inserita\n\n");
                 }
                 break; 
         
         case 3: /* Viene creato il nuovo ordine se è stata scelta almeno una pietanza */
                 if(conto != 0.0){
                    printf("Ordine completato\n\n");
                    nuovoOrdine = (Ordine *)malloc(sizeof(Ordine));
                    nuovoOrdine->id_ordine = generate_random_id();
                    nuovoOrdine->queue_pietanze_ordinate = carrello;
                    nuovoOrdine->conto = conto;      
                 }
                 else
                    printf("Nessuna pietanza aggiunta al carrello, inserire almeno una pietanza per effettuare l'ordine\n\n");
                 
                 break;
        
        default:   printf("Nessuna opzione valida\n\n");
                   break;
      
      }
     
   }
   if(nuovoOrdine == NULL){
      return NULL;
   }
   
   return nuovoOrdine;
}
