# Makefile for Mish

NAME=mish

CSOURCES=bytecode expression functioncallvoid mish compiler executer scheduler syscall code functioncallreturn function scope value thread test
ASOURCES=

LIBS=feta

# ----

-include ../make-base/make-base.mk
-include ../make-base/make-lib.mk

CFLAGS+=-nostdlib
