CC = gcc
CFLAGS = -Wall -g

ODIR = bin

all: $(ODIR)/lsdir $(ODIR)/rmdup 

$(ODIR)/%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)
	
$(ODIR)/lsdir: $(ODIR)/lsdir.o
	gcc -o $@ $^ $(CFLAGS)

$(ODIR)/rmdup: $(ODIR)/rmdup.o
	gcc -o $@ $^ $(CFLAGS)
	


