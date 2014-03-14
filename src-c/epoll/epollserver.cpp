#include "stdafx.h"
#include "epollserver.h"

using namespace std;
#define MAXEVENTS 64

EpollServer::EpollServer()
    : m_pEpollEvents(NULL)
{
}

EpollServer::~EpollServer()
{
    if (m_pEpollEvents != NULL) {
        delete [] m_pEpollEvents;
    }
}

bool EpollServer::_listen(const string& port)
{
    cout << "try to listen port " << port << endl;
    addrinfo *pResult = NULL;
    memset(&(m_serverAddr), '\0', sizeof(m_serverAddr));
    m_serverAddr.ai_family = AF_UNSPEC;         /** Return IPv4 and IPv6 choices */
    m_serverAddr.ai_socktype = SOCK_STREAM;     /** We want a TCP socket */
    m_serverAddr.ai_flags = AI_PASSIVE;         /** All interfaces */

    if (getaddrinfo(NULL, port.c_str(), &m_serverAddr, &pResult) != 0) {
        cerr << "fail to getaddrinfo!" << endl;
        return false;
    }
    if (pResult != NULL) {
        for (addrinfo *pRes = pResult; pRes != NULL; pRes = pRes->ai_next) {
            if ((m_listenerSocket = socket (pRes->ai_family, pRes->ai_socktype, pRes->ai_protocol)) == -1) {
                cerr << "fail to create socket for " << pRes->ai_family << " " << pRes->ai_socktype << " " << pRes->ai_protocol << endl;
                continue;
            }
            if (bind(m_listenerSocket, pRes->ai_addr, pRes->ai_addrlen) == -1) {
                cerr << "fail to bind " << m_listenerSocket << " " << pRes->ai_addr << " " << pRes->ai_addrlen << endl;
                close(m_listenerSocket);
                continue;
            }
            freeaddrinfo(pResult);
            setUnblock(m_listenerSocket);
            if (listen (m_listenerSocket, SOMAXCONN) == -1) {
                cerr << "fail to listen " << m_listenerSocket << endl;
            } else {
                cout << "listen port " << port << " ok! " << endl;
                return createEpoll();                    /** We managed to bind successfully! */
            }
        }
    }
    return false;
}

bool EpollServer::pulse()
{
    int n = epoll_wait(m_epollFd, m_pEpollEvents, MAXEVENTS, -1);
    for (int i = 0; i < n; ++i) {
        epoll_event& e = m_pEpollEvents[i];
        if (isEpollError(e)) {
            _error(e);
        } else if (isEpollNewConnection(e)) {
            _accept(e);
        } else {
            _receive(e);
        }
    }
    return true;
}

bool EpollServer::setUnblock(int socket)
{
    int flag = 0;
    if ((flag = fcntl(socket, F_GETFL, 0)) != -1) {
        flag |= O_NONBLOCK;
        if (fcntl (socket, F_SETFL, flag) != -1) {
            return true;
        }
    }
    cerr << "fail to call fcntl F_SETFL for m_listenerSocket" << endl;
    return false;
}

bool EpollServer::createEpoll()
{
    cout << "try to creat epoll" << endl;
    if ((m_epollFd = epoll_create1(0)) == -1) {
        cerr << "fail to call epoll_create" << endl;
        return false;
    }
    m_epollEvent.data.fd = m_listenerSocket;
    m_epollEvent.events = EPOLLIN | EPOLLET;
    if (addEpoll(m_listenerSocket, m_epollEvent)) {
        m_pEpollEvents = new epoll_event[MAXEVENTS];
        cout << "create epoll ok!" << endl;
        return true;
    }
    return false;
}

bool EpollServer::addEpoll(int socket, epoll_event& e)
{
    if ((epoll_ctl (m_epollFd, EPOLL_CTL_ADD, socket, &e)) == -1) {
        cerr << "fail to call epoll_ctl for " << socket << endl;
        return false;
    }
    return true;
}

bool EpollServer::isEpollError(const epoll_event &e)
{
    return ((e.events & EPOLLERR) || (e.events & EPOLLHUP) || (!(e.events & EPOLLIN)));
}

bool EpollServer::isEpollNewConnection(const epoll_event &e)
{
    return (m_listenerSocket == e.data.fd);
}

bool EpollServer::_error(const epoll_event &e)
{
    /** An error has occured on this fd, or the socket is not ready for reading */
    cerr << "epoll error for client " << e.data.fd << endl;
    removeClient(e.data.fd);
    return true;
}

bool EpollServer::_accept(epoll_event &e)
{
    cout << "a new client is coming - " << e.data.fd << endl;
    sockaddr clientAddr;
    int clientFd = 0;
    socklen_t clientAddrLen = sizeof (clientAddr);
    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

    if ((clientFd = accept (m_listenerSocket, &clientAddr, &clientAddrLen)) == -1) {
        if ((errno != EAGAIN) && (errno != EWOULDBLOCK)) {
            cerr << "fail to accept new client " << endl;
            return false;
        }
    }
    if (getnameinfo (&clientAddr, clientAddrLen, hbuf, sizeof(hbuf), sbuf, sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV) == -1) {
        cerr << "fail to getnameinfo" << endl;
    }
    if (!setUnblock(clientFd)) {
        cerr << "fail to set unblock to client fd" << clientFd << endl;
        return false;
    }
    e.data.fd = clientFd;
    e.events = EPOLLIN | EPOLLET;
    return addEpoll(clientFd, e);
}

bool EpollServer::_receive(const epoll_event &e)
{
    int clientFd = e.data.fd;
    uint32_t nbytes = recv(clientFd, m_readBuf, sizeof(m_readBuf), 0);
    cout << "receive " << nbytes << " bytes data from client " << clientFd << endl;
    if (nbytes > 0) {   /** we got some data from a client*/
        string data(m_readBuf, nbytes);
        _send(1, data);
        _send(clientFd, data);
    } else {
        cout << "socket " << clientFd << " has sth wrong since nbytes == " << nbytes  << endl;
        removeClient(clientFd);
    }
    return true;
}

bool EpollServer::_send(int clientFd, const std::string &data)
{
    if (write(clientFd, data.c_str(), data.size()) == -1) {
        cerr << "fail to send data to " << clientFd << endl;
        return false;
    }
    return true;
}

bool EpollServer::removeClient(int clientFd)
{
    cout << "remove client " << clientFd << endl;
    close (clientFd);
    return true;
}