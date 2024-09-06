#ifndef ABSTRACT_USER_H
#define ABSTRACT_USER

#include <stdio.h>
#include <stdlib.h>
#include "queue.h"


#define MAX_NOME_UTENTE 50
#define MAX_COGNOME_UTENTE 50
#define MAX_POSIZIONE_UTENTE 150

#define MAX_NOME_RIDER 50
#define MAX_COGNOME_RIDER 50

#define MAX_NOME_RISTORANTE 50
#define MAX_POSIZIONE_RISTORANTE 150
#define MAX_NOME_CIBO_O_BEVANDA 100

//STRUTTURA UTENTE
struct utente{
   int id;
   char nome[MAX_NOME_UTENTE];
   char cognome[MAX_COGNOME_UTENTE];
   char posizione[MAX_POSIZIONE_UTENTE];
   double saldo;
};
typedef struct utente Utente;

//STRUTTURA RIDER
struct rider{
   int id;
   char nome[MAX_NOME_RIDER];
   char cognome[MAX_COGNOME_RIDER];
};
typedef struct rider Rider;

//STRUTTURA ORDINAZIONI
struct ordinazioni{
   int id_scelta;
   char nome_cibo_o_bevanda[MAX_NOME_CIBO_O_BEVANDA];
   double costo;
};
typedef struct ordinazioni Ordinazioni;

//STRUTTURA ORDINE 
struct ordine{
   int id_ordine;
   double conto;
   queue *queue_pietanze_ordinate;
};
typedef struct ordine Ordine;

//@FUNZIONI UTENTE
void invia_info_utente(int socket, Utente *nuovoUtente);                       /* Invia le informazioni dell' utente */
Utente *ricevi_info_utente(int socket);                                        /* Riceve le informazioni dell' utente che ha richiesto un nuovo ordine(client) */

//@FUNZIONI RIDER
void invia_info_rider(int socket, Rider *nuovoRider);                          /* Invia le informazioni dei rider connessi */
Rider *ricevi_info_rider(int socket);                                          /* Riceve le informazioni dai rider che si connettono */

//FUNZIONI ORDINAZIONI
void invia_ordinazione(int socket, Ordinazioni *ordinazione_send);             /* Invia un' ordinazione (nome+costo) */
void invia_ordinazioni(int socket, queue *ordinazioni);                        /* Invia una coda di ordinazioni (usa sizeQueue volte "invia_ordinazione")*/
Ordinazioni *ricevi_ordinazione(int socket);                                   /* Riceve un'ordinazione (nome + costo) */
queue* ricevi_ordinazioni(int socket);                                         /* Riceve una coda di ordinazioni usa sizeQueue volte "ricevi_ordinazione")*/

//@FUNZIONI ORDINE
void invia_ordine(int socket,Ordine *nuovoOrdine);                             /* Invia l'ordine dell'utente */
Ordine *ricevi_ordine(int socket);                                             /* Riceve l'ordine creato dall'utente */

//@FUNZIONI STAMPA INFORMAZIONI
void stampa_coda_ordinazioni(queue *ordinazioni);                              /* Stampa la coda di ordinazioni */
void stampa_ordine_utente(Ordine *ordine_utente);                              /* Stampa l'ordine dell' utente */
void stampa_info_utente(Utente *utente);                                       /* Stampa le informazioni dell'utente */
void stampa_info_rider(Rider *nuovoRider);                                     /* Stampa le informazioni del rider */


#endif
