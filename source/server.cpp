// erik vega
// emv76
// data comm networks pa2

#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAXLINE 1000

#include "packet.cpp"

using namespace std;

int main(int argc, char *argv[]){
    struct sockaddr_in server, client;
    int mysocket, handshakePort, rPort;
    char const *randomPort;
    char handshakeMsg[100];
    socklen_t clen = sizeof(client);

    //stage 1: handshake
    stringstream temp(argv[1]);
    temp >> handshakePort;

    //create the UPD socket
    if ((mysocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        cout << "Server: Problem in socket creation.\n";
    }

    memset((char *)&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(handshakePort);

    if (bind(mysocket, (struct sockaddr *)&server, sizeof(server)) < 0){
        cout << "Server: Problem in binding (handshake).\n";
        exit(0);
    }

    if (recvfrom(mysocket, handshakeMsg, sizeof(handshakeMsg), 0, (struct sockaddr *)&client, &clen) < 0){
        cout << "Server: Failed to receive handshake message.\n";
        exit(0);
    }
    else{
        //checking to see if our handshake message is what we expect it to be
        if (strcmp(handshakeMsg, "1234") == 0){
            // documentation for this formula: https://stackoverflow.com/questions/7560114/random-number-c-in-some-range
            rPort = 1024 + (rand() % (65535 - 1024 + 1));
            randomPort = to_string(rPort).c_str();
            if (sendto(mysocket, randomPort, MAXLINE, 0, (struct sockaddr *)&client, clen) < 0){
                cout << "Server: Error in sending random port.\n";
                exit(0);
            }
        }
    }
    close(mysocket);
    
    //new socket with random port
    if ((mysocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        cout << "Server: Problem in socket creation.\n";
        exit(0);
    }

    memset((char *)&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(rPort);

    if (bind(mysocket, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        cout << "Server: Problem in binding (random port).\n";
        exit(0);
    }

    ofstream arrivalLog("arrival.log", ios_base::out | ios_base::trunc);
    bool loopFlag = true;
    char spacket[512];
    string data;
    ofstream file;
    //creates file if not there and empties contents if one already exists with the same name
    file.open(argv[2], ofstream::trunc);

    while (loopFlag){
        memset(spacket, 0, sizeof(spacket));
        
        if (recvfrom(mysocket, spacket, sizeof(spacket), 0, (struct sockaddr *)&client, &clen) > 0){
            packet *receivedPacket = new packet(0, 0, 30, spacket);
            receivedPacket->deserialize(spacket);

            //storing sequence number of packets that arrived
            arrivalLog << receivedPacket->getSeqNum() << endl;

            switch (receivedPacket->getType()){
                //a packet with type 2 is a data packet. append contents to data string so we can stream into file later
                case 1: {
                    data += receivedPacket->getData();

                    packet *ack = new packet(0, receivedPacket->getSeqNum(), 0, NULL);

                    memset(spacket, 0, sizeof(spacket));
                    ack->serialize(spacket);

                    if (sendto(mysocket, spacket, sizeof(spacket), 0, (struct sockaddr *) &client, clen) == -1){
                        cout << "Error in sending ack.\n";
                        exit(0);
                    }
                    
                    break;
                }
                //a packet with type 3 is an eot from the client to the server. send an eot back to server and break the loop
                case 3: {
                    packet *endConnection = new packet(2, receivedPacket->getSeqNum(), 0, NULL);

                    memset(spacket, 0, sizeof(spacket));
                    endConnection->serialize(spacket);
                    
                    if (sendto(mysocket, spacket, sizeof(spacket), 0, (struct sockaddr *) &client, clen) == -1){
                        cout << "Error in sending ack.\n";
                        exit(0);
                    }

                    loopFlag = false;
                    break;
                }
                //if something goes wrong we just break the loop
                default: {
                    loopFlag = false;
                    break;
                }
            }
        }else{
            cout << "Falied to receive spacket.\n";
            exit(0);
        }
    }

    //data should hold all the contents of the file that was sent over. stream data into output file
    file << data;
    file.close();
    arrivalLog.close();
    
    close(mysocket);
    return 0;
}