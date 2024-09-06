#include"wrapperSysCall.h"

pid_t Fork(){
   pid_t pid;
   
   if( (pid = fork()) < 0){
      perror("fork error");
	  exit(-1);
   }
   return pid;
}

int Open(const char *pathname, int flags, mode_t mode){
   int fd;
   if((fd = open(pathname,flags,mode)) < 0){
      perror("open error");
   }
   return fd;
}

size_t Read(int fd, void* buf, size_t count){
   
   size_t nread;
   if( (nread = read(fd,buf,count)) < 0){
      printf("read error");
      return -1;
   }
   return nread;
}
ssize_t FullRead(int fd, void *buf, size_t count){
   size_t nleft;
   ssize_t nread;
   nleft = count;
   while (nleft > 0) {
   /* repeat until no left */
      if ( (nread = read(fd, buf, nleft)) < 0) {
         if (errno == EINTR) { /* if interrupted by system call */
            continue;
            /* repeat the loop */
         }else
         {
            exit(nread);
            /* otherwise exit */
         }
      }
      else if (nread == 0) { /* EOF */
         break;
         /* break loop here */
      }
      nleft -= nread;
      /* set left to read */
      buf +=nread;
      /* set pointer */
   }
   buf=0;
   return (nleft);
}



size_t Write (int fd, void* buf, size_t count){

   size_t nwrite;
   if( (nwrite = write(fd,buf,count)) < 0){
      printf("write error");
      return -1;
   }
   return nwrite;
}

ssize_t FullWrite(int fd, const void *buf, size_t count){ 
   size_t nleft; 
   ssize_t nwritten; 
   nleft = count; 
   
   while (nleft > 0) {             /* repeat until no left */ 
      if(( nwritten = write(fd, buf, nleft) ) < 0){
	     if( errno == EINTR){
		    continue;
		 }
		 else{
		    perror("Fullwrite error"); 
		    exit(nwritten);
		 }
	  }
	  nleft -= nwritten;
	  buf += nwritten;
   }
       
   return (nleft); 
}

//Legge dal file ed inserisce il contenuto in un buffer
char* leggi_da_file(int file){
   
   if(file == -1){
      perror("leggi_da_file error");
      exit(1);
   }
   int dim_file = lseek(file, 0L, SEEK_END);
   char *buff_file;
   
   
   buff_file = (char *)malloc(dim_file*sizeof(char));
   lseek(file, 0L, SEEK_SET);
   
   
   Read(file,buff_file,dim_file);
   
   return buff_file;
}
