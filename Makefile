CC=gcc
CFLAGS= -g -mwindows `pkg-config --cflags gtk+-3.0 glib-2.0 gdk-pixbuf-2.0 pango`
LDFLAGS= `pkg-config --libs gtk+-3.0 glib-2.0 gdk-pixbuf-2.0 pango`
SRCS=rombox.c configure.c cJSON.c
OBJS = ${SRCS:.c=.o}

.SUFFIXES: .c .o

rombox: $(OBJS)
	$(CC) -o RomBox.exe $(OBJS) resources/icon.res $(LDFLAGS)
	
.c.o:; $(CC) $(CFLAGS) -c $<

$OBJS: configure.h rombox.h globals.h

	
clean:
	rm RomBox.exe $(OBJS)
