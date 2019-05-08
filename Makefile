DIET=
OBJECTS=session.o queries.o parsing.o format.o objects.o libspopc.o mutex.o
#
# libspoopc can be built with/without some features:
#
# the full libspopc: with semaphres and SSL (openssl)
VERSION=0.12rs
CFLAGS= -Wall -Wextra -pedantic -pipe -fPIC -Os -DUSE_SSL -D_REENTRANT -DUSE_SEM
LDFLAGS=-lssl -lcrypto -lrt
#
# the full libspopc: with mutexes and SSL (openssl)
#VERSION=0.12ps
#CFLAGS= -Wall -Wextra -pedantic -pipe -fPIC -Os -DUSE_SSL -D_REENTRANT -DUSE_PMUTEX
#LDFLAGS=-lssl -lcrypto -lpthread
#
# the backward-compat non-reentrant libspopc with SSL (openssl)
#VERSION=0.12s
#CFLAGS= -Wall -Wextra -pedantic -pipe -fPIC -g -DUSE_SSL
#LDFLAGS=-lssl -lcrypto
#
# the backward-compat non-reentrant non-SSL libspopc
#VERSION=0.12n
#CFLAGS= -Wall -Wextra -pedantic -pipe -fPIC -Os
#LDFLAGS=
#
# info: strdup, snprintf.. are not in ANSI versions of stdio.h/string.h
# this could be nice to conform to -ansi or to -std=iso9899:199409
#
all : libspopc.a libspopc.so examples
	@echo ""
	@echo "read Makefile comments to build different flavours of libspopc"
	@echo "please also read the README file"
#
%.o : %.c libspopc.h
	$(DIET) $(CC) $(CFLAGS) -c $<
#
libspopc.a : $(OBJECTS)
	$(RM) libspopc*.a
	ar r libspopc-$(VERSION).a $(OBJECTS)
	ranlib libspopc-$(VERSION).a
	ln -s libspopc-$(VERSION).a libspopc.a
#
libspopc.so : $(OBJECTS)
	$(RM) libspopc*.so
	$(DIET) $(CC) -o libspopc-$(VERSION).so -shared $(OBJECTS)
	ln -s libspopc-$(VERSION).so libspopc.so
#
%: examples/%.c
	$(DIET) $(CC) -o $@ $(CFLAGS) $< -L. -lspopc $(LDFLAGS)
#
examples: libspopc.a poptest1 poptest2 butcher poptest3
	@echo ""
	@echo "to use examples, export LD_LIBRARY_PATH=.:"'$$LD_LIBRARY_PATH'
#
uninstall :
	$(RM) $(DESTDIR)/usr/lib/libspopc*.a
	$(RM) $(DESTDIR)/usr/lib/libspopc*.so
	$(RM) $(DESTDIR)/usr/include/libspopc*.h
#
install :
	#strip libspopc.so
	install libspopc-$(VERSION).a $(DESTDIR)/usr/lib/
	cd $(DESTDIR)/usr/lib/ && ln -f -s libspopc-$(VERSION).a libspopc.a
	install libspopc-$(VERSION).so $(DESTDIR)/usr/lib/
	cd $(DESTDIR)/usr/lib/ && ln -f -s libspopc-$(VERSION).so libspopc.so
	install libspopc.h $(DESTDIR)/usr/include/
#
clean :
	$(RM) *.o poptest* butcher*
#
distclean : clean
	$(RM) libspopc*.a libspopc*.so
	$(RM) *~ examples/*~ doc/*~

