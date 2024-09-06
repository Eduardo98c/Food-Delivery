#include<string.h>
#include"abstract_user.h"
#include"wrapperSysCall.h"


//@-------------------------------------------INVIA INFORMAZIONI UTENTE-------------------------------------------
void invia_info_utente(int socket, Utente *nuovoUtente){
   
   /* Invia l'ID del client al server */
   char id_client_invio[10];
   sprintf(id_client_invio,"%d",nuovoUtente->id);
   
   FullWrite(socket,id_client_invio,sizeof(id_client_invio));
   
   /* Invia il nome dell'utente */
   FullWrite(socket,nuovoUtente->nome,sizeof(nuovoUtente->nome));
   /* Invia il cognome dell' utente */
   FullWrite(socket,nuovoUtente->cognome,sizeof(nuovoUtente->cognome));
   /* Invia la posizione dell' utente al server */
   FullWrite(socket,nuovoUtente->posizione,sizeof(nuovoUtente->posizione));
}
//@-------------------------------------------RICEVI INFORMAZIONI UTENTE-------------------------------------------
Utente *ricevi_info_utente(int socket){

   Utente *nuovoUtente = (Utente *)malloc(sizeof(Utente));
   char id_client_ricevuto[10];
   char nome_utente[MAX_NOME_UTENTE];
   char cognome_utente[MAX_COGNOME_UTENTE];
   char posizione_utente[MAX_POSIZIONE_UTENTE];
   
   /* Riceve l'id del client */
   FullRead(socket,id_client_ricevuto,sizeof(id_client_ricevuto));
   int id_client = atoi(id_client_ricevuto);
   /* Riceve il nome dell'utente */
   FullRead(socket,nome_utente,sizeof(nome_utente));
   /* Riceve il cognome dell' utente */
   FullRead(socket,cognome_utente,sizeof(cognome_utente));
   /* Riceve la posizione dell' utente al server */
   FullRead(socket,posizione_utente,sizeof(posizione_utente));
   
   nuovoUtente->id = id_client;
   strcpy(nuovoUtente->nome,nome_utente);
   strcpy(nuovoUtente->cognome,cognome_utente);
   strcpy(nuovoUtente->posizione,posizione_utente);
   
   return nuovoUtente;
}
//@-------------------------------------------INVIA INFORMAZIONI RIDER-------------------------------------------
void invia_info_rider(int socket, Rider *nuovoRider){
   char id_invio[10];
   sprintf(id_invio,"%d",nuovoRider->id);
   
   /* Invia l'ID del rider*/
   FullWrite(socket,id_invio,sizeof(id_invio));
   /* Invia il nome del rider */
   FullWrite(socket,nuovoRider->nome,sizeof(nuovoRider->nome));
   /* Invia il cognome del rider */
   FullWrite(socket,nuovoRider->cognome,sizeof(nuovoRider->cognome));
}
//@-------------------------------------------RICEVI INFORMAZIONI RIDER-------------------------------------------
Rider *ricevi_info_rider(int socket){
   
   Rider *nuovoRider_ricevuto = (Rider *)malloc(sizeof(Rider));
   char id_ricevuto[10];
   /* Ricevi l'ID del rider*/
   FullRead(socket,id_ricevuto,sizeof(id_ricevuto));
   nuovoRider_ricevuto->id = atoi(id_ricevuto);
   /* Ricevi il nome del rider */
   FullRead(socket,nuovoRider_ricevuto->nome,sizeof(nuovoRider_ricevuto->nome));
   /* Ricevi il cognome del rider */
   FullRead(socket,nuovoRider_ricevuto->cognome,sizeof(nuovoRider_ricevuto->cognome));
   
   return nuovoRider_ricevuto;
}      

//@-------------------------------------------INVIA ORDINAZIONI-------------------------------------------
void invia_ordinazione(int socket, Ordinazioni *ordinazione_send){
   
   //printf("Invio: Pietanza: %s - Costo: %.2f\n",ordinazione_send->nome_cibo_o_bevanda,ordinazione_send->costo); 
   char costo_send[317];
   sprintf(costo_send,"%f",ordinazione_send->costo);
   
   FullWrite(socket,ordinazione_send->nome_cibo_o_bevanda,sizeof(ordinazione_send->nome_cibo_o_bevanda));
   FullWrite(socket,costo_send,sizeof(costo_send));

}
void invia_ordinazioni(int socket, queue *ordinazioni){
   
   /* Size del menu da inviare */
   int sizeQueue = getSize(ordinazioni);
   char send_size[4];
   sprintf(send_size,"%d",sizeQueue);

   /* Invio il size del menu da inviare al client così da fargli capire quante ordinazioni leggere */
   FullWrite(socket,send_size,sizeof(send_size));
   
   /* Effettua una copia della coda per poi poterla inviare */
   queue *ordinazioni_temp = copyQueue(ordinazioni);
   
   while(!isEmpty(ordinazioni_temp)){
      Ordinazioni *temp_ord;
      dequeue(ordinazioni_temp,&temp_ord);    
      invia_ordinazione(socket,temp_ord);  
   }
}

//@-------------------------------------------RICEVI ORDINAZIONI-------------------------------------------
Ordinazioni *ricevi_ordinazione(int socket){
   
   char costo_ricevuto[317];
   
   /* Inizializza la nuova ordinazione da ricevere */
   Ordinazioni *ordinazione_ricevuta = (Ordinazioni *)malloc(sizeof(Ordinazioni));
   
   FullRead(socket,ordinazione_ricevuta->nome_cibo_o_bevanda,sizeof(ordinazione_ricevuta->nome_cibo_o_bevanda));
   FullRead(socket,costo_ricevuto,sizeof(costo_ricevuto));
   ordinazione_ricevuta->costo = atof(costo_ricevuto);
   
   //printf("Ricevo: Pietanza: %s - Costo: %.2f\n",ordinazione_ricevuta->nome_cibo_o_bevanda,ordinazione_ricevuta->costo); 
   
   return ordinazione_ricevuta;
}
queue* ricevi_ordinazioni(int socket){
   
   /* Inizializzo la coda che conterrà il menu' ricevuto dal ristorante */
   queue *ordinazioni_ricevute = createQueue(sizeof(Ordinazioni *));
   
   /* Size del menu da ricevere */
   char size_ricevuto[4];
   
   /* Riceve il size del menu da inviare al client così da fargli capire quante ordinazioni leggere */
   FullRead(socket,&size_ricevuto,sizeof(size_ricevuto));
   
   int sizeQueue = atoi(size_ricevuto);
   if(sizeQueue == 0){
      return NULL;
   }
   for(int i = 0; i < sizeQueue; i++){
      Ordinazioni *temp_ord = ricevi_ordinazione(socket);
      temp_ord->id_scelta = i+1;
      enqueue(ordinazioni_ricevute,(Ordinazioni *) &temp_ord);
   }    
   if(ordinazioni_ricevute == NULL)
      return NULL;
      
   return ordinazioni_ricevute;
}

//@-------------------------------------------INVIA ORDINE (ID,CARRELLO E CONTO)-------------------------------------------
void invia_ordine(int socket,Ordine *nuovoOrdine){
   
   char id_invio[10];
   sprintf(id_invio,"%d",nuovoOrdine->id_ordine);
   
   char conto_send[317];
   sprintf(conto_send,"%f",nuovoOrdine->conto);
    
   /* Invia l'ID dell'ordine creato */
   FullWrite(socket,id_invio,sizeof(id_invio));
   /* Invia il conto */
   FullWrite(socket,conto_send,sizeof(conto_send));
   /* Invia il carrello dell'ordine dell'utente */
   invia_ordinazioni(socket,nuovoOrdine->queue_pietanze_ordinate);
}
//@-------------------------------------------RICEVI ORDINE (ID,CARRELLO E CONTO)-------------------------------------------
Ordine *ricevi_ordine(int socket){
   
   Ordine *nuovoOrdine_utente = (Ordine *)malloc(sizeof(Ordine));
   char id_ricevuto[10];
   char conto_ricevuto[317];
   
   int id_ordine;
   double conto;
   queue *carrello_utente = createQueue(sizeof(Ordinazioni *)); 
    
   /* Riceve l'ID dell'ordine creato */
   FullRead(socket,id_ricevuto,sizeof(id_ricevuto));
   id_ordine = atoi(id_ricevuto);
   /* Riceve il conto */
   FullRead(socket,conto_ricevuto,sizeof(conto_ricevuto));
   conto = atof(conto_ricevuto);
   
   /* Riceve il carrello dell' ordine dell'utente */
   carrello_utente = ricevi_ordinazioni(socket); 
   
   nuovoOrdine_utente->id_ordine = id_ordine;
   nuovoOrdine_utente->conto = conto;
   nuovoOrdine_utente->queue_pietanze_ordinate = carrello_utente;
   
   return nuovoOrdine_utente;
}

//---------------------------------------------STAMPA INFORMAZIONI---------------------------------------------
void stampa_coda_ordinazioni(queue *ordinazioni){
   
   queue *temp = copyQueue(ordinazioni);
   
   while(!isEmpty(temp)){
      Ordinazioni *temp_ord;
      dequeue(temp,&temp_ord);
      printf("Pietanza: %s - Costo: %.2f\n",temp_ord->nome_cibo_o_bevanda,temp_ord->costo);  
   }
   printf("\n\n");
}
void stampa_ordine_utente(Ordine *ordine_utente){
   
   printf("**********************NUOVO ORDINE**********************\nID: %d\n\n",ordine_utente->id_ordine);
   stampa_coda_ordinazioni(ordine_utente->queue_pietanze_ordinate);
   printf("conto: %.2f\n***************************************************************\n",ordine_utente->conto);
}  

void stampa_info_utente(Utente *utente){
   printf("***************************UTENTE***************************\nID: %d\nNome: %s\nCognome %s\nPosizione: %s\n************************************************************\n\n",utente->id,utente->nome,utente->cognome,utente->posizione);
}  

void stampa_info_rider(Rider *nuovoRider){
   printf("***************************RIDER***************************\nID: %d\nNome: %s\nCognome: %s\n***********************************************************\n\n",nuovoRider->id,nuovoRider->nome,nuovoRider->cognome);
}
