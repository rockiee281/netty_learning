#include "stdafx.h"
#include "selectserver.h"

using namespace std;

SelectServer::SelectServer()
{
    FD_ZERO(&m_masterFdSet);
    FD_ZERO(&m_readFdSet);
}

bool SelectServer::init()
{
    /** get the listener */
    if((m_listenerSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        cerr << "Server-socket() error, fail to get the listener Socket" << endl;
        return false;
    }
    /** "address already in use" error message */
    uint32_t yes = 0;
    if(setsockopt(m_listenerSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        cerr << "Server-setsockopt() error, address already in use!" << endl;
        return false;
    }
    return true;
}

bool SelectServer::_listen(uint32_t port)
{
    m_serverAddr.sin_family = AF_INET;
    m_serverAddr.sin_addr.s_addr = INADDR_ANY;
    m_serverAddr.sin_port = htons(port);
    memset(&(m_serverAddr.sin_zero), '\0', 8);

    if(bind(m_listenerSocket, (struct sockaddr *)&m_serverAddr, sizeof(m_serverAddr)) == -1){
        cerr << "Server-bind() error lol!" << endl;
        return false;
    }
    if(listen(m_listenerSocket, 100) == -1) {
        cerr << "Server-listen() error lol!" << endl;
        return false;
    }

    FD_SET(m_listenerSocket, &m_masterFdSet);      /** add the listener to the master set */
    m_maxFd = m_listenerSocket; /** keep track of the biggest file descriptor, so far, it's this one*/
    return true;
}

bool SelectServer::_accept()
{
    sockaddr_in clientAddr;
    uint32_t clientAddrLen = sizeof (clientAddr);
    int newClientFd = accept(m_listenerSocket, (struct sockaddr *)&clientAddr, (socklen_t*)&clientAddrLen);
    cout << "_accept called...newClientFd is " << newClientFd << endl;
    if (newClientFd == -1) {
        cerr << "Server-accept() error lol!" << endl;
        return false;
    }
    FD_SET(newClientFd, &m_masterFdSet);            /** add to master set */
    m_maxFd = (newClientFd > m_maxFd) ? newClientFd : m_maxFd;  /** keep track of the maximum */
    onNewConnection(clientAddr);
    return true;
}

void onNewConnection(const sockaddr_in& clientAddr){
    cout << "accept new fd " << clientAddr.sin_addr.s_addr << endl;
}

bool SelectServer::_receive(int clientFd)
{
    uint32_t nbytes = recv(clientFd, buf, sizeof(buf), 0);
    string data(buf, nbytes);
    //cout << "_receive called...nbytes is " << nbytes << " data is " << data << endl;
    onClientData(clientFd, data);
    if (nbytes > 0) {   /** we got some data from a client*/
        broadcast(data);
        //echo(clientFd);
    } else {
        cout << "socket " << clientFd << " has sth wrong since nbytes == " << nbytes  << endl;
        removeClient(clientFd);
    }
    return true;
}

bool SelectServer::removeClient(int clientFd)
{
    onClientBroken(1);
    close(clientFd);
    FD_CLR(clientFd, &m_masterFdSet);
    return true;
}

bool SelectServer::pulse()
{
    m_readFdSet = m_masterFdSet;
    if(select(m_maxFd + 1, &m_readFdSet, NULL, NULL, NULL) == -1) {
        cerr << "Server-select() error lol!" << endl;
        return false;
    }

    /** run through the existing connections looking for data to be read*/
    printf("***************** some fd readable ********************\n");
    for(int i = 3; i <= m_maxFd; ++i) {
        if(FD_ISSET(i, &m_readFdSet)) { /** we got one... */
            printf("fd [%d] is readable!\n", i);
            if(i == m_listenerSocket) {
                printf("accept new fd [%d] !\n", i);
                _accept();
                return true;
            } else {
                printf("get data from  fd [%d] !\n", i);
                _receive(i);
            }
        }
    }
    return true;
}
void SelectServer::echo(int sockfd){
    if(FD_ISSET(sockfd, &m_masterFdSet) && (sockfd != m_listenerSocket)){
        char buffer[80];
        sprintf(buffer, "hello, client[%d],this is sky net\n", sockfd);
        if(send(sockfd,buffer,strlen(buffer),0) == -1){
            cerr << "echo() failed!" << endl;
        }
    }
}

void SelectServer::broadcast(const string &data)
{
    cout << "begin to send broadcast" << endl;
    for(int i = 0; i <= m_maxFd; i++) {
        /** send to everyone except the listener and ourselves */
        if(FD_ISSET(i, &m_masterFdSet) && (i != m_listenerSocket)) {
            cout << "send broadcast to " << i << endl;
            if(send(i, data.c_str(), data.size(), 0) == -1) {
            //char buffer[80];
            //sprintf(buffer, "hello, client[%d],this is sky net\n", i);
            //if(send(i,buffer,strlen(buffer),0) == -1){
                cerr << "send() to " << i << " error lol!" << endl;
            }
        }
    }
}
