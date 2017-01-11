#
# Foment
#

TEST_OPTIONS = --check-heap
TEST_BUILD = debug

debug ?= yes
compiler ?= gnu


DEFINES = -DFOMENT_UNIX
ifeq ($(debug),yes)
DEFINES += -DFOMENT_DEBUG
endif

ifeq ($(compiler),gnu)
WARNS = -Wall
FDEBUG = -ggdb
FOPTIM = -O2
CCDEBUG = $(WARNS) $(FDEBUG) $(DEFINES)
CCRELEASE = $(WARNS) $(DEFINES) $(FOPTIM)
CCPROFILE = -pg $(CCRELEASE)
else ifeq ($(compiler),intel)
WARNS = -Wall
FDEBUG = -ggdb
FOPTIM = -Ofast -unroll -funroll-loops -inline-level=2
CCDEBUG = $(WARNS) $(FDEBUG) $(DEFINES)
CCRELEASE = $(WARNS) $(DEFINES) $(FOPTIM)
CCPROFILE = -pg $(CCRELEASE)
else ifeq ($(compiler),sun)
endif


ifeq ($(debug),yes)
CFLAGS += $(CCDEBUG)
CXXFLAGS += $(CCDEBUG)
else
CFLAGS += $(CCRELEASE)
CXXFLAGS += $(CCRELEASE)
endif

## note :
##		for intel compilers, don't forget to export these variables
##		before runing make
##			export PATH=/opt/intel/bin:$PATH
##			export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/intel/lib/intel64_lin

ifeq ($(compiler),gnu)
CC = gcc
CXX = g++
CFLAGS +=
CXXFLAGS +=
LDFLAGS +=
else ifeq ($(compiler),intel)
CC = icc
CXX = icpc
CFLAGS += -m64 -std=c11
CXXFLAGS += -m64 -std=c++11
LDFLAGS += -m64
else ifeq ($(compiler),sun)
CC = cc
CXX = cc
CFLAGS += -m64
CXXFLAGS += -m64
LDFLAGS += -m64
endif

help:
	@echo "$(MAKE) [compiler=gnu|intel|sun] [debug=yes|no] [all|]install|clean]"
	@echo "default values :"
	@echo "  compiler: gnu"
	@echo "  debug   : yes"


ifeq ($(debug),yes)
all: debug debug/foment
else
all: debug debug/foment release release/foment
endif

prof: profile profile/foment

install: all
ifeq ($(debug),yes)
	cp debug/foment ~/bin
else
	cp release/foment ~/bin
endif


clean: debug release profile
	-rm debug/*
	-rm release/*
	-rm profile/*

test: all foment-test stress-test chibi-test

test-all: all
	$(MAKE) test TEST_BUILD=debug TEST_OPTIONS=--no-collector
	$(MAKE) test TEST_BUILD=release TEST_OPTIONS=--no-collector
	$(MAKE) test TEST_BUILD=debug TEST_OPTIONS=--mark-sweep
	$(MAKE) test TEST_BUILD=release TEST_OPTIONS=--mark-sweep
	$(MAKE) test TEST_BUILD=debug "TEST_OPTIONS=--no-collector --check-heap"
	$(MAKE) test TEST_BUILD=release "TEST_OPTIONS=--no-collector --check-heap"
	$(MAKE) test TEST_BUILD=debug "TEST_OPTIONS=--mark-sweep --check-heap"
	$(MAKE) test TEST_BUILD=release "TEST_OPTIONS=--mark-sweep --check-heap"

test-generational: all
	$(MAKE) test TEST_BUILD=debug TEST_OPTIONS=--generational
	$(MAKE) test TEST_BUILD=release TEST_OPTIONS=--generational
	$(MAKE) test TEST_BUILD=debug "TEST_OPTIONS=--generational --check-heap"
	$(MAKE) test TEST_BUILD=release "TEST_OPTIONS=--generational --check-heap"

foment-test: all
	cd ../test ; ../unix/$(TEST_BUILD)/foment $(TEST_OPTIONS) runtests.scm r7rs.scm r7rs-eval.scm foment.scm eccentric.scm r5rs_pitfall.scm unicode.scm srfi.scm
	cd ../test ; ../unix/$(TEST_BUILD)/foment $(TEST_OPTIONS) r7rs.scm

stress-test: all
	cd ../test ; ../unix/$(TEST_BUILD)/foment $(TEST_OPTIONS) runtests.scm stress.scm r7rs.scm

chibi-test: all
	cd ../test ; ../unix/$(TEST_BUILD)/foment $(TEST_OPTIONS) r7rs-tests.scm

debug:
	-mkdir debug

release:
	-mkdir release

profile:
	-mkdir profile

debug/base.cpp: debug/txt2cpp ../src/base.scm makefile
	debug/txt2cpp ../src/base.scm debug/base.cpp

debug/foment: debug/foment.o debug/gc.o debug/syncthrd.o debug/compile.o debug/io.o\
		debug/synrules.o debug/synpass.o debug/midpass.o debug/genpass.o\
		debug/pairs.o debug/unicode.o debug/chars.o debug/strings.o debug/vectors.o\
		debug/library.o debug/execute.o debug/numbers.o	debug/mini-gmp.o debug/write.o\
		debug/read.o debug/filesys.o debug/compare.o debug/main.o debug/hashtbl.o\
		debug/bignums.o debug/base.o
	$(CXX) $(LDFLAGS) -o debug/foment $^ -lpthread

release/foment: release/foment.o release/gc.o release/syncthrd.o release/compile.o release/io.o\
		release/synrules.o release/synpass.o release/midpass.o release/genpass.o\
		release/pairs.o release/unicode.o release/chars.o release/strings.o\
		release/vectors.o release/library.o release/execute.o release/numbers.o\
		release/mini-gmp.o release/write.o release/read.o release/filesys.o\
		release/compare.o release/main.o release/hashtbl.o release/bignums.o release/base.o
	$(CXX) $(LDFLAGS) -o release/foment $^ -lpthread

profile/foment: profile/foment.o profile/gc.o profile/syncthrd.o profile/compile.o profile/io.o\
		profile/synrules.o profile/synpass.o profile/midpass.o profile/genpass.o\
		profile/pairs.o profile/unicode.o profile/chars.o profile/strings.o\
		profile/vectors.o profile/library.o profile/execute.o profile/numbers.o\
		profile/mini-gmp.o profile/write.o profile/read.o profile/filesys.o\
		profile/compare.o profile/main.o profile/hashtbl.o profile/bignums.o profile/base.o
	$(CXX) $(LDFLAGS) -pg -o profile/foment $^ -lpthread

debug/foment.o: ../src/foment.cpp ../src/foment.hpp ../src/syncthrd.hpp ../src/unicode.hpp
debug/gc.o: ../src/gc.cpp ../src/foment.hpp ../src/syncthrd.hpp ../src/io.hpp
debug/syncthrd.o: ../src/syncthrd.cpp ../src/foment.hpp ../src/execute.hpp ../src/syncthrd.hpp
debug/compile.o: ../src/compile.cpp ../src/foment.hpp ../src/compile.hpp
debug/synrules.o: ../src/synrules.cpp ../src/foment.hpp ../src/compile.hpp
debug/synpass.o: ../src/synpass.cpp ../src/foment.hpp ../src/compile.hpp
debug/midpass.o: ../src/midpass.cpp ../src/foment.hpp ../src/compile.hpp
debug/genpass.o: ../src/genpass.cpp ../src/foment.hpp ../src/compile.hpp ../src/execute.hpp
debug/pairs.o: ../src/pairs.cpp ../src/foment.hpp
debug/unicode.o: ../src/unicode.cpp ../src/foment.hpp ../src/unicode.hpp ../src/unidata.hpp
debug/chars.o: ../src/chars.cpp ../src/foment.hpp ../src/unicode.hpp
debug/strings.o: ../src/strings.cpp ../src/foment.hpp ../src/unicode.hpp
debug/vectors.o: ../src/vectors.cpp ../src/foment.hpp ../src/unicode.hpp
debug/library.o: ../src/library.cpp ../src/foment.hpp ../src/compile.hpp
debug/execute.o: ../src/execute.cpp ../src/foment.hpp ../src/execute.hpp ../src/syncthrd.hpp
debug/numbers.o: ../src/numbers.cpp ../src/foment.hpp ../src/unicode.hpp ../src/bignums.hpp
debug/bignums.o: ../src/bignums.cpp ../src/foment.hpp ../src/unicode.hpp ../src/bignums.hpp
debug/mini-gmp.o: ../src/mini-gmp.c ../src/mini-gmp.h
debug/io.o: ../src/io.cpp ../src/foment.hpp ../src/syncthrd.hpp ../src/io.hpp\
	../src/unicode.hpp
debug/write.o: ../src/write.cpp ../src/foment.hpp ../src/syncthrd.hpp ../src/io.hpp\
	../src/compile.hpp
debug/read.o: ../src/read.cpp ../src/foment.hpp ../src/syncthrd.hpp ../src/io.hpp\
	../src/unicode.hpp
debug/filesys.o: ../src/filesys.cpp ../src/foment.hpp ../src/unicode.hpp
debug/hashtbl.o: ../src/hashtbl.cpp ../src/foment.hpp ../src/syncthrd.hpp
debug/compare.o: ../src/compare.cpp ../src/foment.hpp ../src/syncthrd.hpp ../src/unicode.hpp
debug/main.o: ../src/main.cpp ../src/foment.hpp
debug/base.o: debug/base.cpp

release/foment.o: ../src/foment.cpp ../src/foment.hpp ../src/syncthrd.hpp ../src/unicode.hpp
release/gc.o: ../src/gc.cpp ../src/foment.hpp ../src/syncthrd.hpp ../src/io.hpp
release/syncthrd.o: ../src/syncthrd.cpp ../src/foment.hpp ../src/execute.hpp ../src/syncthrd.hpp
release/compile.o: ../src/compile.cpp ../src/foment.hpp ../src/compile.hpp
release/synrules.o: ../src/synrules.cpp ../src/foment.hpp ../src/compile.hpp
release/synpass.o: ../src/synpass.cpp ../src/foment.hpp ../src/compile.hpp
release/midpass.o: ../src/midpass.cpp ../src/foment.hpp ../src/compile.hpp
release/genpass.o: ../src/genpass.cpp ../src/foment.hpp ../src/compile.hpp ../src/execute.hpp
release/pairs.o: ../src/pairs.cpp ../src/foment.hpp
release/unicode.o: ../src/unicode.cpp ../src/foment.hpp ../src/unicode.hpp ../src/unidata.hpp
release/chars.o: ../src/chars.cpp ../src/foment.hpp ../src/unicode.hpp
release/strings.o: ../src/strings.cpp ../src/foment.hpp ../src/unicode.hpp
release/vectors.o: ../src/vectors.cpp ../src/foment.hpp ../src/unicode.hpp
release/library.o: ../src/library.cpp ../src/foment.hpp ../src/compile.hpp
release/execute.o: ../src/execute.cpp ../src/foment.hpp ../src/execute.hpp ../src/syncthrd.hpp
release/numbers.o: ../src/numbers.cpp ../src/foment.hpp ../src/unicode.hpp ../src/bignums.hpp
release/bignums.o: ../src/bignums.cpp ../src/foment.hpp ../src/unicode.hpp ../src/bignums.hpp
debug/mini-gmp.o: ../src/mini-gmp.c ../src/mini-gmp.h
release/io.o: ../src/io.cpp ../src/foment.hpp ../src/syncthrd.hpp ../src/io.hpp\
	../src/unicode.hpp
release/write.o: ../src/write.cpp ../src/foment.hpp ../src/syncthrd.hpp ../src/io.hpp\
	../src/compile.hpp
release/read.o: ../src/read.cpp ../src/foment.hpp ../src/syncthrd.hpp ../src/io.hpp\
	../src/unicode.hpp
release/filesys.o: ../src/filesys.cpp ../src/foment.hpp ../src/unicode.hpp
release/hashtbl.o: ../src/hashtbl.cpp ../src/foment.hpp ../src/syncthrd.hpp
release/compare.o: ../src/compare.cpp ../src/foment.hpp ../src/syncthrd.hpp ../src/unicode.hpp
release/main.o: ../src/main.cpp ../src/foment.hpp
release/base.o: debug/base.cpp

debug/%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) -I ../src -o $@ $<

debug/%.o: ../src/%.cpp
	$(CXX) -c $(CXXFLAGS) -I ../src -o $@ $<

debug/%.o: ../src/%.c
	$(CC) -c $(CFLAGS) -I ../src -o $@ $<

debug/%.o: debug/%.cpp
	$(CXX) -c $(CXXFLAGS) -I ../src -o $@ $<

release/%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) -I ../src -o $@ $<

release/%.o: ../src/%.cpp
	$(CXX) -c $(CXXFLAGS) -I ../src -o $@ $<

release/%.o: ../src/%.c
	$(CC) -c $(CFLAGS) -I ../src -o $@ $<

release/%.o: debug/%.cpp
	$(CXX) -c $(CXXFLAGS) -I ../src -o $@ $<

profile/%.o: %.cpp
	$(CXX) -c $(CCPROFILE) -I ../src -o $@ $<

profile/%.o: ../src/%.cpp
	$(CXX) -c $(CCPROFILE) -I ../src -o $@ $<

profile/%.o: ../src/%.c
	$(CC) -c $(CCPROFILE) -I ../src -o $@ $<

profile/%.o: debug/%.cpp
	$(CXX) -c $(CCPROFILE) -I ../src -o $@ $<

debug/txt2cpp: ../src/txt2cpp.cpp
	$(CXX) $(LDFLAGS) ../src/txt2cpp.cpp -o debug/txt2cpp

.PHONY: help all
