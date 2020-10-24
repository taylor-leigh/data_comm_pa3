// Taylor Morgan - tm2045
// Khalil Markham - km1783

// sources
// lecture notes and recordings, code from previous programming assignment
// http://stackoverflow.com/questions/29395081/break-a-file-into-chunks-and-send-it-as-binary-from-client-to-server-in-c-using
// https://mycodecamp.blogspot.com/2019/03/c-program-to-implement-go-back-n.html
// https://tutorialspoint.com/c-standard-library/c_function_difftime.htm
// https://w3schools.com/cpp/cpp_files.asp
// https://github.com/8tiqa/go-back-n-udp
// https://cs.cmu.edu/~srini/15-441/F01.full/www/assignments/P2/htmlsim_split/node12.html

#include <stdlib.h>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fstream>
#include <arpa/inet.h>
#include "packet.h"
#include "packet.cpp"
#include <math.h>
#include <sys/time.h>
#include <unistd.h>

using namespace std;

int main(int argc, char *argv[]){

  struct hostent *s;
  s = gethostbyname(argv[1]);

  struct sockaddr_in server;
  int mysocket = 0;
  socklen_t slen = sizeof(server);
  char payload[37];
  char spayload[37];	// for the serialized payload
  char r_ack[37];
  int port = atoi(argv[2]);
  int base = 0;
  int N = 7;	// window size is 7
  int seqnum = 0;
  //packet *window[N];	// a packet array for the window
  int a_seqnum = 0;
  int timeout = 2;

  // for debugging
  //cout << "using port " << port << endl;

  // create socket
  if ((mysocket=socket (AF_INET, SOCK_DGRAM, 0))==-1)
    cout << "Error in creating socket.\n";

  // create log files
  ofstream clientseqnum("clientseqnum.log");
  ofstream clientack("clientack.log");

  // get name of input file
  char* filename;
  filename = argv[3];

  // open input file
  ifstream file_to_send;
  file_to_send.open(filename);
  //cout << "opened file " << filename << endl;

  memset((char *) &server, 0, sizeof(server));
  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  bcopy((char *)s->h_addr,
        (char *)&server.sin_addr.s_addr,
	s->h_length);

  // split the file into packets, serialize, send, and wait for acks
  while (file_to_send.peek() != EOF) {

    if (seqnum <= base + N) {	// checks if window is full

      // splits file into 30-char chunks
      for (int i = 0; i < 30; i++) {
        char nextchar;
        if (file_to_send.peek() != EOF) {
          nextchar = file_to_send.get();
        } else {
          file_to_send.close();
        }
        payload[i] = nextchar;
      }

      // put 30-char chunks into packets & serialize
      packet pack(1, seqnum, 37, payload);
      pack.serialize(spayload);

      // send payload to server
      if (sendto(mysocket, spayload, 37, 0, (struct sockaddr *)&server, slen)==-1)
        cout << "Error in sendto function.\n";
      // put seqnum of sent packet into clientseqnum.log + newline
      clientseqnum << seqnum << "\n";
      cout << "sent payload with seqnum " << seqnum << endl;

      // add packet to window
      //window[seqnum] = &pack;	// TODO: do i even need this ?

      // set timeout to 2 seconds on first transmitted packet
      if (base == seqnum)
        if (setsockopt(mysocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
          //cout << "timeout!" << endl;	//oops why is this printing every time
        }
      
      if (seqnum < base + N) {
        seqnum++;
      } else {
         seqnum = 0;
      }

      // listen for acks from server
      if (recvfrom(mysocket, r_ack, 30, 0, (struct sockaddr *)&server, &slen)==-1)
        cout << "Error getting ack" << endl;

      // get seqnum of ack packet
      packet a_pack(0, a_seqnum, 0, r_ack);
      a_pack.deserialize(r_ack);
      a_seqnum = a_pack.getSeqNum();

      // add to clientack file then remove from the window array
      clientack << a_seqnum << "\n";
      cout << "got ack " << a_seqnum << endl;

      //delete window[0]; 	//this feels horribly wrong	
    }
  }

  // create and send EOT packet
  packet EOT(3, seqnum, 0, NULL);
  EOT.serialize(spayload);
  if (sendto(mysocket, spayload, 30, 0, (struct sockaddr *)&server, slen)==-1)
    cout << "Error sending EOT packet." << endl;
  clientseqnum << seqnum << "\n";
  cout << "sent EOT packet" << endl;

  // listen for EOT ack
  if (recvfrom(mysocket, r_ack, 30, 0, (struct sockaddr *)&server, &slen)==-1)
    cout << "Error getting EOT ack" << endl;
  
  // add EOT ack data to clientack.log
  packet a_pack(0, a_seqnum, 0, r_ack);		// this should reset type to 2
  a_pack.deserialize(r_ack);
  a_seqnum = a_pack.getSeqNum();
  clientack << a_seqnum << "\n";

  // test type to make sure the ack received is an EOT ack
  if (a_pack.getType() == 2) {
    cout << "got EOT ack" << endl; 
    // close log files and socket
    clientseqnum.close();
    clientack.close();
    close(mysocket);
  }

  return 0;

}
