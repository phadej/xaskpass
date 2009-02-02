# xaskpass Makefile
#
# Copyright (C) 2005 Juho Salminen
#
# Please see xaskpass.c for more information.
#

PREFIX          ?= /usr/local

# Change this to your X directory
XDIR            ?= /usr/X11R6

LIBS		= -lX11

CPPFLAGS	+= -I${XDIR}/include
LDFLAGS		+= -L${XDIR}/lib ${LIBS}
CFLAGS		+= -Wall -Wextra -ansi -pedantic

ifeq "$(origin CC)" "default"
CC		= gcc
endif

TARGET          = xaskpass

ARCHIVE		= ${TARGET}.tar.gz

all: ${TARGET}

clean:
	echo ${CC} ${CFLAGS}
	rm -f ${TARGET} *~ \#*\#

archive: clean
	rm -f ${ARCHIVE}
	tar -czvf ${ARCHIVE} *

install : all
	cp ${TARGET} ${DESTDIR}${PREFIX}/bin
