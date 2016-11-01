# Makefile for Mish

NAME=mish

CSOURCES=bytecode expression mish compiler executer scheduler syscall code functioncallreturn function scope value thread mishtest
ASOURCES=

LIBS=feta

# ----

-include ../make-base/make-base.mk
-include ../make-base/make-lib.mk

CFLAGS+=-nostdlib
