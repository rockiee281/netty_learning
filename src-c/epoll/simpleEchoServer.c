#include <sys/epoll.h>  
#include <netinet/in.h>  
#include <sys/types.h>          /* See NOTES */  
#include <sys/socket.h>  
#include <string.h>  
#include <stdio.h>  
#include <unistd.h>  
#include <fcntl.h>  
  
#include <errno.h>  
#include <stdlib.h>  
typedef struct sockaddr_in sockaddr_in ;  
typedef struct sockaddr     sockaddr ;  
  
#define SER_PORT    9191
  
int nonblock(int fd){  
    int opt ;  
    opt = fcntl(fd,F_GETFL);  
    opt |= O_NONBLOCK ;  
    return fcntl(fd,F_SETFL,opt);  
}  
  
int main(int argc,char**argv){  
    sockaddr_in srv, cli ;  
    int listen_fd ,con_fd ;  
    socklen_t  len;  
    int res ,nsize,ws;  
    char buf[8];  
  
    int epfd,ers;  
    struct epoll_event evn,events[50];  
    int i;  
  
    bzero(&srv,sizeof(srv));  
    bzero(&cli,sizeof(cli));  
    srv.sin_port= htons(SER_PORT);  
    srv.sin_family = AF_INET ;  
    listen_fd = socket(AF_INET,SOCK_STREAM,0);  
  
    int yes = 1;  
    setsockopt(listen_fd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int));  
  
    if(bind(listen_fd,(sockaddr*)&srv,sizeof(sockaddr))<0)  {  
        perror("bind");  
        exit(0);  
    }  
    listen(listen_fd,100);  
    nonblock(listen_fd);  
    epfd = epoll_create(200);  
    evn.events = EPOLLIN|EPOLLET ;  
    evn.data.fd = listen_fd;   
    epoll_ctl(epfd,EPOLL_CTL_ADD,listen_fd,&evn);  
    static int count ;  
    while(1){  
        ers = epoll_wait(epfd,events,200,-1);  
        if(ers<0 ){  
            perror("epoll_wait:");exit(0);  
        }else if(ers==0){  
            printf("time out:%d\n",count++);  
            continue ;  
        }  
        for(i=0;i<ers;i++){  
            if(events[i].data.fd == listen_fd){  
                con_fd = accept(listen_fd,(sockaddr_in*)&cli ,&len);  
                nonblock(con_fd);  
//                printf("connect from:%s\n",inet_ntoa(cli.sin_addr));  
                evn.data.fd = con_fd;  
                evn.events = EPOLLIN | EPOLLET ;  
                epoll_ctl(epfd,EPOLL_CTL_ADD,con_fd,&evn);  
  
            }else if(events[i].events & EPOLLIN){     
                  
                nsize = 0;  
                while((res=read(events[i].data.fd,buf+nsize,sizeof(buf)-1))>0){  
                    nsize+= res;  
                }  
                if(res==0){  
                    epoll_ctl(epfd,EPOLL_CTL_DEL,events[i].data.fd,NULL);     
                    printf("a client over\n");  
                    close(con_fd);  
                    continue ;  
                }else if(res<0 && errno!=EAGAIN){  
                    perror("read");  
                    continue ;  
                }  
                buf[nsize]=0;  
                evn.data.fd = events[i].data.fd;  
                evn.events=EPOLLOUT|EPOLLET ;  
                epoll_ctl(epfd,EPOLL_CTL_MOD,events[i].data.fd,&evn);                 
                  
            }else if(events[i].events & EPOLLOUT){  
                nsize = strlen(buf);  
                ws = 0;  
                while(nsize>0){  
                     ws=write(events[i].data.fd,buf,nsize);  
                    nsize-=ws;  
                }  
                evn.data.fd = events[i].data.fd;  
                evn.events=EPOLLIN|EPOLLET ;  
                epoll_ctl(epfd,EPOLL_CTL_MOD,events[i].data.fd,&evn);     
                close(events[i].data.fd);
            }else{  
                printf("others\n");  
                  
            }             
        }  
  
    }  
  
    close(listen_fd);  
      
    return 0;  
}
