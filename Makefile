CFLAGS=-Wall

PROGRAMS=sling-input sling-catch

.c.o:
	$(CC) -c $< $(CFLAGS)

all: $(PROGRAMS)

sling-input: sling-input.o path.o descriptor.o
	$(CC) -o $@ $^

sling-catch: sling-catch.o path.o descriptor.o
	$(CC) -o $@ $^

clean:
	rm -f *.o $(PROGRAMS)
