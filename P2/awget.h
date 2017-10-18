/* Awget Header File

Contains the definition of the data structure for passing the URL and the S list
between awget and SS, and between SS and SS.

Data structure:
- URL
- Number of SS entries
- For each entry  - SS addr
                  - SS port

What about getting the file back?

*/

#ifndef AWGET_H
#define AWGET_H

#include <stdlib.h>

struct ss{
  char[50] ip;
  char[6] port;
};

struct ss_packet{
  uint8_t num_steps;
  char url[500];
  struct ss steps[256];
}

#endif