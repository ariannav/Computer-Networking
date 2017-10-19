/* Stepping Stone

ss [-p port]
    port is optional, have a default.
*/
#include "awget.h"

void ss_begin(int port);
void get_ip(char* ip);

int main(int argc, char *argv[]){
  int port;
  if(argc == 1){
    port = 7014;
  }
  else if(argc == 3){
    port = atoi(argv[2]);
  }
  else{
    printf("Usage: $./ss [-p port]\n");
  }

  ss_begin(port);

}


void ss_begin(int port){
  struct sockaddr_in server;

  //Creating socket
  int sockit = socket(AF_INET, SOCK_STREAM, 0);
  if(sockit == -1){
    printf("Error creating socket. Exiting ss program.\n");
    exit(1);
  }

  //Making socket reusable.
  setsockopt(sockit, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

  char ip[50];
  get_ip(ip);
  server.sin_addr.s_addr = inet_addr(ip);
  server.sin_family = AF_INET;
  server.sin_port = htons(port);

  //Connecting to a remote server;
  if(bind(sockit, (struct sockaddr*) &server, sizeof(server)) < 0){
      printf("Binding error. Exiting ss program.\n");
      close(sockit);
      exit(1);
  }

  printf("ss <%s, %s>\n", ip, port);

  //Listening for connection from last SS.
  if(listen(sockit, 1) < 0){
    printf("Error listening for incoming connection. Exiting program.\n");
    close(sockit);
    exit(1);
  }

  struct sockaddr_in client;
  socklen_t size = sizeof(client);
  int last_ss_fd;
  if((last_ss_fd = accept(sockit, (struct sockaddr*) &client, &size)) < 0){
    printf("Last stepping stone not accepted. Exiting program.\n");
    close(sockit);
    exit(1);
  }

  //Receive list of stepping stones
  struct ss_packet packet;
  if(recv(last_ss_fd, packet, sizeof(packet), 0) < 0){
    printf("Could not receive packet from last ss. Exiting program.\n");
    close(sockit);
    exit(1);
  }

  //Process packet, and share URL/Future SS list, if num_steps = 0, fetch the file.
  printf("Request: %s\n", packet.url);

  int cl_empty = 0;
  if(packet.num_steps == 0){
    printf("Chainlist is empty.\n");
    cl_empty = 1;
  }
  else{
    printf("Chainlist is\n");
  }

  //Printing chainlist.
  for(int i = 0; i< packet.num_steps; i++){
    printf("<%s, %s>\n", packet.steps[i].ip, packet.steps[i].port);
  }

  //Selecting next ss if there are more. 


  // fd_set active_fd, read_fd;
  // FD_ZERO (&active_fd);
  // FD_SET (sockit, &active_fd);
  //
  // while(1){
  //   read_fd = active_fd;
  //   if((select(FD_SETSIZE, &read_fd, )))
  // }

  /*
  General set-up with a socket. Needs to be a client and a server. One on each machine.
  Use SELECT() here.

  Print hostname and port it is running on.
  Listen for connections.

  Connection arrives, reads URL and chain info.
              If chain info is empty, ss uses system() to issue wget.
                                      reads file & transmits it back to previous ss
                                      ss tears down connection, erases local copy
                                      listens for more requests
              If chain info not empty, use rand() like in awget.c to select next ss.
                                      connects to next ss
                                      remove next ss from chainlist
                                      sends URL and chain list, like in awget.c
                                      waits for file
                                      relays file to previous ss
                                      tears down connection and goes back to listening.

  Clean up and close connections.
  */



}

void get_ip(char* ip){
    char hostname[255];
    gethostname(hostname, 255);

    //Get IP Address from hostname
    struct addrinfo hints, *info, *rough_ip;
    struct sockaddr_in *decoy_server;
    int zero;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if((zero = getaddrinfo(hostname, "7014", &hints, &info)) != 0){
        printf("Problem converting hostname to IP address.\n");
        exit(1);
    }

    //Here, I am cycling through the IP addresses to find the first available one.
    for(rough_ip = info; rough_ip != NULL; rough_ip = rough_ip->ai_next){
        decoy_server = (struct sockaddr_in*) rough_ip->ai_addr;
        strcpy(ip, inet_ntoa(decoy_server->sin_addr));
    }

    freeaddrinfo(info);
}

