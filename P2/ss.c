/* Stepping Stone

ss [-p port]
    port is optional, have a default.
*/
#include "awget.h"

void ss_begin(int port);
void get_ip(char* ip);
void sighandler(int sig);

int sockit;

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
  sockit = socket(AF_INET, SOCK_STREAM, 0);
  if(sockit == -1){
    printf("Error creating socket. Exiting ss program.\n");
    exit(1);
  }

  //Signal handler
  signal(SIGINT, sighandler);

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

  printf("ss <%s, %s>:\n", ip, port);

  while(1){
    //Listening for connection from last SS.
    if(listen(sockit, 1) < 0){
      printf("Error listening for incoming connection. Exiting program.\n");
      close(sockit);
      exit(1);
    }

    //Set up & using select.
    fd_set readfds;
    int max_sd, activity, i;

    FD_ZERO(&readfds);
    FD_SET(sockit, &readfds);
    max_sd = sockit;

    if((activity = select(max_sd+1; &readfds, NULL, NULL< NULL))<0){
      printf("Select failed. Exiting program.");
      close(sockit);
      exit(1);
    }

    //Create client, and accept connection.
    struct sockaddr_in client;
    socklen_t size = sizeof(client);
    int last_ss_fd;

    //If the sockit is set, then we know that we should accept a connection on there.
    //At this point, only the previous ss will be connecting to this socket.
    if(FD_ISSET(sockit, &readfds)){
      if((last_ss_fd = accept(sockit, (struct sockaddr*) &client, &size)) < 0){
        printf("Last stepping stone not accepted. Exiting program.\n");
        close(sockit);
        exit(1);
      }
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
    if(!cl_empty){
      //Keep passing on.
      srand(time(NULL));
      int r = rand() % num_ss;
      char ip[50]; strcpy(ip, packet.steps[i].ip);
      char port[6]; strcpy(port, packet.steps[i].port);
      printf("Next SS is <%s, %s>\n", ip, port);

      //Remove ss from packet.
      num_ss--;
      for(int j = r; j<num_ss; j++){
          packet.steps[j] = packet.steps[j+1];
      }
      packet.num_steps--;
      //TODO: Go to the next step.
    }
    else{
      //Client list is empty, Process file name
      char* file_name = strrchr(packet.url, '/');
      if(file_name == NULL){
        printf("No file name given, defaulting to index.html");
        file_name = "index.html";
      }
      else{
        file_name++;
      }

      //wget(url)
      char command[550];
      long int f_size = 0;
      struct stats file_stats;
      printf("Issuing wget for file %s\n", file_name);
      sprintf(command, "wget -q %s", packet.url);
      system(command);

      //Getting file size.
      if(stat(file_name, &file_stats) == 0){
        f_size = file_stats.st_size;
      }
      else{
        printf("No file retrieved. Sending back empty buffer.\n");
      }

      //Getting file contents.
      char *file_contents = (char *) malloc(f_size);
      int open_file = open(file_name, O_RDONLY, (S_IRGRP | S_IWGRP | S_IXGRP));
      if(read(open_file, file_contents, file_size) < 0){
        printf("Error reading file read from wget. Exiting program.\n");
        close(sockit);
        exit(1);
      }
      close(open_file);
      
      //Removing file:
      command = "";
      sprintf(command, "rm %s", file_name);
      system(command);

      //Sending size
      char size[10];
      f_size = htonl(f_size);\
      memcpy(size, f_size, sizeof(f_size));
      if(send(last_ss_fd, size, 10, 0) < 0){
        printf("Could not send file size. Exiting program.\n");
        close(sockit);
        exit(1);
      }

      //Sending file data.
      if(send(last_ss_fd, file_contents, f_size, 0) < 0){
        printf("Could not send file. Exiting program.\n");
        close(sockit);
        exit(1);
      }
      //Done here!
      //TODO: Do I need to do anything to listen again?
    }
  }
}


//Get this computer's IP to broadcast it.
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

//Handling a signal after ctrl+c
void sighandler(int sig){
    printf(" Exiting program, closing socket.\n");
    close(sockit);
    exit(1);
}

