## Settings
dbdir = $(localstatedir)/opendfs
dbfile = $(dbdir)/opendfs.db
conflicts=.conflicts
opendfs_conf = $(sysconfdir)/opendfs.conf
COMMON_CPPFLAGS = -DDBFILE='"$(dbfile)"' -DLOCALEDIR='"$(localedir)"' -DCONFLICTS='"$(conflicts)"' 
CONF_CPPFLAGS = -DOPENDFS_CONF='"$(opendfs_conf)"'


bin_PROGRAMS=opendfs
opendfs_SOURCES=opendfs.c queue_sqlite.c monitor_inotify.c sender.c receiver.c executer.c server.c client.c utils.c
opendfs_CPPFLAGS = $(COMMON_CPPFLAGS) $(CONF_CPPFLAGS)

clean-local:
	@rm -f *.gcno
	@rm -f *.gcda