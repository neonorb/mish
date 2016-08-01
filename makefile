# Makefile for Mish

NAME=mish

CSOURCES=bytecode expression functioncallvoid mish syscall code functioncallreturn function scope value
ASOURCES=

LIBS=feta

# ----

-include ../make-base/make-base.mk
-include ../make-base/make-lib.mk
