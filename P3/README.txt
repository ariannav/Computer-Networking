Author: Arianna Vacca
ID: 830550508
Assignment: 457 PA3

*****************************************************
                       README
*****************************************************

COMPILE
==================
  MANAGER and ROUTER:
      make
  MANAGER:
      make manager
  ROUTER:
      make router
  CLEAN:
      make clean

RUN
==================
    - awget:  $./manager <input file>

    The url for awget must be given before the chainfile.
    The default port for the stepping stone is 7014, if no port is given.

HOW TO USE
==================
    1. Compile both files using 'make'.
    2. Run the manager, giving a space-delimited input file with no more than
       100 routers, 500 edges, and 1000 packets.
    3. Routers must be numbered sequentially starting from 0.
    4. View output in manager.out and router<i>.out.
    5. Enjoy!

ASSUMPTIONS
==================
    - The input file must be space delimited.
    - Routers must be named sequentially, starting from 0.
    - There may not be more than 100 routers, 500 links, and 1000 packets to send.
    - The maximum number of stepping stones is 30.

MESSAGE FORMAT
==================
    - Messages are formatted in a very lightweight fashion. Depending on the
      state of the program, the messages may be of certain length. Almost all
      messages are sent using structs defined in manager.h. These structs have
      special identifiers which make it easier for the receiving process to
      determine if the send/receive was successful.

FUNCTIONS
==================
  - The functions for each file are listed at the top of router.c and manager.c.
    For manager.h, its only function, 'void get_ip(char* ip)' returns
    the IP address of the machine you are running the program off of. 
