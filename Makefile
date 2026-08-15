##
##				Gurich
##	**	Ricoh SP110 series driver **
##
##	Copyright (C) 2016, 2017 Gustaf Haglund <kontakt@ghaglund.se>
##
##	This program is free software: you can redistribute it and/or modify
##	it under the terms of the GNU General Public License as published by
##	the Free Software Foundation, either version 3 of the License, or
##	(at your option) any later version.
##
##	This program is distributed in the hope that it will be useful,
##	but WITHOUT ANY WARRANTY; without even the implied warranty of
##	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##	GNU General Public License for more details.
##
##	You should have received a copy of the GNU General Public License
##	along with this program.  If not, see <http://www.gnu.org/licenses/>.
##

DEBUG = -D_DEBUG -D_NO_USB -g3 -ggdb -fvar-tracking
CC = gcc -Wall -pedantic -std=gnu99 -I/usr/local/include
LIBS = $(shell pkg-config --cflags libusb-1.0)
FINAL_LIBS=$(shell pkg-config --libs libusb-1.0) -ljbig -lcups

BIN = bin/gurich
BIN_CBACKEND = bin/gurich_cbackend

CFLAGS = -Isrc -Ilibjbig -Llibjbig $(LIBS)

CBACKEND_DEPEND = src/prntother.o src/basic.o src/prntcommon.o

all: bin $(patsubst src/%.c, src/%.o,$(wildcard src/*.c)) $(patsubst src/cbackend/%.c, src/cbackend/%.o,$(wildcard src/cbackend/*.c))
	$(CC) $(CFLAGS) -o $(BIN) src/*.o  $(FINAL_LIBS)
	$(CC) $(CFLAGS) -o $(BIN_CBACKEND) $(CBACKEND_DEPEND) src/cbackend/*.o $(FINAL_LIBS)

bin:
	mkdir -p bin

# Passed on the command line so it overrides the CFLAGS set above; exporting
# it as an environment variable would silently lose to that assignment, which
# is why this target used to produce a non-debug build.
debug: dev
	$(MAKE) CFLAGS="-Isrc -Ilibjbig -Llibjbig $(LIBS) $(DEBUG)" all

dev:
	rm -f src/*.o
	rm -f src/cbackend/*.o

cups: dev all
	sudo cp $(BIN) /usr/lib/cups/filter/
	sudo cp $(BIN_CBACKEND) /usr/lib/cups/backend/gurich

clean:
	rm -f src/*.o
	rm -f src/cbackend/*.o
	rm -f $(BIN)
	rm -f $(BIN_CBACKEND)

.PHONY: all debug dev cups clean
