/* Stepping Stone

ss [-p port]
    port is optional, have a default.

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