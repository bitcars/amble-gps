# The name of the PSERVER/binary
PSERVER = server
PCLIENT = client

SRCDIR = ./
OBJDIR = ./
INCDIR = ./

HOST_TYPE = x86_64

CC:=gcc
LDFINAL:=gcc

ifeq ($(HOST_TYPE), x86_64)
CCFLAG:=
LDFLAGS:=
else
ifeq ($(HOST_TYPE), x86)
CCFLAG:=
LDFLAGS:=
endif
endif

# Optimization and g++ flags
CCFLAG+=-g -Wall
# Linker flags
LDFLAGS+=-lyajl -lpthread
INCS:=-I$(INCDIR)

CCOBJ=protocol.c.o global.c.o

SERVERDEP=$(CCOBJ) tsh.c.o server.c.o
CLIENTDEP=$(CCOBJ) gpspipe.c.o client.c.o

all: $(PSERVER) $(PCLIENT)

$(PSERVER): $(SERVERDEP)
	@echo "Linking the target $@"
	$(LDFINAL) $(SERVERDEP) -o $@ $(LDFLAGS)

$(PCLIENT): $(CLIENTDEP)
	@echo "Linking the target $@"
	$(LDFINAL) $(CLIENTDEP) -o $@ -Wl,-rpath=//usr/local/lib -L. -L/usr/local/lib -lrt -lgps -lm -lyajl -lpthread

%.c.o: %.c
	@echo "Compiling C source: $<"
	$(CC) -c $(CCFLAG) -D_PSERVER_="\"$(PSERVER)\"" $(INCS) $< -o $@

clean:
	@echo "Cleaning $(PSERVER)"
	rm -f *.c.o
	rm -f $(PSERVER) $(PCLIENT)
