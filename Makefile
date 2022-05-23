# Adapted from http://make.mad-scientist.net/papers/multi-architecture-builds
#

# Check if we are not in the target directory (where compiled files will be generated), recognizable by the _ prefix in the name.
# If we are not in the target directory (e.g. we are in the project root dir), then there should also be a target.mk file
# That target.mk file will change to the object dir and run the makefile (this file) there.
ifeq (,$(filter _%,$(notdir $(CURDIR))))
# Not in target dir
include target.mk
else
#----- End Boilerplate
# Now we are in the target directory, where the objects will be compiled.

# PJROOTDIR is set in target.mk. It is the root directory of the source tree. The Makefile is also there.
VPATH = $(PJROOTDIR)/src:$(PJROOTDIR)/include

# Normal makefile rules here
CC        = gcc
CCFLAGS   = -I$(PJROOTDIR)/include -DNLPRESTATOUT
DIRLIBNLPRE = $(CURDIR)
FC        = gfortran
FCFLAGS   = -I/usr/include
MATHFLAGS  = -llapack -lblas -lm
AR         = ar
ARFLAGS    = -rv

%.o: %.c
	$(CC) $(CCFLAGS) -c -o $@ $<

%.o: %.f
	$(FC) $(FCFLAGS) -c -o $@ $<

%.gz: %
	gzip -k $<

all: libnldspred.a

libnldspred.a: bundle.o heap.o kdt.o fn.o fn_exp.o fn_tls.o \
	log.o logtotbl.o mkembed.o point.o sets.o stat.o traverse.o tsfile.o \
	tstoembdef.o dqrdc.o dsvdc.o dtls.o housh.o tr2.o
	$(AR) $(ARFLAGS) $@ $^;

traverse.o: traverse.c traverse.h tsfile.h tstoembdef.h embed.h mkembed.h sets.h point.h fn_exp.h \
	stat.h log.h bundle.h traverse_log.h

traverse.h: tsfile.h tstoembdef.h sets.h fn_exp.h traverse_stat.h

tsfile.o: tsfile.c datalimits.h tsdat.h tsfile.h

tsfile.h: tsdat.h

tstoembdef.o: tstoembdef.c tstoembdef.h

mkembed.o: mkembed.c mkembed.h tsdat.h tsfile.h tstoembdef.h log.h mkembed_log.h

mkembed.h: embed.h

sets.o: sets.c sets.h point.h embed.h log.h bundle.h sets_log.h

sets.h: embed.h bundle.h point.h

sets_log.h: log_meta.h

stat.o: stat.c point.h stat.h log.h stat_log.h

stat.h: point.h

stat_log.h: log_meta.h

point.o: point.c point.h

bundle.o: bundle.c bundle.h log.h bundle_log.h embed.h

bundle_log.h: log_meta.h

fn_exp.o: fn_exp.c kdt.h point.h fn.h fn_exp.h log.h fn_exp_log.h

fn_exp.h: fn.h

fn_exp_log.h: log_meta.h

fn_tls.o: fn_tls.c kdt.h point.h fn.h fn_tls.h log.h fn_tls_log.h

fn_tls.h: fn.h

fn_tls_log.h: log_meta.h

fn.o: fn.c point.h log.h fn.h fn_log.h

fn.h: point.h

fn_log.h: log_meta.h

kdt.o: kdt.c kdt.h heap.h

test_kdt.o: test_kdt.c kdt.h heap.h

heap.o: heap.c heap.h

log.o: log.c log.h log_meta.h

log.h: log_meta.h

logtotbl.o: logtotbl.c logtotbl.h log.h log_meta.h bundle_log.h fn_log.h fn_exp_log.h fn_tls_log.h \
	mkembed_log.h sets_log.h stat_log.h traverse_log.h

#----- Begin Boilerplate
endif
