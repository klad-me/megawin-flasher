CFLAGS=-Wall
LDFLAGS=-lhidapi-libusb

OBJ=megawin.o


.PHONY:	all

all:	megawin

clean:
	rm -f megawin $(OBJ)

megawin: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
