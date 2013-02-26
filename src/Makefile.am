## Settings
dbdir = $(localstatedir)/openrfs
dbfile = $(dbdir)/openrfs.db
conflicts=.conflicts
openrfs_conf = $(sysconfdir)/openrfs.conf
COMMON_CPPFLAGS = -DDBFILE='"$(dbfile)"' -DLOCALEDIR='"$(localedir)"' -DCONFLICTS='"$(conflicts)"' 
CONF_CPPFLAGS = -DOPENRFS_CONF='"$(openrfs_conf)"'


bin_PROGRAMS=openrfs
openrfs_SOURCES=openrfs.c queue_sqlite.c sender.c receiver.c executer.c server.c client.c utils.c
openrfs_CPPFLAGS = $(COMMON_CPPFLAGS) $(CONF_CPPFLAGS)

clean-local:
	@rm -f *.gcno
	@rm -f *.gcda



install-exec-local:
	$(MKDIR_P) "$(DESTDIR)$(sbindir)"
	$(LN_S) -f "$(DESTDIR)$(bindir)/openrfs" "$(DESTDIR)$(sbindir)/mount.openrfs"

uninstall-local:
	$(RM) -f "$(DESTDIR)$(sbindir)/mount.openrfs" 


