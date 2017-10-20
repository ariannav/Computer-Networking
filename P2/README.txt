Author: Arianna Vacca
Assignment: 457 PA2

******
README
******

COMPILE
==================
  SS AND AWGET:
      make all
  AWGET:
      make awget
  SS:
      make ss
  CLEAN:
      make clean

RUN
==================
    - awget:  $./awget <URL> [-c chainfile]
    - ss:     $./ss [-p port]

    The url for awget must be given before the chainfile.
    The default port for the stepping stone is 7014, if no port is given.

HOW TO USE
==================
    1. Compile both files using 'make all'.
    2. Run the desired number of stepping stones on the desired ports.
    3. Provide a chaingang.txt file with the correct IP addresses and port
       numbers corresponding to the running stepping stones.
    4. Run awget with the desired file's URL and the created chaingang.txt from
       step 3.
    5. Enjoy!

ASSUMPTIONS
==================
    - All URLs given must begin with "https://", "http://", or "www."
    - A working URL must be given.
    - A stepping stone run without a port number ($./ss) will run from port 7014.
    - If no chainfile is given when running awget ($./awget <URL>), a properly
      formatted chaingang.txt must be within the working directory.
    - The maximum number of stepping stones is 30. 