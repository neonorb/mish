# Makefile for Mish

NAME=mish

CSOURCES=bytecode state mish compile execute schedule scope thread mishtest
ASOURCES=

LIBS=feta

# ----

-include ../make-base/make-base.mk
-include ../make-base/make-lib.mk

CFLAGS+=-nostdlib
