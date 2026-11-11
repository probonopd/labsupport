# ctlsurface: the static C library and its tests.
#
#     make          build libctlsurface.a
#     make test     run the vectors against the C and the Java implementation
#     make clean

CC ?= cc
CFLAGS ?= -O2
CFLAGS += -std=c99 -Wall -Wextra -Werror -Iinclude
JAVAC ?= javac
JAVA ?= java

SRC := src/cs_core.c src/cs_models.c src/cs_minilab3.c src/cs_keylab.c src/cs_keylab_essential.c src/cs_keylab_mk2.c src/cs_keylab_essential_mk3.c src/cs_keylab_mk3.c
OBJ := $(SRC:src/%.c=obj/%.o)
JSRC := $(wildcard java/io/github/probonopd/ctlsurface/*.java)

all: libctlsurface.a

obj/%.o: src/%.c src/cs_internal.h include/ctlsurface.h
	@mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@

libctlsurface.a: $(OBJ)
	$(AR) rcs $@ $^

obj/test_ctlsurface: tests/test_ctlsurface.c libctlsurface.a
	$(CC) $(CFLAGS) $< libctlsurface.a -o $@

test: obj/test_ctlsurface $(JSRC) tests/CtlSurfaceTest.java
	obj/test_ctlsurface tests/vectors.txt
	@mkdir -p obj/java
	$(JAVAC) -Xlint:all,-path -Werror -encoding UTF-8 -d obj/java $(JSRC) tests/CtlSurfaceTest.java
	$(JAVA) -cp obj/java io.github.probonopd.ctlsurface.CtlSurfaceTest tests/vectors.txt

clean:
	rm -rf obj libctlsurface.a

.PHONY: all test clean
