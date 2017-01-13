#
# Unix only
# Makefile
#

fromdir ?= unix
debug ?= yes
compiler ?= gnu

help:
	$(MAKE) -C $(fromdir) debug=$(debug) compiler=$(compiler) help

all:
	$(MAKE) -C $(fromdir) debug=$(debug) compiler=$(compiler) all

prof:
	$(MAKE) -C $(fromdir) debug=$(debug) compiler=$(compiler) prof

install:
	$(MAKE) -C $(fromdir) debug=$(debug) compiler=$(compiler) install

clean:
	$(MAKE) -C $(fromdir) debug=$(debug) compiler=$(compiler) clean

test:
	$(MAKE) -C $(fromdir) debug=$(debug) compiler=$(compiler) test

test-all:
	$(MAKE) -C $(fromdir) debug=$(debug) compiler=$(compiler) test-all


.PHONY: help all prof install clean test test-all
