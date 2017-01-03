AM_CPPFLAGS = \
	$(GLIB_CFLAGS)						\
	$(GUSB_CFLAGS)						\
	$(PIE_CFLAGS)						\
	-I$(top_srcdir)/libsynapticsmst				\
	-I$(top_srcdir)						\
	-I$(top_builddir)					\
	-DG_USB_API_IS_SUBJECT_TO_CHANGE			\
	-DG_LOG_DOMAIN=\"libsynapticsmst\"				\
	-DTESTDATADIR=\""$(top_srcdir)/data/tests/synapticsmst"\"	\
	-DLOCALEDIR=\""$(localedir)"\"

lib_LTLIBRARIES =						\
	libsynapticsmst.la

libsynapticsmst_includedir = $(includedir)
libsynapticsmst_include_HEADERS =					\
	synapticsmst.h

libsynapticsmstbase_includedir = $(libsynapticsmst_includedir)/libsynapticsmst
libsynapticsmstbase_include_HEADERS =					\
	synapticsmst-device.h

libsynapticsmst_la_SOURCES =						\
	synapticsmst.h						\
	synapticsmst-device.c						\
	synapticsmst-error.c					\
	synapticsmst-device.h                  \
	synapticsmst-common.c                  \
	synapticsmst-common.h

libsynapticsmst_la_LIBADD =						\
	$(GUSB_LIBS)						\
	$(GLIB_LIBS)

libsynapticsmst_la_LDFLAGS =						\
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)	\
	-export-dynamic						\
	-no-undefined						\
	-export-symbols-regex '^synapticsmst_.*'

libsynapticsmst_la_CFLAGS =						\
	$(PIE_CFLAGS)						\
	$(WARN_CFLAGS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = synapticsmst.pc

EXTRA_DIST =							\
	synapticsmst.pc.in

bin_PROGRAMS =							\
	synapticsmst-tool

synapticsmst_tool_SOURCES =						\
	synapticsmst-tool.c

synapticsmst_tool_LDADD =						\
	$(lib_LTLIBRARIES)					\
	$(APPSTREAM_GLIB_LIBS)					\
	$(GLIB_LIBS)						\
	$(GUSB_LIBS)						\
	$(LIBM)

synapticsmst_tool_CFLAGS = -DEGG_TEST $(AM_CFLAGS) $(WARN_CFLAGS)

clean-local:
	rm -f *~

-include $(top_srcdir)/git.mk
