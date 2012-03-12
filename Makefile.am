SUBDIRS=src

clean-local:
	$(SHELL) $(top_srcdir)/tests/testsuite --clean

check-local: atconfig $(top_srcdir)/tests/testsuite
	$(SHELL) $(top_srcdir)/tests/testsuite

installcheck-local:
	$(SHELL) $(top_srcdir)/tests/testsuite \
		AUTOTEST_PATH=$(exec_prefix)/bin:$(exec_prefix)/libexec

$(top_srcdir)/tests/package.m4: $(top_srcdir)/configure.ac
	{								   \
	  echo '# Signature of the current package.';			   \
	  echo 'm4_define([AT_PACKAGE_NAME],      [@PACKAGE_NAME@])';	   \
	  echo 'm4_define([AT_PACKAGE_TARNAME],   [@PACKAGE_TARNAME@])';   \
	  echo 'm4_define([AT_PACKAGE_VERSION],   [@PACKAGE_VERSION@])';   \
	  echo 'm4_define([AT_PACKAGE_STRING],    [@PACKAGE_STRING@])';	   \
	  echo 'm4_define([AT_PACKAGE_BUGREPORT], [@PACKAGE_BUGREPORT@])'; \
	  echo 'm4_define([AT_PACKAGE_URL],       [@PACKAGE_URL@])';	   \
	} >$(top_srcdir)/tests/package.m4

$(top_srcdir)/tests/testsuite: tests/package.m4 tests/testsuite.at $(TESTFILES)
	$(AUTOM4TE) --language=autotest -I $(top_srcdir)/tests \
		$(top_srcdir)/tests/testsuite.at -o $@.tmp
	mv $@.tmp $@

atconfig: $(top_builddir)/config.status
	cd $(top_builddir) && $(SHELL) ./config.status ./atconfig
