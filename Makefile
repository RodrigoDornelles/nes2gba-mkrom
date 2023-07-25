SOURCES = $(wildcard *.c)
OFILES  = $(patsubst %.c, %.o, $(SOURCES))
CFLAGS = -g
LDFLAGS = -lexpat

all: mkrom

mkrom: $(OFILES)
	gcc -o mkrom $(OFILES) $(LDFLAGS)

static: $(OFILES)
	gcc -o mkrom -static $(OFILES) $(LDFLAGS)

clean:
	rm -f mkrom $(OFILES)

depend: .depend
	makedepend -f.depend $(SRCFILES)

.depend: ; touch .depend

TEST_DEPEND := $(shell if [ -f .depend ]; then echo 1; fi)
ifeq ($(TEST_DEPEND), 1)
	include .depend
endif

