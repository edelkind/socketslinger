xlib_base=.

LXLIB_INC=-I$(xlib_base)/lx_lib/lib
LXLIB_OTH=
LXLIB_CFLAGS=$(LXLIB_INC) $(LXLIB_OTH)
LXLIB_LDFLAGS=-L$(xlib_base)/lx_lib/lib -llx

MINILIB_BASE=$(xlib_base)/minilib
MINILIB_INC=-I$(MINILIB_BASE)
MINILIB_OTH=
MINILIB_CFLAGS=$(MINILIB_INC) $(MINILIB_OTH)
MINILIB_LDFLAGS=-L$(MINILIB_BASE) -lminilib

GETOPTS_BASE=$(xlib_base)/get_opts
GETOPTS_INC=-I$(GETOPTS_BASE)
GETOPTS_OTH=
GETOPTS_CFLAGS=$(GETOPTS_INC) $(GETOPTS_OTH)
GETOPTS_LDFLAGS=-L$(GETOPTS_BASE) -lget_opts

GLIB_INC=$(shell pkg-config --cflags-only-I     glib-2.0)
GLIB_OTH=$(shell pkg-config --cflags-only-other glib-2.0)
GLIB_CFLAGS=$(GLIB_INC) $(GLIB_OTH)
GLIB_LDFLAGS=$(shell pkg-config --libs glib-2.0)


WARNS   = -Wall -Werror
DEBUG   = -g
DEFINES =
CFLAGS  = $(LXLIB_CFLAGS) $(GETOPTS_CFLAGS) $(MINILIB_CFLAGS) $(WARNS) $(DEFINES) $(DEBUG)
LDFLAGS = $(LXLIB_LDFLAGS) $(GETOPTS_LDFLAGS) $(MINILIB_LDFLAGS)
#CFLAGS  = $(GLIB_CFLAGS) $(WARNS) $(DEFINES) $(DEBUG)
#LDFLAGS = $(GLIB_LDFLAGS)


PROGRAMS=sling-input sling-catch sling-watch

.c.o:
	$(CC) -c $< $(CFLAGS)

all: $(PROGRAMS)

sling-input: sling-input.o path.o descriptor.o socket.o
	$(CC) -o $@ $^ $(LDFLAGS)

sling-catch: sling-catch.o path.o descriptor.o socket.o
	$(CC) -o $@ $^ $(LDFLAGS)

sling-watch: sling-watch.o path.o
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *.o $(PROGRAMS)
