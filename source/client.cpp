// erik vega
// emv76
// data comm networks pa2

#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "packet.cpp"

using namespace std;


int main(int argc, char *argv[]){
    struct hostent *s;
    s = gethostbyname(argv[1]);
    struct sockaddr_in server;
    int mysocket, handshakePort, serverPort;
    socklen_t slen = sizeof(server);
    char randomPort[100];
    
    //stage 1: handshake
    char handshakeMsg[100] = "1234";

    stringstream temp(argv[2]);
    temp >> handshakePort;  

    if((mysocket=socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        cout << "Client: Problem in socket creation.\n";
        exit(0);
    }

    memset((char *)&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(handshakePort);
    bcopy((char *)s->h_addr, 
    (char *)&server.sin_addr.s_addr,
    s->h_length);

    //sending handshake message to server
    if(sendto(mysocket, handshakeMsg, 16, 0, (struct sockaddr *)&server, slen) < 0){
        cout << "Client: Problem in sending handshake.\n";
        exit(0);
    }

    //getting random port from the server
    if(recvfrom(mysocket, randomPort, sizeof(randomPort), 0, (struct sockaddr *)&server, &slen) < 0){
        cout << "Client: Error in receiving random port.\n";
        exit(0);
    }
    close(mysocket);

    //new socket with random port
    if((mysocket=socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        cout << "Client: Problem in socket creation.\n";
        exit(0);
    }

    stringstream temp2(randomPort);
    temp2 >> serverPort;  

    memset((char *)&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(serverPort);
    bcopy((char *)s->h_addr, 
    (char *)&server.sin_addr.s_addr,
    s->h_length);

    int sequenceNums[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
    ifstream file(argv[3]);
    char buffer[30];
    char spacket[100];

    file.read(buffer, 30);
    packet myPacket = packet(1, 0, sizeof(buffer), buffer);
    myPacket.printContents();
    myPacket.serialize(spacket);

    if (sendto(mysocket, &spacket, sizeof(spacket), 0, (struct sockaddr *)&server, slen) < 0){
        cout << "Client: Problem in sending payload.\n";
        exit(0);
    }

    close(mysocket);
}