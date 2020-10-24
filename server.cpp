// Taylor Morgan - tm2045
// Khalil Markham - km1783

// sources
// lecture notes and recordings, code from previous programming assignment
// https://github.com/8tiqa/go-back-n-udp

#include <stdlib.h>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <string.h>
#include <fstream>
#include <arpa/inet.h>
#include <netdb.h>
#include "packet.h"
#include "packet.cpp"
#include <unistd.h>

using namespace std;

int main(int argc, char *argv[]){

  struct sockaddr_in server;
  struct sockaddr_in client;
  int mysocket = 0;
  socklen_t clen = sizeof(client);
  char payload[512];
  char s_ack[512];
  int port = atoi(argv[1]);
  int seqnum = 0;
  packet *window[7];

  // create socker
  if ((mysocket=socket(AF_INET, SOCK_DGRAM, 0))==-1)
    cout << "Error in socket creation.\n";

  memset((char *) &server, 0, sizeof(server));
  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  server.sin_addr.s_addr = htonl(INADDR_ANY);

  // bind socket
  if (bind(mysocket, (struct sockaddr *)&server, sizeof(server)) == -1)
    cout << "Error in binding.\n";

  // create output files
  ofstream arrival("arrival.log");
  ofstream output("output.txt");

  while (1) {

    if (recvfrom(mysocket, payload, 40, 0, (struct sockaddr *)&client, &clen)==-1)
      cout << "Failed to receive.\n";

    // deserialize payload and write to fileName
    packet rpack(1, seqnum, 40, payload);
    rpack.deserialize(payload);
    output << rpack.getData();

    // for debugging
    cout << "rpack has seqnum " << rpack.getSeqNum() << endl;
    cout << "got payload " << payload << endl;

    // write seqnum to arrival.log
    seqnum = rpack.getSeqNum(); 	// TODO: figure out if this is dangerous
    arrival << seqnum << "\n";

    // TODO: BREAK ON EOT - should i do this here or elsewhere
    if (rpack.getType() == 3)
      break;

    // create ack packet
    packet ack(0, seqnum, 0, NULL);
    ack.serialize(s_ack);

    if (sendto(mysocket, s_ack, 40, 0, (struct sockaddr *)&client, clen)==-1)
      cout << "Error in sendto function.\n";
    cout << "sent ack with seqnum " << seqnum << endl;

  }

  // TODO: send EOT ack
  packet eot_ack(2, seqnum, 0, NULL);
  eot_ack.serialize(s_ack);

  if (sendto(mysocket, s_ack, 40, 0, (struct sockaddr *)&client, clen)==-1)
    cout << "Error in sendto function.\n";
  cout << "sent EOT ack" << endl;

  // close output files and socket
  arrival.close();
  output.close();
  close(mysocket);

  return 0;

}
