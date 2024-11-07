#
# The MIT License (MIT)
#
# Copyright (c) Adel Mamin
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

CC = gcc

CFLAGS = -O2 -flto=auto
CFLAGS += -std=c99 -ffunction-sections -fdata-sections -ftrapv

CFLAGS += -Wall -Wextra -Wpedantic -Werror
CFLAGS += -Wchar-subscripts
CFLAGS += -Winit-self
CFLAGS += -Wmain
CFLAGS += -Wmissing-include-dirs
CFLAGS += -Wswitch-default
CFLAGS += -Wswitch-enum
CFLAGS += -Wunused
CFLAGS += -Wunused-parameter
CFLAGS += -Wunused-variable
CFLAGS += -Wuninitialized
CFLAGS += -Wunknown-pragmas
CFLAGS += -Wstrict-overflow=5
CFLAGS += -Wmissing-format-attribute
CFLAGS += -Wfloat-equal
CFLAGS += -Wshadow
CFLAGS += -Wpointer-arith
CFLAGS += -Wundef
CFLAGS += -Wunused-macros
CFLAGS += -Wcast-qual
CFLAGS += -Wwrite-strings
CFLAGS += -Wconversion
CFLAGS += -Wdangling-else
CFLAGS += -Wdate-time
CFLAGS += -Wmissing-declarations
CFLAGS += -Wpacked
CFLAGS += -Wredundant-decls
CFLAGS += -Winline
CFLAGS += -Wdisabled-optimization
CFLAGS += -Winvalid-pch
CFLAGS += -Wmissing-noreturn
CFLAGS += -Wunreachable-code
CFLAGS += -Wformat
CFLAGS += -Wformat=2
CFLAGS += -Wformat-nonliteral
CFLAGS += -Wformat-security
CFLAGS += -Wcast-align
CFLAGS += -Wno-declaration-after-statement

LDFLAGS = -Wl,--gc-sections -flto=auto
TESTFLAGS = $(CFLAGS) $(LDFLAGS) -ggdb -DAMAST_UNIT_TESTS
AR = ar
ARFLAGS = rcs
TEST_EXEC = amast-test

SRC = amast.c amast_test.c
OBJ = amast.o amast_test.o
HEADER = amast.h amast_test.h

LIB = libamast.a

.PHONY: all test clean lib

all: test amast

test: $(OBJ)
	$(CC) $(CFLAGS) $(TESTFLAGS) -o amast-test $(OBJ)
	./amast-test

clean:
	rm -f $(OBJ) amast-test $(LIB)

lib: amast.o
	ar rcs $(LIB) amast.o

%.o: %.c $(HEADER)
	$(CC) $(CFLAGS) -c $< -o $@
