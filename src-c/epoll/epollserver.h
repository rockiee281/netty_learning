#ifndef EPOLLSERVER_H
#define EPOLLSERVER_H

/**
 * @brief The EpollServer class
 * @author yurunsun@gmail.com
 */

class EpollServer
{
public:
    EpollServer();
    ~EpollServer();
    bool _listen(const std::string &port);
    bool pulse();

private:
    bool setUnblock(int socket);
    bool createEpoll();
    bool addEpoll(int socket, epoll_event &e);
    bool isEpollError(const epoll_event& e);
    bool isEpollNewConnection(const epoll_event& e);
    bool _error(const epoll_event& e);
    bool _accept(epoll_event &e);
    bool _receive(const epoll_event& e);
    bool _send(int clientFd, const std::string& data);
    bool removeClient(int clientFd);

    addrinfo m_serverAddr;              /** server address */
    int m_listenerSocket;               /** listening socket descriptor */
    int m_epollFd;                      /** epoll operation fd */
    epoll_event m_epollEvent;           /** epoll event*/
    epoll_event* m_pEpollEvents;        /** epoll events buffer to hold notification from kernal*/
    char m_readBuf[1024];               /** buffer for client data */
};

#endif // EPOLLSERVER_H