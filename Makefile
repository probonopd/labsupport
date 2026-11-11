# labsupport: the static C library and its tests.
#
#     make          build liblabsupport.a
#     make test     run the vectors against the C and the Java implementation
#     make clean

CC ?= cc
CFLAGS ?= -O2
CFLAGS += -std=c99 -Wall -Wextra -Werror -Iinclude
JAVAC ?= javac
JAVA ?= java

SRC := src/cs_core.c src/cs_models.c src/cs_minilab3.c src/cs_keylab.c src/cs_keylab_essential.c src/cs_keylab_mk2.c src/cs_keylab_essential_mk3.c src/cs_keylab_mk3.c
OBJ := $(SRC:src/%.c=obj/%.o)
JSRC := $(wildcard java/io/github/probonopd/labsupport/*.java)

all: liblabsupport.a

obj/%.o: src/%.c src/cs_internal.h include/labsupport.h
	@mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@

liblabsupport.a: $(OBJ)
	$(AR) rcs $@ $^

obj/test_labsupport: tests/test_labsupport.c liblabsupport.a
	$(CC) $(CFLAGS) $< liblabsupport.a -o $@

test: obj/test_labsupport $(JSRC) tests/LabSupportTest.java
	obj/test_labsupport tests/vectors.txt
	@mkdir -p obj/java
	$(JAVAC) -Xlint:all,-path -Werror -encoding UTF-8 -d obj/java $(JSRC) tests/LabSupportTest.java
	$(JAVA) -cp obj/java io.github.probonopd.labsupport.LabSupportTest tests/vectors.txt

clean:
	rm -rf obj liblabsupport.a

.PHONY: all test clean
