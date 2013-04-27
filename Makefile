# The name of the PSERVER/binary
PSERVER = server
PCLIENT = client

SRCDIR = ./
OBJDIR = ./
INCDIR = ./

HOST_TYPE = x86_64

CC := g++
LDFINAL := g++

ifeq ($(HOST_TYPE), x86_64)
CCFLAG := -m64
LDFLAGS := -m64
else
ifeq ($(HOST_TYPE), x86)
CCFLAG := -m32
LDFLAGS := -m32
endif
endif

# Optimization and g++ flags
CCFLAG += -g -Wall
# Linker flags
LDFLAGS += -static -lyajl_s -lpthread
INCS := -I$(INCDIR)

CCOBJ = protocol.c.o global.c.o

SERVERDEP = $(CCOBJ) tsh.c.o server.c.o
CLIENTDEP = $(CCOBJ) gpspipe.c.o

all: $(PSERVER) $(PCLIENT)

$(PSERVER): $(SERVERDEP)
	@echo "Linking the target $@"
	$(LDFINAL) $(SERVERDEP) -o $@ $(LIBS) $(LDFLAGS)

$(PCLIENT): $(CLIENTDEP)
	@echo "Linking the target $@"
	$(LDFINAL) $(CLIENTDEP) -o $@ $(LIBS) $(LDFLAGS)

%.c.o: %.c
	@echo "Compiling C source: $<"
	$(CC) -c $(CCFLAG) -D_PSERVER_="\"$(PSERVER)\"" $(INCS) $< -o $@

clean:
	@echo "Cleaning $(PSERVER)"
	rm -f *.c.o
	rm -f $(PSERVER)
