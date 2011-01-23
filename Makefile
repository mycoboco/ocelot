#
# Makefile for buliding Ocelot
#

SHELL = /bin/sh

.SUFFIXES:
.SUFFIXES: .c .o

AR = ar ruv
RANLIB = ranlib
CP = cp -f
CPR = cp -R
RM = rm -f
RMR = rm -rf
MKDIR = mkdir
RMDIR = rmdir
TAR = tar

SHAREDOPT = -shared
ALL_CFLAGS = -I$I -Wall -W -fPIC $(CFLAGS)

.c.o:
	$(CC) -o $@ -c $(CPPFLAGS) $(ALL_CFLAGS) $<


BLDDIR = build
B = $(BLDDIR)
INCDIR = $B/include
I = $(INCDIR)
LIBDIR = $B/lib
L = $(LIBDIR)
DOCDIR = doc

CBLOBJS = cbl/arena/arena.o cbl/assert/assert.o cbl/except/except.o cbl/memory/memory.o \
	cbl/text/text.o
CBLDOBJS = cbl/arena/arena.o cbl/assert/assert.o cbl/except/except.o cbl/memory/memoryd.o \
	cbl/text/text.o
CDSLOBJS = cdsl/dlist/dlist.o cdsl/hash/hash.o cdsl/list/list.o cdsl/set/set.o cdsl/stack/stack.o \
	cdsl/table/table.o
CELOBJS = cel/conf/conf.o cel/opt/opt.o

CBLHORIG = $(CBLOBJS:.o=.h)
CDSLHORIG = $(CDSLOBJS:.o=.h)
CELHORIG = $(CELOBJS:.o=.h)

CBLHCOPY = $I/cbl/arena.h $I/cbl/assert.h $I/cbl/except.h $I/cbl/memory.h $I/cbl/text.h
CDSLHCOPY = $I/cdsl/dlist.h $I/cdsl/hash.h $I/cdsl/list.h $I/cdsl/set.h $I/cdsl/stack.h \
	$I/cdsl/table.h
CELHCOPY = $I/cel/conf.h $I/cel/opt.h

M = 1
N = 0


what:
	-@echo make all cbl cdsl cel clean

cbl: $(CBLHCOPY) $L/libcbl.a $L/libcbl.so.$M.$N $L/libcbld.a $L/libcbld.so.$M.$N

cdsl: $(CBLHCOPY) $(CDSLHCOPY) $L/libcdsl.a $L/libcdsl.so.$M.$N

cel: $(CBLHCOPY) $(CDSLHCOPY) $(CELHCOPY) $L/libcel.a $L/libcel.so.$M.$N

all: cbl cdsl cel

clean:
	$(RM) $(CBLOBJS) $(CBLDOBJS) $(CDSLOBJS) $(CELOBJS)
	$(RM) $(CBLHCOPY) $(CDSLHCOPY) $(CELHCOPY)
	-$(RMDIR) $I/cbl $I/cdsl $I/cel
	$(RM) $L/libcbl.a $L/libcbld.a $L/libcdsl.a $L/libcel.a
	$(RM) $L/libcbl.so.$M.$N $L/libcbld.so.$M.$N $L/libcdsl.so.$M.$N $L/libcel.so.$M.$N


$L/libcbl.a: $(CBLOBJS)
	$(AR) $@ $(CBLOBJS); $(RANLIB) $@ || true

$L/libcbld.a: $(CBLDOBJS)
	$(AR) $@ $(CBLDOBJS); $(RANLIB) $@ || true

$L/libcdsl.a: $(CDSLOBJS)
	$(AR) $@ $(CDSLOBJS); $(RANLIB) $@ || true

$L/libcel.a: $(CELOBJS)
	$(AR) $@ $(CELOBJS); $(RANLIB) $@ || true

$L/libcbl.so.$M.$N: $(CBLOBJS)
	$(LD) $(SHAREDOPT) -soname=libcbl.so.$M -o $@ $(CBLOBJS)

$L/libcbld.so.$M.$N: $(CBLDOBJS)
	$(LD) $(SHAREDOPT) -soname=libcbld.so.$M -o $@ $(CBLDOBJS)

$L/libcdsl.so.$M.$N: $(CDSLOBJS)
	$(LD) $(SHAREDOPT) -soname=libcdsl.so.$M -o $@ $(CDSLOBJS)

$L/libcel.so.$M.$N: $(CELOBJS)
	$(LD) $(SHAREDOPT) -soname=libcel.so.$M -o $@ $(CELOBJS)

$(CBLHCOPY): $I/cbl $(CBLHORIG)
	$(CP) $(CBLHORIG) $I/cbl/

$(CDSLHCOPY): $I/cdsl $(CDSLHORIG)
	$(CP) $(CDSLHORIG) $I/cdsl/

$(CELHCOPY): $I/cel $(CELHORIG)
	$(CP) $(CELHORIG) $I/cel/

$I/cbl:
	$(MKDIR) $I/cbl

$I/cdsl:
	$(MKDIR) $I/cdsl

$I/cel:
	$(MKDIR) $I/cel


cbl/arena/arena.o:    cbl/arena/arena.c   cbl/arena/arena.h   cbl/assert/assert.h cbl/except/except.h
cbl/assert/assert.o:  cbl/assert/assert.c cbl/assert/assert.h cbl/except/except.h
cbl/except/except.o:  cbl/except/except.c cbl/except/except.h cbl/assert/assert.h
cbl/memory/memory.o:  cbl/memory/memory.c cbl/memory/memory.h cbl/assert/assert.h cbl/except/except.h
cbl/memory/memoryd.o: cbl/memory/memoryd.c cbl/memory/memory.h cbl/assert/assert.h cbl/except/except.h
cbl/text/text.o:      cbl/text/text.c     cbl/text/text.h     cbl/assert/assert.h cbl/memory/memory.h

cdsl/dlist/dlist.o: cdsl/dlist/dlist.c cdsl/dlist/dlist.h cbl/assert/assert.h cbl/memory/memory.h
cdsl/hash/hash.o:   cdsl/hash/hash.c   cdsl/hash/hash.h   cbl/assert/assert.h cbl/memory/memory.h
cdsl/list/list.o:   cdsl/list/list.c   cdsl/list/list.h	  cbl/assert/assert.h cbl/memory/memory.h
cdsl/set/set.o:     cdsl/set/set.c     cdsl/set/set.h     cbl/assert/assert.h cbl/memory/memory.h
cdsl/stack/stack.o: cdsl/stack/stack.c cdsl/stack/stack.h cbl/assert/assert.h cbl/memory/memory.h
cdsl/table/table.o: cdsl/table/table.c cdsl/table/table.h cbl/assert/assert.h cbl/memory/memory.h

cel/conf/conf.o: cel/conf/conf.c cel/conf/conf.h cbl/assert/assert.h cbl/memory/memory.h \
	cdsl/hash/hash.h cdsl/table/table.h
cel/opt/opt.o:   cel/opt/opt.c   cel/opt/opt.h

# end of Makefile
