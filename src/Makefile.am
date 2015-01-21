bin_PROGRAMS=openrfs
openrfs_SOURCES=openrfs.c utils.c privs.c openrfs_fuse.c events.c
openrfs_CFLAGS=@FUSE_CFLAGS@
openrfs_LDADD=@FUSE_LIBS@

clean-local:
	@rm -f *.gcno
	@rm -f *.gcda

install-exec-local:
	$(MKDIR_P) "$(DESTDIR)$(sbindir)"
	$(LN_S) -f "$(DESTDIR)$(bindir)/openrfs" "$(DESTDIR)$(sbindir)/mount.openrfs"

uninstall-local:
	$(RM) -f "$(DESTDIR)$(sbindir)/mount.openrfs" 


