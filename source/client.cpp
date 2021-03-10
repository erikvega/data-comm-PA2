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
#include <vector>

#include "packet.cpp"

using namespace std;


int main(int argc, char *argv[]){
    struct hostent *s;
    s = gethostbyname(argv[1]);
    struct sockaddr_in server;
    int mysocket, handshakePort, serverPort;
    socklen_t slen = sizeof(server);
    char randomPort[100];
    
    //handshake
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

    ifstream file(argv[3]);

    //storing contents of the file as a string
    stringstream stringBuff;
    file >> noskipws;
    stringBuff << file.rdbuf();
    string fileString = stringBuff.str();
    file.close();

    //initializing file contents and getting size of fileString
    int seqNum = 0;
    int fileSize = fileString.size();

    //creating file to keep logs of seqnum and acks
    ofstream seqNumLog("clientseqnum.log", ios_base::out | ios_base::trunc);
    ofstream ackLog("clientack.log", ios_base::out | ios_base::trunc);

    char spacket[512];
    int counter = 0;

    //this loop creates all of the packets in advance, serializes them, and stores them in the vector 
    //seriliazedPackets. after all the packets are created and serialized, we will send them to the server
    vector <string> seriliazedPackets;
    while(1){
        int charactersLeft = fileSize - counter;
        string data;
        int i = 0;
        int dataLength = 0;

        
        if (charactersLeft == 0){               
            break;                               //no more charcters left
        }else if(charactersLeft >= 30){
            while (i < 30){
                data += fileString[counter];   
                i++;                            //more than 30 characters left
                counter++;
            }
            dataLength = 30;
        }else{                                 
            while (i < charactersLeft){
                data += fileString[counter];
                i++;                           //less than 30 characters left
                counter++;
            }
            dataLength = charactersLeft;
        }
        
        //creating packet with data from loop
        char *dataCString = (char*)data.c_str();
        packet *myPacket = new packet(1, seqNum, dataLength + 7, dataCString);

        //serializing packet and storing it in spacket
        memset(spacket, 0, sizeof(spacket));
        myPacket->serialize(spacket);

        //appending spacket to our vector
        seriliazedPackets.push_back(spacket);

        seqNum++;
        //resetting sequence numbers so that they are not greater than 7
        if(seqNum > 7){
            seqNum = 0;
        }

    }

    //creating eot packet and appending it to the back of the vector
    packet *eot = new packet(3, seqNum, 0, NULL);
    memset(spacket, 0, sizeof(spacket));
    eot->serialize(spacket);
    seriliazedPackets.push_back(spacket);

    //looping through the vector to send the serialized packets one at a time
    for(int i = 0; i < seriliazedPackets.size(); i++){
        memset(spacket, 0, sizeof(spacket));
        strcpy(spacket, seriliazedPackets[i].c_str());

        if (sendto(mysocket, spacket, sizeof(spacket), 0, (struct sockaddr *) &server, slen) < 0) {
          cout << "Error in sendto function.\n";
          exit(0);
        }

        packet *sequencePacket = new packet(0, 0, 0, spacket);
        sequencePacket->deserialize(spacket);
        seqNumLog << sequencePacket->getSeqNum() << endl;

        memset(spacket, 0, sizeof(spacket));
        char received[512];
        if (recvfrom(mysocket, spacket, sizeof(spacket), 0, (sockaddr*)&server, &slen) > 0){
            strcpy(received, spacket);

            packet *receivedPacket = new packet(0, 0, 0, spacket);
            receivedPacket->deserialize(spacket);

            packet *sequencePacket = new packet(0, 0, 0, received);

            switch (receivedPacket->getType()){
                //a packet with type 2 means a data packet. store the the seqnum in the ack log and continue the loop
                case 0: {
                    sequencePacket->deserialize(received);
                    ackLog << (sequencePacket->getSeqNum()) << endl;
                    continue;
                }
                //a packet with type 2 means an eot from the server to the client. store the seqnum in the ack log and break the loop
                case 2: {
                    sequencePacket->deserialize(received);
                    ackLog << (sequencePacket->getSeqNum()) << endl;
                    break;
                }
            }
        }else{
            cout << "Failed to receive after sending packet." << endl;
            exit(0);
        }

    
    }      
    seqNumLog.close();
    ackLog.close();
    close(mysocket);
    return 0;
}