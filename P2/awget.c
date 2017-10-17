/*Reader

awget <URL> [-c chainfile]
            or local chaingang.txt
            or error and exit.

Open file.
Pick random ss using rand() and seed.
Connect to ss.
Send URL and chainlist, remove entry that was selected.

Wait for data to arrive.

Saves data to local file, name same as URL.
Process name.
            www.cs.colostate.edu/~cs457/p2.html => p2.html
            no name: index.html from URL

Close connection. */
