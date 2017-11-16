//Router.c
//Author: Arianna Vacca
//CS457 P3

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>
#include <limits.h>
#include "manager.h"

void client_connect(int udp_port);
int router_initial_contact();
void router_secondary_contact(int master);
void forward(struct lsp f_lsp, int r_port, int s);
void run_dijkstras();
int find_prev(int curr, int* previous);
void output_routing_table();
void mlog(char* usr_message);
void send_packet(int master, int sockit);
void alert(int s);

struct router_packet me;
char ip[50];
int udp_ports[100];
struct links all_edges[100];
struct routing_table rt;
FILE* log_file;

int main(int argc, char* argv[]){
    //Get IP
    get_ip(ip);

    //Get UDP port.
    int udp_port;
    do{
        srand(atoi(argv[0]));
        udp_port = (rand() % 8980) + 1015;
    }while(udp_port == 7014);

    client_connect(udp_port);
}

void client_connect(int udp_port){
    char ip[50];
    get_ip(ip);
    struct sockaddr_in server;

    int sockit = socket(AF_INET, SOCK_STREAM, 0);
    if(sockit == -1){
        printf("Could not create router socket! Exiting program.\n");
        exit(1);
    }

    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_family = AF_INET;
    server.sin_port = htons(7014);

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

    //Sending UDP port information.
    if(send(sockit, &udp_port, sizeof(int), 0)<0){
        printf("Send failed.\n");
        close(sockit);
        exit(1);
    }

    if(recv(sockit, &me, sizeof(me), MSG_WAITALL) < 0){
        printf("Could not receive packet from manager. Exiting program.\n");
        close(sockit);
        exit(1);
    }

    if(me.udp_port != udp_port){
        printf("My ports don't match!\n");
    }

    //Opening log file.
    char file_name[30];
    sprintf(file_name, "router%d.out", me.node_num);
    log_file = fopen(file_name,"wb");

    int ready = 1;
    if(send(sockit, &ready, sizeof(int), 0)<0){
        printf("Send 1 failed.");
        close(sockit);
        exit(1);
    }

    if(recv(sockit, &udp_ports, sizeof(udp_ports), MSG_WAITALL) < 0){
        printf("Could not receive packet from manager. Exiting program.\n");
        close(sockit);
        exit(1);
    }

    printf("Router %d UDP[0]: %d, UDP[1]: %d, UDP[2]: %d.\n", me.node_num, udp_ports[0], udp_ports[1], udp_ports[2]);

    int master = router_initial_contact();

    if(send(sockit, &ready, sizeof(int), 0)<0){
        printf("Send 2 failed.\n");
        close(sockit);
        exit(1);
    }

    if(recv(sockit, &ready, sizeof(int), MSG_WAITALL) < 0){
        printf("Could not receive ready signal 2 from manager. Exiting program.\n");
        close(sockit);
        exit(1);
    }

    router_secondary_contact(master);
    run_dijkstras();
    output_routing_table();

    if(send(sockit, &ready, sizeof(int), 0)<0){
        printf("Send 3 failed.\n");
        close(sockit);
        exit(1);
    }

    send_packet(master, sockit);

    close(sockit);
    fclose(log_file);
}

int router_initial_contact(){
    int id = fork();

    if(id < 0){
        printf("fork() was not successful. Exiting program.\n");
        exit(1);
    }
    else if(id == 0){ //Child process. Should be the client, loops through number of connections. Receives and sends.
        struct sockaddr_in si_other;
        int s;
        socklen_t slen = sizeof(si_other);

        if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
            printf("Can't make socket in child!\n");
            exit(1);
        }

        for(int i = 0; i < me.link.num_edges; i++){

            memset((char *) &si_other, 0, sizeof(si_other));
            si_other.sin_family = AF_INET;
            si_other.sin_addr.s_addr = inet_addr(ip);
            si_other.sin_port = htons(udp_ports[me.link.dest[i]]);
            printf("Router %d is contacting router %d on their port %d.\n", me.node_num, me.link.dest[i], udp_ports[me.link.dest[i]]);

            int buf = 1;
            if (sendto(s, &buf, sizeof(int), 0, (struct sockaddr *) &si_other, slen) == -1){
                printf("Send from in child failed.\n");
                exit(1);
            }

            if (recvfrom(s, &buf, sizeof(int), 0, (struct sockaddr *) &si_other, &slen) == -1){
                printf("Recv from in child failed.\n");
                exit(1);
            }
            printf("Router %d received acknowledgement from router %d.\n", me.node_num, me.link.dest[i]);
        }
        exit(0);
    }


    struct sockaddr_in si_me, si_other;
    int s;
    socklen_t slen = sizeof(si_other);

    //create a UDP socket
    if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
        printf("Parent socket failed.\n");
        exit(1);
    }

    // zero out the structure
    memset((char *) &si_me, 0, sizeof(si_me));

    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(me.udp_port);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);

    //bind socket to port
    if( bind(s, (struct sockaddr*)&si_me, sizeof(si_me)) == -1){
        printf("Parent bind failed.\n");
        exit(1);
    }

    for(int i = 0; i < me.num_neighbors; i++){
        int request = 1;

        //printf("Router %d, waiting to hear something.\n", me.node_num);
        if(recvfrom(s, &request, sizeof(int), 0, (struct sockaddr *) &si_other, &slen) == -1){
            printf("Failed to receive from other router.\n");
            exit(1);
        }

        //printf("Router %d: Received something!\n", me.node_num);
        if(sendto(s, &request, sizeof(int), 0, (struct sockaddr *) &si_other, slen) == -1){
            printf("Failed to send to other router.\n");
            exit(1);
        }
    }

    //Wait for routers to exit.
    int status;
    waitpid(id, &status, 0);
    return(s);
    //Tell manager we are done!
}

void router_secondary_contact(int master){
    int id = fork();

    if(id < 0){
        printf("fork() was not successful. Exiting program.\n");
        exit(1);
    }
    else if(id == 0){ //Child process. Should be the client, loops through number of connections. Sends then receives.
        struct sockaddr_in si_other;
        int s;
        socklen_t slen = sizeof(si_other);

        if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
            printf("Can't make socket in child!\n");
            exit(1);
        }

        //Prepare lsp
        struct lsp r_lsp;
        r_lsp.node_num = me.node_num;
        r_lsp.link = me.link;
        r_lsp.s_num = -1;

        for(int i = 0; i < me.link.num_edges; i++){

            memset((char *) &si_other, 0, sizeof(si_other));
            si_other.sin_family = AF_INET;
            si_other.sin_addr.s_addr = inet_addr(ip);
            si_other.sin_port = htons(udp_ports[me.link.dest[i]]);
            printf("Router %d is contacting router %d on their port %d.\n", me.node_num, me.link.dest[i], udp_ports[me.link.dest[i]]);

            r_lsp.s_num++;
            if (sendto(s, &r_lsp, sizeof(r_lsp), 0, (struct sockaddr *) &si_other, slen) == -1){
                printf("LSP send from in child failed.\n");
                exit(1);
            }
        }
        exit(0);
    }

    struct sockaddr_in si_other;
    socklen_t slen = sizeof(si_other);

    struct lsp o_lsp;
    int num_recieved = 0;
    int latest_s_num[100];
    for(int i = 0; i < me.num_routers; i++){
        all_edges[i].num_edges= 0;
        latest_s_num[i] = -1;
    }

    all_edges[me.node_num] = me.link;

    while(num_recieved != me.num_routers-1){

        //Received LSP from another router
        if(recvfrom(master, &o_lsp, sizeof(o_lsp), 0, (struct sockaddr *) &si_other, &slen) == -1){
            printf("Failed to receive from other router.\n");
            exit(1);
        }

        if(all_edges[o_lsp.node_num].num_edges == 0){   //First LSP from this router.
            all_edges[o_lsp.node_num] = o_lsp.link;
            latest_s_num[o_lsp.node_num] = o_lsp.s_num;
            num_recieved++;
            forward(o_lsp, si_other.sin_port, master);
        }
        else if(latest_s_num[o_lsp.node_num] < o_lsp.s_num){
            all_edges[o_lsp.node_num] = o_lsp.link;
            latest_s_num[o_lsp.node_num] = o_lsp.s_num;
            forward(o_lsp, si_other.sin_port, master);
        }
        //else: Throw away. Keep going.
    }

    //Wait for routers to exit.
    int status;
    waitpid(id, &status, 0);

    //Tell manager we are done!
}

void forward(struct lsp f_lsp, int r_port, int s){
    struct sockaddr_in si_other;
    socklen_t slen = sizeof(si_other);

    for(int i = 0; i < me.link.num_edges; i++){

        memset((char *) &si_other, 0, sizeof(si_other));
        si_other.sin_family = AF_INET;
        si_other.sin_addr.s_addr = inet_addr(ip);
        si_other.sin_port = htons(udp_ports[me.link.dest[i]]);

        printf("Router %d is forwarding lsp from router %d to router %d.\n", me.node_num, f_lsp.node_num, me.link.dest[i]);

        if(si_other.sin_port != r_port && f_lsp.node_num != me.link.dest[i]){
            if(sendto(s, &f_lsp, sizeof(f_lsp), 0, (struct sockaddr *) &si_other, slen) == -1){
                printf("Forwarding in router %d failed.\n", me.node_num);
                exit(1);
            }
        }
    }
}

void run_dijkstras(){
    //TODO: Implement dijstras on all_edges.
    int distance[me.num_routers];
    memset(distance, INT_MAX, sizeof(distance));
    distance[me.node_num] = 0;

    int previous[me.num_routers];
    memset(previous, -1, sizeof(previous));

    int rem_nodes[me.num_routers];
    memset(rem_nodes, -1, sizeof(rem_nodes));

    for(int i = 0; i < me.num_routers; i++){
        rem_nodes[i] = i;
    }

    int remaining_nodes = me.num_routers;
    while(remaining_nodes != 0){
        int min = INT_MAX; int min_index = 0;
        for(int a = 0; a < me.num_routers; a++){
            if(rem_nodes[a] != -1 && distance[a] < min){
                min = distance[a];
                min_index = a;
            }
        }

        //Remove min from remaining nodes.
        rem_nodes[min_index] = -1;
        remaining_nodes--;
        for(int j = 0; j < all_edges[min_index].num_edges; j++){
            int alt = distance[min_index] + all_edges[min_index].cost[j];
            if(alt < distance[all_edges[min_index].dest[j]]){
                distance[all_edges[min_index].dest[j]] = alt;
                previous[all_edges[min_index].dest[j]] = min_index;
            }
        }
    }
    //Previous now contains the node that comes before
    //rt contains next_node[100]. next_node[i] => i = destination, next_node[i] = next node to forward to.
    for(int r = 0; r < me.num_routers; r++){
        if(r == me.node_num){
            continue;
        }
        else{
            rt.next_node[r] = find_prev(r, previous);
        }
    }

    //Routing table created.
}

int find_prev(int curr, int* previous){
    if(previous[curr] == me.node_num){
        return curr;
    }
    else{
        return find_prev(previous[curr], previous);
    }
}

void output_routing_table(int* previous){
    char line[200];
    sprintf(line, "Shortest Path table: \n|\tDestination\t|\tNext Node\t|\n");
    fwrite(line, sizeof(char), strlen(line), log_file);

    for(int i = 0; i< me.num_routers; i++){
        sprintf(line, "|\t%d\t\t|\t%d\t\t|\n", i, rt.next_node[i]);
        fwrite(line, sizeof(char), strlen(line), log_file);
    }

    sprintf(line, "\t------\t\t\t------\t\n");
    fwrite(line, sizeof(char), strlen(line), log_file);
}

void send_packet(int master, int sockit){
    int id = fork();

    if(id < 0){
        printf("fork() was not successful. Exiting program.\n");
        exit(1);
    }
    else if(id == 0){ //Child process. Should be the client, loops through number of connections. Sends then receives.
        struct sockaddr_in si_other;
        int s;
        socklen_t slen = sizeof(si_other);
        char line[100];
        struct packet_src_dest packet;

        if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
            printf("Can't make socket in child!\n");
            exit(1);
        }

        if(recv(sockit, &packet, sizeof(packet), MSG_WAITALL) < 0){
            printf("Could not receive first packet instruction from manager. Exiting program.\n");
            close(sockit);
            exit(1);
        }
        while(packet.dest != -1){
            if(packet.dest == me.node_num){
                sprintf(line, "Received packet from manager for me.");
                mlog(line);
                continue;
            }

            memset((char *) &si_other, 0, sizeof(si_other));
            si_other.sin_family = AF_INET;
            si_other.sin_addr.s_addr = inet_addr(ip);
            si_other.sin_port = htons(udp_ports[rt.next_node[packet.dest]]);

            sprintf(line, "Received packet from me for router %d, forwarding packet to router %d.", packet.dest, rt.next_node[packet.dest]);
            mlog(line);

            if (sendto(s, &packet, sizeof(packet), 0, (struct sockaddr *) &si_other, slen) == -1){
                printf("Packet send from in child failed.\n");
                exit(1);
            }

            if(recv(sockit, &packet, sizeof(packet), MSG_WAITALL) < 0){
                printf("Could not receive packet instruction from manager. Exiting program.\n");
                close(sockit);
                exit(1);
            }
        }
        alert(s);
        exit(0);
    }

    struct sockaddr_in si_other;
    socklen_t slen = sizeof(si_other);
    struct packet_src_dest packet;
    int done = 0;
    char line[100];
    while(done < me.num_routers-1){
        //Received Packet from another router
        if(recvfrom(master, &packet, sizeof(packet), 0, (struct sockaddr *) &si_other, &slen) == -1){
            printf("Failed to receive from other router.\n");
            exit(1);
        }

        if(packet.dest == me.node_num){   //Packet is for me.
            sprintf(line, "Received packet from router %d.", packet.src);
            mlog(line);
            continue;
        }
        else if(packet.dest == -1){  //One of my neighbors is done.
            done++;
            continue;
        }
        else{
            si_other.sin_port = htons(udp_ports[rt.next_node[packet.dest]]);

            sprintf(line, "Received packet from router %d for router %d, forwarding packet to router %d.", packet.src, packet.dest, rt.next_node[packet.dest]);
            mlog(line);

            if (sendto(master, &packet, sizeof(packet), 0, (struct sockaddr *) &si_other, slen) == -1){
                printf("Packet send from parent failed.\n");
                exit(1);
            }
        }
    }

    //Wait for routers to exit.
    int status;
    waitpid(id, &status, 0);

    //Tell manager we are done!
}

void alert(int s){
    struct sockaddr_in si_other;
    socklen_t slen = sizeof(si_other);
    struct packet_src_dest packet;
    packet.src = -1;
    packet.dest = -1;

    for(int i = 0; i < me.num_routers; i++){

        memset((char *) &si_other, 0, sizeof(si_other));
        si_other.sin_family = AF_INET;
        si_other.sin_addr.s_addr = inet_addr(ip);
        si_other.sin_port = htons(udp_ports[i]);

        mlog("Telling neighbors there are no more packets from me.\n");

        if(i == me.node_num){
            continue;
        }
        if(sendto(s, &packet, sizeof(packet), 0, (struct sockaddr *) &si_other, slen) == -1){
            printf("Failed telling routers I'm done sending packets.\n");
            exit(1);
        }
    }
}

void mlog(char* usr_message){
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    char line[200];
    sprintf(line, "%d:%d:%d \t| %s\n", tm.tm_hour, tm.tm_min, tm.tm_sec, usr_message);
    fwrite(line, sizeof(char), strlen(line), log_file);
}



