/* 
 * a simple multi-thread socket server
 * gcc -lpthread -wall -o bio_mul bio_multi_thread.c
 * 
 * start server on local, and access by Chrome with url 127.0.0.1:[port] 
 * you can get http header info back
 * */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/time.h>

#define THREAD_COUNT 5
pthread_t thread[THREAD_COUNT];
int sockfd, portno;
socklen_t clilen;
struct sockaddr_in serv_addr, cli_addr;

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void *workerThread(){
     char buffer[256];
     bzero(buffer,256);
     //4. accept connection,if no incoming connection, block here
     int newsockfd = accept(sockfd, 
                 (struct sockaddr *) &cli_addr, 
                 &clilen);
     printf("new incoming socketfd [%d]\n", newsockfd);
     if (newsockfd < 0) 
          error("ERROR on accept");
     bzero(buffer,256);
     int n;
     //5. read data from client, if no data, block here
     while(1){
        n = read(newsockfd,buffer,255);
        printf("get data:%s, n is:%d, newsockfd:[%d],sockfd:[%d]\n",buffer,n,newsockfd,sockfd);
        if(write(newsockfd,buffer,n) != n){
            error("ERROR write");
        }
        if(n < 255){
            break;
        }
     }
     //6. close sockect
     close(newsockfd);
     printf("bye,[%d]\n", newsockfd);
     return NULL;
}

int main(int argc, char *argv[])
{
     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }
    
     //1. create socket
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");

     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     //2. bind socket on port
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              error("ERROR on binding");
     //3. start listen on port
     listen(sockfd, 1);	// listen第二个参数为backlog即队列的排队数，还未理解
     clilen = sizeof(cli_addr);
    
     // create work thread
     memset(&thread, 0, sizeof(thread));
     int i;
     for(i=0;i< THREAD_COUNT;i++){
        int temp = pthread_create(&thread[i], NULL, workerThread, NULL);
        if(temp != 0){
            printf("thread[%d] create failed!", i);
        }
     }
     
     int j;
     for(j=0; j<THREAD_COUNT;j++){
        pthread_join(thread[j], NULL);
     }
     /*
     while(1){
     //4. accept connection,if no incoming connection, block here
     newsockfd = accept(sockfd, 
                 (struct sockaddr *) &cli_addr, 
                 &clilen);
     printf("new incoming socketfd [%d]\n", newsockfd);
     if (newsockfd < 0) 
          error("ERROR on accept");
     int n;
     while(1){
        //5. read data from client, if no data, block here
        n=read(newsockfd,buffer,255);
        if(n <= 0){
            break;
        } 
        printf("get data:%s, n is:%d, newsockfd:[%d],sockfd:[%d]\n",buffer,n,newsockfd,sockfd);
        if(write(newsockfd,buffer,n) != n){
            error("ERROR write");
        }
        bzero(buffer,256);
     }
     //6. close sockect
     close(newsockfd);
     printf("bye,[%d]", newsockfd);
     }
     */

     close(sockfd);
     return 0; 
}