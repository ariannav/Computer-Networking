//Router.c
//Author: Arianna Vacca
//CS457 P3

//Includes
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>
#include <limits.h>
#include <errno.h>
#include "manager.h"

//Functions
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

//Class Variables
struct router_packet me;
char ip[50];
int udp_ports[100];
struct links all_edges[100];
struct routing_table rt;
FILE* log_file;
int udp_s;
int distance[100];
struct packet_src_dest confirmation[1000];
int confirm;

int main(int argc, char* argv[]){
    //Get IP
    get_ip(ip);
    //strcpy(ip, "127.0.0.1");

    //Get UDP port.
    int udp_port;
    srand(atoi(argv[0]));
    do{
        udp_port = (rand() % 8980) + 1015;

        struct sockaddr_in si_me;

        //create a UDP socket
        if ((udp_s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
            printf("Router %d: Parent socket in initial contact failed.\n", me.node_num);
            exit(1);
        }

        // zero out the structure
        memset((char *) &si_me, 0, sizeof(si_me));

        si_me.sin_family = AF_INET;
        si_me.sin_port = htons(udp_port);
        si_me.sin_addr.s_addr = inet_addr(ip);

        //bind socket to port
        if( bind(udp_s, (struct sockaddr*)&si_me, sizeof(si_me)) == -1){
            udp_port = 7014;
        }
    }while(udp_port == 7014);

    client_connect(udp_port);
}

void client_connect(int udp_port){
    char ip[50];
    get_ip(ip);
    struct sockaddr_in server;

    int sockit = socket(AF_INET, SOCK_STREAM, 0);
    if(sockit == -1){
        printf("Router: Could not create socket!\n");
        exit(1);
    }

    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_family = AF_INET;
    server.sin_port = htons(7014);

    if(inet_pton(AF_INET, ip, &(server.sin_addr))<=0){
            printf("Router: Invalid IP address given.\n");
            close(sockit);
            exit(1);
    }

    if(connect(sockit, (struct sockaddr*)&server, sizeof(server))<0){
        printf("Router: Could not connect to manager.\n");
        close(sockit);
        exit(1);
    }

    //Sending UDP port information.
    if(send(sockit, &udp_port, sizeof(int), 0)<0){
        printf("Router: Send UDP port failed.\n");
        close(sockit);
        exit(1);
    }

    if(recv(sockit, &me, sizeof(me), MSG_WAITALL) < 0){
        printf("Router: Could not receive my information (me).\n");
        close(sockit);
        exit(1);
    }

    if(me.udp_port != udp_port){
        printf("Router %d: My ports don't match!\n", me.node_num);
    }

    //Opening log file.
    char file_name[30];
    sprintf(file_name, "router%d.out", me.node_num);
    log_file = fopen(file_name,"wb");
    char line[100];
    sprintf(line, "Router number: %d, UDP port: %d, Number of edges: %d.", me.node_num, me.udp_port, me.link.num_edges);
    mlog(line);

    //Sending ready to manager
    int ready = 1;
    if(send(sockit, &ready, sizeof(int), 0)<0){
        printf("Router %d: Send 'ready' to manager failed.\n", me.node_num);
        close(sockit);
        exit(1);
    }
    mlog("Telling manager I am ready.");

    //Receive UDP ports from neighbors.
    if(recv(sockit, &udp_ports, sizeof(udp_ports), MSG_WAITALL) < 0){
        printf("Router %d: Could not receive neighbor UDP ports.\n", me.node_num);
        close(sockit);
        exit(1);
    }
    mlog("Received acknowledgement. Commencing link establishment.");

    //printf("Router %d UDP[0]: %d, UDP[1]: %d, UDP[2]: %d.\n", me.node_num, udp_ports[0], udp_ports[1], udp_ports[2]);
    //Make first contact, establish links with neighbors.
    int master = router_initial_contact();

    //Send second ready to manager.
    if(send(sockit, &ready, sizeof(int), 0)<0){
        printf("Router %d: could not send 2nd ready to manager.\n", me.node_num);
        close(sockit);
        exit(1);
    }
    fflush(log_file);
    mlog("Telling manager I am ready to make routing table.");

    //Receive ready signal. Next, we should make routing table.
    if(recv(sockit, &confirmation, sizeof(confirmation), MSG_WAITALL) < 0){
        printf("Router %d: could not receive ready signal from manager to make routing table.\n", me.node_num);
        close(sockit);
        exit(1);
    }
    if(recv(sockit, &confirm, sizeof(int), MSG_WAITALL) < 0){
        printf("Router %d: could not receive ready signal from manager to make routing table.\n", me.node_num);
        close(sockit);
        exit(1);
    }
    mlog("Received acknowledgement. Commencing LSP sending, dijkstras, and creating forwarding table.");

    //Send LSP to neighbors and have them forwarded.
    router_secondary_contact(master);
    //Run dijkstras on the all_edges information. Contains the link information for each router.
    run_dijkstras();
    //Output the forwarding/output table stored in rt.
    output_routing_table();

    //Send ready to manager.
    if(send(sockit, &ready, sizeof(int), 0)<0){
        printf("Router %d: Can't tell manager I'm ready to send packets.\n", me.node_num);
        close(sockit);
        exit(1);
    }
    mlog("Telling manager I am ready to send packets.");

    send_packet(master, sockit);

    mlog("I have finished forwarding and receiving packets.");
    mlog("Goodbye!");

    close(sockit);
    fclose(log_file);
}

int router_initial_contact(){
    fflush(log_file);
    int id = fork();

    if(id < 0){
        printf("Router %d: fork() was not successful. Exiting program.\n", me.node_num);
        exit(1);
    }
    else if(id == 0){ //Child process. Should be the client, loops through number of connections. Receives and sends.
        struct sockaddr_in si_other;
        int s;
        socklen_t slen = sizeof(si_other);

        if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
            printf("Router %d: Can't make socket in child for initial contact.\n", me.node_num);
            exit(1);
        }
        char line[100];
        for(int i = 0; i < me.link.num_edges; i++){

            memset((char *) &si_other, 0, sizeof(si_other));
            si_other.sin_family = AF_INET;
            si_other.sin_addr.s_addr = inet_addr(ip);
            si_other.sin_port = htons(udp_ports[me.link.dest[i]]);

            int buf = 1;
            if (sendto(s, &buf, sizeof(int), 0, (struct sockaddr *) &si_other, slen) == -1){
                printf("Router %d: Child send from initial contact failed.\n", me.node_num);
                exit(1);
            }
            sprintf(line, "Contacting router %d on port %d.", me.link.dest[i], udp_ports[me.link.dest[i]]);
            mlog(line);

            if (recvfrom(s, &buf, sizeof(int), 0, (struct sockaddr *) &si_other, &slen) == -1){
                printf("Router %d: Child receive during initial contact failed.\n", me.node_num);
                exit(1);
            }
            sprintf(line,"Received acknowledgement from router %d.", me.link.dest[i]);
            mlog(line);
        }
        exit(0);
    }


    struct sockaddr_in si_other;
    socklen_t slen = sizeof(si_other);

    for(int i = 0; i < me.num_neighbors; i++){
        int request = 1;

        if(recvfrom(udp_s, &request, sizeof(int), 0, (struct sockaddr *) &si_other, &slen) == -1){
            printf("Router %d: Initial contact, parent, cannot receive.\n", me.node_num);
            exit(1);
        }
        mlog("Received link request from neighbor.");

        //printf("Router %d: Received something!\n", me.node_num);
        if(sendto(udp_s, &request, sizeof(int), 0, (struct sockaddr *) &si_other, slen) == -1){
            printf("Router %d: Initial contact, parent, cannot send back.\n", me.node_num);
            exit(1);
        }
    }

    //Wait for routers to exit.
    int status;
    waitpid(id, &status, 0);
    mlog("Link establishment complete.");
    return(udp_s);
    //Tell manager we are done!
}

void router_secondary_contact(int master){
    fflush(log_file);
    int id = fork();

    if(id < 0){
        printf("Router %d: Secondary fork() was not successful.\n", me.node_num);
        exit(1);
    }
    else if(id == 0){ //Child process. Should be the client, loops through number of connections. Sends then receives.
        struct sockaddr_in si_other;
        int s;
        socklen_t slen = sizeof(si_other);

        if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
            printf("Router %d: Secondary contact, can't make child socket.\n", me.node_num);
            exit(1);
        }

        //Prepare lsp
        struct lsp r_lsp;
        r_lsp.node_num = me.node_num;
        r_lsp.link = me.link;
        r_lsp.s_num = -1;
        char line[100];

        for(int i = 0; i < me.link.num_edges; i++){

            memset((char *) &si_other, 0, sizeof(si_other));
            si_other.sin_family = AF_INET;
            si_other.sin_addr.s_addr = inet_addr(ip);
            si_other.sin_port = htons(udp_ports[me.link.dest[i]]);
            sprintf(line, "LSP: send to router %d on their port %d.", me.link.dest[i], udp_ports[me.link.dest[i]]);
            mlog(line);

            r_lsp.s_num++;
            if (sendto(s, &r_lsp, sizeof(r_lsp), 0, (struct sockaddr *) &si_other, slen) == -1){
                printf("Router %d: LSP send from in child failed, secondary contact.\n", me.node_num);
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
    char line[100];
    while(num_recieved != me.num_routers-1){

        //Received LSP from another router
        if(recvfrom(master, &o_lsp, sizeof(o_lsp), 0, (struct sockaddr *) &si_other, &slen) == -1){
            printf("Router %d: Failed to receive from other router in parent, secondary contact.\n", me.node_num);
            exit(1);
        }
        sprintf(line, "Received lsp from router %d via neighbor.",o_lsp.node_num);
        mlog(line);

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
    mlog("Finished accepting LSP packets!");
    //Wait for routers to exit.
    int status;
    waitpid(id, &status, 0);

    //Tell manager we are done!
}

void forward(struct lsp f_lsp, int r_port, int s){
    struct sockaddr_in si_other;
    socklen_t slen = sizeof(si_other);
    char line[100];

    for(int i = 0; i < me.link.num_edges; i++){

        memset((char *) &si_other, 0, sizeof(si_other));
        si_other.sin_family = AF_INET;
        si_other.sin_addr.s_addr = inet_addr(ip);
        si_other.sin_port = htons(udp_ports[me.link.dest[i]]);


        if(si_other.sin_port != r_port && f_lsp.node_num != me.link.dest[i]){
            sprintf(line, "Forwarding lsp from router %d to router %d.", f_lsp.node_num, me.link.dest[i]);
            mlog(line);
            fflush(log_file);

            if(sendto(s, &f_lsp, sizeof(f_lsp), 0, (struct sockaddr *) &si_other, slen) == -1){
                //Doesn't matter if this fails, because that means the receiving router has received LSPs from all routers.
                //printf("Router %d: Forwarding failed. %s\n", me.node_num, strerror(errno));
                //exit(1);
            }
        }
    }
}

void run_dijkstras(){
    mlog("Beginning dijkstras' algorithm.");
    int previous[me.num_routers];
    int rem_nodes[me.num_routers];

    for(int i = 0; i < me.num_routers; i++){
        rem_nodes[i] = i;
        previous[i] = -1;
        distance[i] = INT_MAX;
    }

    distance[me.node_num] = 0;
    previous[me.node_num] = me.node_num;

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
            rt.next_node[r] = me.node_num;
            continue;
        }
        else{
            rt.next_node[r] = find_prev(r, previous);
        }
    }
    mlog("Finished dijkstras'. Created forwarding table.");
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
    sprintf(line, "\tRouting table: \n\t|\tDestination\t|\tNext Node\t|\tTotal Distance\t|\n");
    fwrite(line, sizeof(char), strlen(line), log_file);

    for(int i = 0; i< me.num_routers; i++){
        sprintf(line, "\t|\t%d\t\t\t|\t%d\t\t\t|\t%d\t\t\t\t|\n", i, rt.next_node[i], distance[i]);
        fwrite(line, sizeof(char), strlen(line), log_file);
    }

    sprintf(line, "\t\t------\t\t\t------\t\t\t------\n");
    fwrite(line, sizeof(char), strlen(line), log_file);
}

void send_packet(int master, int sockit){
    fflush(log_file);
    int id = fork();

    if(id < 0){
        printf("Router %d: fork() in packet sending was not successful.\n", me.node_num);
        exit(1);
    }
    else if(id == 0){ //Child process. Should be the client, loops through number of connections. Sends then receives.
        struct sockaddr_in si_other;
        int s;
        socklen_t slen = sizeof(si_other);
        char line[100];
        struct packet_src_dest packet;

        if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
            printf("Router %d: Can't make socket in child, packet sending.\n", me.node_num);
            exit(1);
        }

        if(recv(sockit, &packet, sizeof(packet), MSG_WAITALL) < 0){
            printf("Router %d: In child, could not receive packet from manager, packet sending.\n", me.node_num);
            close(sockit);
            exit(1);
        }
        while(packet.dest != -1){
            fflush(log_file);
            if(packet.dest == me.node_num){
                mlog("Received packet from manager for me.");
                fflush(log_file);

                int confirma = 1;
                if(send(sockit, &confirma, sizeof(int), 0)<0){
                    printf("Router %d: Confirmation send failed.\n", me.node_num);
                    close(sockit);
                    exit(1);
                }

                if(recv(sockit, &packet, sizeof(packet), MSG_WAITALL) < 0){
                    printf("Router %d: Child, packet sending, manager receive err (2).\n", me.node_num);
                    close(sockit);
                    exit(1);
                }

                continue;
            }

            memset((char *) &si_other, 0, sizeof(si_other));
            si_other.sin_family = AF_INET;
            si_other.sin_addr.s_addr = inet_addr(ip);
            si_other.sin_port = htons(udp_ports[rt.next_node[packet.dest]]);

            sprintf(line, "Got packet from manager. Sending packet to router %d via router %d.", packet.dest, rt.next_node[packet.dest]);
            mlog(line);

            if (sendto(s, &packet, sizeof(packet), 0, (struct sockaddr *) &si_other, slen) == -1){
                printf("Router %d: Child, packet sending. Send failed.\n", me.node_num);
                exit(1);
            }

            int confirma = 1;
            if(send(sockit, &confirma, sizeof(int), 0)<0){
                printf("Router %d: Confirmation send failed.\n", me.node_num);
                close(sockit);
                exit(1);
            }

            if(recv(sockit, &packet, sizeof(packet), MSG_WAITALL) < 0){
                printf("Router %d: Child, packet sending, manager receive err (2).\n", me.node_num);
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
            printf("Router %d: Parent, packet sending, receive failure.\n", me.node_num);
            exit(1);
        }

        int exists = 0;
        for(int i = 0; i < confirm; i++){
            if(confirmation[i].src == packet.src && confirmation[i].dest == packet.dest){
                exists = 1;
            }
        }

        if(!exists && packet.dest != -1){
            continue;
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

            sprintf(line, "Forwarding packet from router %d to router %d via router %d.", packet.src, packet.dest, rt.next_node[packet.dest]);
            mlog(line);

            if (sendto(master, &packet, sizeof(packet), 0, (struct sockaddr *) &si_other, slen) == -1){
                printf("Router %d: Packet sending, parent, send to router failed.\n", me.node_num);
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
    packet.src = me.node_num;
    packet.dest = -1;

    mlog("Telling neighbors there are no more packets from me.");
    for(int i = 0; i < me.num_routers; i++){

        memset((char *) &si_other, 0, sizeof(si_other));
        si_other.sin_family = AF_INET;
        si_other.sin_addr.s_addr = inet_addr(ip);
        si_other.sin_port = htons(udp_ports[i]);

        if(i == me.node_num){
            continue;
        }
        if(sendto(s, &packet, sizeof(packet), 0, (struct sockaddr *) &si_other, slen) == -1){
            printf("Router %d: Failed telling routers I'm done sending packets.\n", me.node_num);
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



