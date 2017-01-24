# Makefile for Mish

NAME=mish

CPPSOURCES=bytecode state mish compile execute schedule thread mishtest
ASOURCES=

LIBS=feta

# ----

-include ../make-base/make-base.mk
-include ../make-base/make-lib.mk

CFLAGS+=-nostdlib -fno-rtti -fno-exceptions -fPIC
