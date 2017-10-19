/*Reader*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "awget.h"

/*awget <URL> [-c chainfile]
            or local chaingang.txt
            or error and exit.
*/
void ss_start(char* url, char* cgang);
void client_connect(char* ip, char* port, struct ss_packet packet);

int main(int argc, char* argv[]){
  char* url;
  char* cgang = "./chaingang.txt";

  if(argc==2){
    url = argv[1];
    //Grab chaingang.txt
    ss_start(url, cgang);
  }
  else if(argc==4){
    url = arg[1];
    cgang = arg[3];
    //Do something with chaingang and URL
    ss_start(url, cgang);
  }
  else{
    //Usage error.
    printf("Usage: $./awget <URL> [-c chainfile]\n");
  }

}

void ss_start(char* url, char* cgang){
  printf("Request: %s\n", url);

  FILE *fptr;
  char line[50];
  //Open cgang file.
  if((fptr = fopen(cgang,"r")) == NULL){
    printf("Problem opening file %s . Exiting program.\n", cgang);
    exit(1);
  }

  //Get number of stepping stones.
  fgets(line, 50, (FILE*)fptr);
  int num_ss = atoi(line);

  struct ss_packet packet;
  packet.num_steps = num_ss;

  //Populate & Print Chainlist
  printf("Chainlist is: \n");
  for(int i = 0; i< num_ss; i++){
    fscanf(fptr, "%s", line);
    strcpy(packet.steps[i].ip, line);
    fscanf(fptr, "%s", line);
    strncpy(packet.steps[i].port, line, 6);
    printf("<%s, %s>\n", packet.steps[i].ip, packet.steps[i].port);
  }

  //Pick a random ss
  srand(time(NULL));
  int r = rand() % num_ss;
  char ip[50]; strcpy(ip, packet.steps[r].ip);
  char port[6]; strcpy(port, packet.steps[r].port);
  printf("Next SS is <%s, %s>\n", ip, port);

  //Remove ss from packet.
  num_ss--;
  for(int j = r; j<num_ss; j++){
      packet.steps[j] = packet.steps[j+1];
  }
  packet.num_steps--;

  //Connect to ss as client
  client_connect(ip, port, packet);

}

void client_connect(char* ip, char* port, struct ss_packet packet){
  struct sockaddr_in server;

  int sockit = socket(AF_INET, SOCK_STREAM, 0);
  if(sockit == -1){
    printf("Could not create socket! Exiting program.\n");
    exit(1);
  }

  server.sin_addr.s_addr = inet_addr(ip);
  server.sin_family = AF_INET;
  server.sin_port = htons(atoi(port));

  if(inet_pton(AF_INET, ip, &(server.sin_addr))<=0){
        printf("Invalid IP address given.\n");
        close(sockit);
        exit(1);
  }

  if(connect(sockit, (struct sockaddr*)&server, sizeof(server))<0){
    printf("Connection failed. Please make sure you are using a valid IP address and port number.\n");
    close(sockit);
    exit(1);
  }

  //Sending the ss information.
  if(send(sockit, &packet, sizeof(packet), 0)<0){
    printf("Send failed.");
    close(sockit);
    exit(1);
  }

  //Waiting for the file.
  printf("Waiting for file...\n");

  //Process file. First, receive the size of the file.
  char file_size[10];
  int f_size = 0;
  char* file_contents;
  if(recv(sockit, file_size, 10, 0)<0){
    printf("Could not recieve file information.\n");
    close(sockit);
    exit(1);
  }


  f_size = ntohl(atoi(file_size));
  if(f_size >0){
    file_contents = (char*) malloc(f_size);

    printf("...\n");
    if((recv(sockit, file_contents, f_size, MSG_WAITALL))<0){
      printf("Could not recieve file.\n");
      close(sockit);
      exit(1);
    }
  }

  close(sockit);
  sleep(1);

  //Processing file name.
  char* file_name = strrchr(packet.url, '/');
  if(file_name == NULL){
    printf("No file name given, defaulting to index.html");
    file_name = "index.html";
  }
  else{
    file_name++;
  }

  printf("Received file %s\n", file_name);

  //Writing to local file.
  int fptr;
  //Open cgang file.
  if((fptr = open(file_name, (O_CREAT | O_TRUNC | O_WRONLY), (S_IRGRP | S_IWGRP | S_IXGRP))) < 0){
    printf("Problem creating file %s . Exiting program.\n", file_name);
    exit(1);
  }

  //Write data to file.
  int success;
  if((success = write(fptr, &file_contents, f_size))<0){
      printf("Error writing to file %s. Exiting program.\n", file_name);
      close(fptr);
      exit(1);
  }

  //Quit. Free memory/close any connections. Close fptr.
  close(fptr);
  free(file_contents);
  printf("Goodbye!");
}

