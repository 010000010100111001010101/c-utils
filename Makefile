PROG = cutils
SRCS = $(wildcard *.c) hashers/spooky.c
OBJS = $(SRCS:.c=.o)
IGNORE = -Wno-implicit-fallthrough -Wno-pointer-to-int-cast
DEBUGFLAGS = -Og -ggdb -DDEBUG -fsanitize=address

CFLAGS = -std=c18 -pedantic -Wall -Wextra -Werror $(IGNORE) $(DEBUGFLAGS)
INCLUDES = -I/usr/local/include -I/usr/include

LDFLAGS = -L/usr/local/lib -L/usr/lib -L.
LDLIBS = -lpthread -lcurl -ljson-c -lwebsockets -lsqlite3

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(PROG) $(OBJS) $(LDFLAGS) $(LDLIBS)

lib$(PROG).so: $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o lib$(PROG).so -fPIC $(SRCS) $(LDFLAGS) -shared $(LDLIBS) 

.PHONY: clean
clean:
	rm -rf $(PROG) $(OBJS) *.o *.so *.core vgcore.*
