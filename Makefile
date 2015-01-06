CC = gcc -g
INC = 
LDS = 

SHLD = ${CC} ${CFLAGS}
LDSHFLAGS = -pthread
LDFLAGS = 

all: smtp stress_prog

smtp:	smtp.o
	$(CC) -o smtp smtp.o $(INCS) $(LDS)

smtp.o:	smtp.c
	$(CC) -c -o smtp.o smtp.c $(INCS) 


stress_prog:	stress_prog.o
	$(CC) -o stress_prog stress_prog.o -pthread $(INCS) $(LDS)

stress_prog.o:	stress_prog.c
	$(CC) -c -o stress_prog.o stress_prog.c 



clean:
	rm -f *.o
	rm -f smtp 
	rm -f stress_prog
