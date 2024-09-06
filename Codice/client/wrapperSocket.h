#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<errno.h> 
#include<netdb.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<time.h>


int Socket(int family, int type, int protocol){
   
   int n; 
   if ( (n= socket(family, type, protocol)) < 0) {
       perror("socket error");
       exit(1);
   }
   return n;
}

int Connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen){
	
   int n;
   if ( (n=connect(sockfd,(struct sockaddr *) addr, addrlen)) < 0){
      perror("connect error");
	  return -1;
   }
   return n;
}


int Bind(int sockfd, struct sockaddr *servaddr,int dim){
   
   int n;
   if ( (n=bind(sockfd,(struct sockaddr *) servaddr, dim) ) < 0) {
        printf("bind error");
        return -1;
   }
   return n;
}

int Listen(int sockfd, int lunghezza_coda){
	
   int n;
   if( (n=listen(sockfd, lunghezza_coda)) < 0) {
      perror("listen error");
       exit(1);
   }
   return n;
}

int Accept(int sockfd, struct sockaddr *clientaddr, socklen_t *addr_dim){
   
   int connfd;
   if( (connfd = accept(sockfd, (struct sockaddr*) clientaddr, addr_dim)) < 0) {
      perror("accept error");
      exit(1);
   }
   return connfd;
}


/*recvfrom: solo per connessioni UDP, equivalente a Accept+read nelle connessioni TCP */
//sockfd - socket descriptor, buf - buffer di memorizzazione del messaggio, len - lunghezza del buffer, flags - imposta la modalità di funzionamento della comunicazione from - memorizza l'indirizzo del mittente, fromlen – memorizza la dimensione di from
ssize_t Recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen){
   int n;
   if( (n=recvfrom(sockfd,buf,len,flags,(struct sockaddr *) from,fromlen)) < 0){
      perror("recvfrom error");
      exit(0);
   }
   return n;
}
/*sendto: solo per connessioni UDP, equivalente a Connect+write nelle connessioni TCP*/
//sockfd - socket descriptor, buf - buffer che memorizza il messaggio da inviare, len - lunghezza del messaggio, flags - imposta la modalità di funzionamento della comunicazione, to - memorizza l'indirizzo del destinatario, addrlen – memorizza la dimensione di to
ssize_t Sendto(int sockfd, const void* buf, size_t len, int flags, const struct sockaddr* to, socklen_t addrlen){
   int n;
   if( (n = sendto(sockfd,buf,len,flags,(struct sockaddr*) to, addrlen)) < 0){
      perror("sendto error");
      exit(0);
   }
   return n;
}

int Select(int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset, const struct timeval *timeout){
   int n;
   if( (n=select(maxfdp1,readset,writeset,exceptset,(struct timeval *) timeout)) < 0){
      ("select error");
      exit(1);
   }
   return n;
}

int Setsockopt(int sd, int level, int optname, const void *optval, socklen_t optlen){
   
   int n;
   if( (n = setsockopt(sd,level,optname,optval,optlen)) < 0){
      perror("setsockopt error");
      exit(1);
   }
   return n;
}

//converte la stringa passata 'input_addr' in un indirizzo di rete scritto in network order e lo memorizza in 'servaddr_sin_addr' (struttura in_addr)
int Inet_pton(int family, char *input_addr, struct in_addr *servaddr_sin_addr){

   int n;
   if( (n=inet_pton(AF_INET, input_addr, servaddr_sin_addr)) < 0) {
      fprintf(stderr,"inet_pton error for %s\n",input_addr);
      exit(1);
   }
   return n;
}

//Converte l'indirizzo memorizzato in formato network in src in notazione dotted (192.167.11.34) memorizzandolo in dst
const char *Inet_ntop(int family, const void *src, char *dst, socklen_t cnt){ 
   const char *n;
   if( (n=inet_ntop(family,src,dst,cnt)) == NULL){
      fprintf(stderr,"inet_ntop error for %s\n",(char *) src);
      exit(1);
   }
   return n;
}

struct hostent *Gethostbyname(const char *name){
   struct hostent *host;
   
   if( (host = gethostbyname(name)) == NULL){
      herror("gethostbyname herror");
      exit(1);
   }
   return host;
}

struct hostent *Gethostbyname2(const char *name, int af){
   struct hostent *host;
   
   if( (host = gethostbyname2(name,af)) == NULL){
      herror("gethostbyname2 herror");
      exit(1);
   }
   return host;
}

struct hostent *Gethostbyaddr(const void *addr, socklen_t len, int type){
   struct hostent *host;
   
   if( (host = gethostbyaddr(addr,len,type)) == NULL){
      herror("gethostbyaddr herror");
      exit(1);
   }
   return host;
}
	
char* get_this_Info_host(){
   char hostbuffer[256];
   char *IPbuffer;
   struct hostent *host_entry;
   int hostname;
  
   // To retrieve hostname
   hostname = gethostname(hostbuffer, sizeof(hostbuffer));
  
   // To retrieve host information
   host_entry = Gethostbyname(hostbuffer);
  
   // To convert an Internet network
   // address into ASCII string
   IPbuffer = inet_ntoa(*((struct in_addr*)
                           host_entry->h_addr_list[0]));
  
   printf("Hostname: %s\n", hostbuffer);
   printf("Host IP: %s\n", IPbuffer);
   
   return IPbuffer;
  
}
//Identifica le informazioni di un determinato indirizzo 
struct hostent* detect_info_address(char *addr_input){
   
   struct hostent *data;
   char *addr, **alias;
   char recvline[1025];
   
   data = Gethostbyname(addr_input);	
    	 
   printf("[Canonical name]: %s\n", data->h_name);
   alias = data->h_aliases;
    
   while(*alias != NULL){
      printf("Alias %s\n", *alias);
	  alias++;
   }
   if(data->h_addrtype == AF_INET) {
      printf("[Address type]: IPv4\n");
   } 
   else if(data->h_addrtype == AF_INET6){
		printf("[Address type]: IPv6\n");
   } 
   else{
      printf("Tipo di indirizzo non valido\n");
	  exit(1);
   }
   alias = data->h_addr_list;
   
   while(*alias != NULL){
      addr = (char *)inet_ntop(data->h_addrtype, *alias, recvline, sizeof(recvline));
	  printf("[Address]: %s\n", addr);
	  alias++;
   }
   
   return data;
}
