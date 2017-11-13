#Makefile: Arianna Vacca

#Target
default: all
#Compile .c to .o
all:
	gcc -Wall -o manager manager.c
	gcc -Wall -o router router.c
#Compile manager
manager:
	gcc -Wall -o manager manager.c
#Compile router
router:
	gcc -Wall -o router router.c
#Clean
clean:
	rm -f router.o manager.o
	rm -f router manager