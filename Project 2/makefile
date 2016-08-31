CC = gcc
CFLAGS = -Wall -lpthread

ODIR = bin

all: $(ODIR)/gerador $(ODIR)/parque

$(ODIR)/%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)
	
$(ODIR)/gerador: $(ODIR)/gerador.o
	gcc -o $@ $^ $(CFLAGS)

$(ODIR)/parque: $(ODIR)/parque.o
	gcc -o $@ $^ $(CFLAGS)
	


