xlib_base=.

LXLIB_BASE=$(xlib_base)/lx_lib
LXLIB_LIBDIR=$(LXLIB_BASE)/lib
LXLIB_INC=-I$(LXLIB_LIBDIR)
LXLIB_CFLAGS=$(LXLIB_INC)
LXLIB_LDFLAGS=-L$(LXLIB_LIBDIR) -llx
LXLIB_LIB=$(LXLIB_LIBDIR)/liblx.a

MINILIB_BASE=$(xlib_base)/minilib
MINILIB_LIBDIR=$(MINILIB_BASE)
MINILIB_INC=-I$(MINILIB_BASE)
MINILIB_CFLAGS=$(MINILIB_INC)
MINILIB_LDFLAGS=-L$(MINILIB_BASE) -lminilib
MINILIB_LIB=$(MINILIB_LIBDIR)/libminilib.a

GETOPTS_BASE=$(xlib_base)/get_opts
GETOPTS_LIBDIR=$(GETOPTS_BASE)
GETOPTS_INC=-I$(GETOPTS_BASE)
GETOPTS_CFLAGS=$(GETOPTS_INC)
GETOPTS_LDFLAGS=-L$(GETOPTS_BASE) -lget_opts
GETOPTS_LIB=$(GETOPTS_LIBDIR)/libget_opts.a

#WARNS   = -Wall -Werror
WARNS   = -Wall
DEBUG   = -g
DEFINES =
CFLAGS  = $(LXLIB_CFLAGS) $(GETOPTS_CFLAGS) $(MINILIB_CFLAGS) $(WARNS) $(DEBUG) $(DEFINES)
LDFLAGS = $(LXLIB_LDFLAGS) $(GETOPTS_LDFLAGS) $(MINILIB_LDFLAGS)

PREP = $(LXLIB_LIB) $(MINILIB_LIB) $(GETOPTS_LIB)

PROGRAMS=sling-input sling-catch sling-watch

all: $(PROGRAMS)

$(PREP):
	@echo "*** run ./prep.sh to prep dependencies"
	@exit 1

.c.o:
	$(CC) -c $< $(CFLAGS)

sling-input: $(PREP) sling-input.o path.o descriptor.o socket.o
	$(CC) -o $@ $^ $(LDFLAGS)

sling-catch: $(PREP) sling-catch.o path.o descriptor.o socket.o
	$(CC) -o $@ $^ $(LDFLAGS)

sling-watch: $(PREP) sling-watch.o path.o
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *.o $(PROGRAMS)
