NULL =

include $(top_srcdir)/Makefile.decl

INCLUDES = 			\
	-g 			\
	$(gio_INCLUDES)		\
	$(GLIB_DEBUG_FLAGS)	\
	-DSRCDIR=\""$(srcdir)"\"

noinst_PROGRAMS = $(TEST_PROGS) $(SAMPLE_PROGS)
noinst_DATA = $(MISC_STUFF)

progs_ldadd     = 					\
	$(top_builddir)/glib/libglib-2.0.la 		\
	$(top_builddir)/gthread/libgthread-2.0.la	\
	$(top_builddir)/gobject/libgobject-2.0.la	\
	$(top_builddir)/gio/libgio-2.0.la

TEST_PROGS +=	 		\
	memory-input-stream 	\
	memory-output-stream 	\
	readwrite		\
	g-file 			\
	g-file-info 		\
	converter-stream	\
	data-input-stream 	\
	data-output-stream 	\
	g-icon			\
	buffered-input-stream	\
	buffered-output-stream	\
	sleepy-stream		\
	filter-streams		\
	volumemonitor		\
	simple-async-result	\
	srvtarget		\
	contexts		\
	gsettings		\
	gschema-compile		\
	async-close-output-stream \
	gdbus-addresses		\
	network-address		\
	gdbus-message		\
	$(NULL)

if OS_UNIX
TEST_PROGS +=			\
	gdbus-connection	\
	gdbus-connection-slow	\
	gdbus-names		\
	gdbus-proxy		\
	gdbus-proxy-well-known-name \
	gdbus-introspection	\
	gdbus-threading		\
	gdbus-export		\
	gdbus-error		\
	gdbus-peer		\
	gdbus-exit-on-close	\
	gdbus-non-socket	\
	gdbus-bz627724		\
	appinfo			\
	contenttype		\
	file			\
	$(NULL)
endif

SAMPLE_PROGS = 				\
	resolver			\
	socket-server			\
	socket-client			\
	echo-server			\
	httpd				\
	send-data			\
	filter-cat			\
	gdbus-example-export		\
	gdbus-example-own-name		\
	gdbus-example-watch-name	\
	gdbus-example-watch-proxy	\
	gdbus-example-server		\
	gdbus-example-subtree		\
	gdbus-example-peer		\
	gdbus-example-proxy-subclass	\
	gdbus-connection-flush-helper	\
	appinfo-test			\
	proxy				\
	$(NULL)


if OS_UNIX
TEST_PROGS += live-g-file desktop-app-info unix-fd unix-streams
SAMPLE_PROGS +=	gdbus-example-unix-fd-client
endif

if OS_WIN32
TEST_PROGS += win32-streams
endif

memory_input_stream_SOURCES	  = memory-input-stream.c
memory_input_stream_LDADD	  = $(progs_ldadd)

memory_output_stream_SOURCES	  = memory-output-stream.c
memory_output_stream_LDADD	  = $(progs_ldadd)

g_file_SOURCES	  = g-file.c
g_file_LDADD	  = $(progs_ldadd)

readwrite_SOURCES	  = readwrite.c
readwrite_LDADD	  = $(progs_ldadd)

g_file_info_SOURCES	= g-file-info.c
g_file_info_LDADD	= $(progs_ldadd)

converter_stream_SOURCES	= converter-stream.c
converter_stream_LDADD		= $(progs_ldadd)

data_input_stream_SOURCES	= data-input-stream.c
data_input_stream_LDADD		= $(progs_ldadd)

data_output_stream_SOURCES	= data-output-stream.c
data_output_stream_LDADD	= $(progs_ldadd)

async_close_output_stream_SOURCES	= async-close-output-stream.c
async_close_output_stream_LDADD		= $(progs_ldadd)

filter_cat_SOURCES	= filter-cat.c
filter_cat_LDADD	= $(progs_ldadd)

g_icon_SOURCES	= g-icon.c
g_icon_LDADD	= $(progs_ldadd)

buffered_input_stream_SOURCES	= buffered-input-stream.c
buffered_input_stream_LDADD	= $(progs_ldadd)

buffered_output_stream_SOURCES	= buffered-output-stream.c
buffered_output_stream_LDADD	= $(progs_ldadd)

live_g_file_SOURCES	  = live-g-file.c
live_g_file_LDADD	  = $(progs_ldadd)

desktop_app_info_SOURCES  = desktop-app-info.c
desktop_app_info_LDADD	  = $(progs_ldadd)

unix_streams_SOURCES	  = unix-streams.c
unix_streams_LDADD	  = $(progs_ldadd) \
	$(top_builddir)/gthread/libgthread-2.0.la

win32_streams_SOURCES	  = win32-streams.c
win32_streams_LDADD	  = $(progs_ldadd) \
	$(top_builddir)/gthread/libgthread-2.0.la

unix_fd_SOURCES	  = unix-fd.c
unix_fd_LDADD	  = $(progs_ldadd)

simple_async_result_SOURCES	= simple-async-result.c
simple_async_result_LDADD	= $(progs_ldadd)

sleepy_stream_SOURCES		= sleepy-stream.c
sleepy_stream_LDADD		= $(progs_ldadd)

filter_streams_SOURCES		= filter-streams.c
filter_streams_LDADD		= $(progs_ldadd)

volumemonitor_SOURCES		= volumemonitor.c
volumemonitor_LDADD		= $(progs_ldadd)

resolver_SOURCES	  = resolver.c
resolver_LDADD		  = $(progs_ldadd) \
	$(top_builddir)/gthread/libgthread-2.0.la

socket_server_SOURCES	  = socket-server.c
socket_server_LDADD	  = $(progs_ldadd) \
	$(top_builddir)/gthread/libgthread-2.0.la

socket_client_SOURCES	  = socket-client.c
socket_client_LDADD	  = $(progs_ldadd) \
	$(top_builddir)/gthread/libgthread-2.0.la

echo_server_SOURCES	  = echo-server.c
echo_server_LDADD	  = $(progs_ldadd) \
	$(top_builddir)/gthread/libgthread-2.0.la

httpd_SOURCES		  = httpd.c
httpd_LDADD		  = $(progs_ldadd) \
	$(top_builddir)/gthread/libgthread-2.0.la

send_data_SOURCES	  = send-data.c
send_data_LDADD		  = $(progs_ldadd) \
	$(top_builddir)/gthread/libgthread-2.0.la

srvtarget_SOURCES	  = srvtarget.c
srvtarget_LDADD		  = $(progs_ldadd)

network_address_SOURCE	  = network-address.c
network_address_LDADD	  = $(progs_ldadd)

contexts_SOURCES	  = contexts.c
contexts_LDADD		  = $(progs_ldadd) \
	$(top_builddir)/gthread/libgthread-2.0.la

gsettings_SOURCES	  = gsettings.c
gsettings_LDADD		  = $(progs_ldadd)

gschema_compile_SOURCES	  = gschema-compile.c
gschema_compile_LDADD	  = $(progs_ldadd)

if HAVE_DBUS1
TEST_PROGS += gdbus-serialization
gdbus_serialization_SOURCES = gdbus-serialization.c gdbus-tests.h gdbus-tests.c
gdbus_serialization_CFLAGS = $(DBUS1_CFLAGS)
gdbus_serialization_LDADD = $(progs_ldadd) $(DBUS1_LIBS)
endif

gdbus_addresses_SOURCES = gdbus-addresses.c
gdbus_addresses_LDADD = $(progs_ldadd)

gdbus_bz627724_SOURCES = gdbus-bz627724.c gdbus-sessionbus.c gdbus-sessionbus.h gdbus-tests.h gdbus-tests.c
gdbus_bz627724_LDADD = $(progs_ldadd)

gdbus_connection_SOURCES = gdbus-connection.c gdbus-sessionbus.c gdbus-sessionbus.h gdbus-tests.h gdbus-tests.c
gdbus_connection_LDADD = $(progs_ldadd)

gdbus_connection_slow_SOURCES = gdbus-connection-slow.c gdbus-sessionbus.c gdbus-sessionbus.h gdbus-tests.h gdbus-tests.c
gdbus_connection_slow_LDADD = $(progs_ldadd)

gdbus_message_SOURCES = gdbus-message.c
gdbus_message_LDADD = $(progs_ldadd)

gdbus_names_SOURCES = gdbus-names.c gdbus-sessionbus.c gdbus-sessionbus.h gdbus-tests.h gdbus-tests.c
gdbus_names_LDADD = $(progs_ldadd)

gdbus_proxy_SOURCES = gdbus-proxy.c gdbus-sessionbus.c gdbus-sessionbus.h gdbus-tests.h gdbus-tests.c
gdbus_proxy_LDADD = $(progs_ldadd)

gdbus_proxy_well_known_name_SOURCES = gdbus-proxy-well-known-name.c gdbus-sessionbus.c gdbus-sessionbus.h gdbus-tests.h gdbus-tests.c
gdbus_proxy_well_known_name_LDADD = $(progs_ldadd)

gdbus_introspection_SOURCES = gdbus-introspection.c gdbus-sessionbus.c gdbus-sessionbus.h gdbus-tests.h gdbus-tests.c
gdbus_introspection_LDADD = $(progs_ldadd)

gdbus_threading_SOURCES = gdbus-threading.c gdbus-sessionbus.c gdbus-sessionbus.h gdbus-tests.h gdbus-tests.c
gdbus_threading_LDADD = $(progs_ldadd)

gdbus_export_SOURCES = gdbus-export.c gdbus-sessionbus.c gdbus-sessionbus.h gdbus-tests.h gdbus-tests.c
gdbus_export_LDADD = $(progs_ldadd)

gdbus_error_SOURCES = gdbus-error.c gdbus-sessionbus.c gdbus-sessionbus.h gdbus-tests.h gdbus-tests.c
gdbus_error_LDADD = $(progs_ldadd)

gdbus_peer_SOURCES = gdbus-peer.c gdbus-tests.h gdbus-tests.c
gdbus_peer_LDADD = $(progs_ldadd)

gdbus_non_socket_SOURCES = gdbus-non-socket.c gdbus-tests.h gdbus-tests.c
gdbus_non_socket_LDADD = $(progs_ldadd)

gdbus_exit_on_close_SOURCES = gdbus-exit-on-close.c gdbus-sessionbus.c gdbus-sessionbus.h gdbus-tests.h gdbus-tests.c
gdbus_exit_on_close_LDADD = $(progs_ldadd)

gdbus_example_watch_name_SOURCES = gdbus-example-watch-name.c
gdbus_example_watch_name_LDADD   = $(progs_ldadd)

gdbus_example_watch_proxy_SOURCES = gdbus-example-watch-proxy.c
gdbus_example_watch_proxy_LDADD   = $(progs_ldadd)

gdbus_example_own_name_SOURCES = gdbus-example-own-name.c
gdbus_example_own_name_LDADD   = $(progs_ldadd)

gdbus_example_server_SOURCES = gdbus-example-server.c
gdbus_example_server_LDADD   = $(progs_ldadd)

gdbus_example_unix_fd_client_SOURCES = gdbus-example-unix-fd-client.c
gdbus_example_unix_fd_client_LDADD   = $(progs_ldadd)

gdbus_example_subtree_SOURCES = gdbus-example-subtree.c
gdbus_example_subtree_LDADD   = $(progs_ldadd)

gdbus_example_peer_SOURCES = gdbus-example-peer.c
gdbus_example_peer_LDADD   = $(progs_ldadd)

gdbus_example_proxy_subclass_SOURCES = gdbus-example-proxy-subclass.c
gdbus_example_proxy_subclass_LDADD   = $(progs_ldadd)

gdbus_example_export_SOURCES = gdbus-example-export.c
gdbus_example_export_LDADD   = $(progs_ldadd)

gdbus_connection_flush_helper_SOURCES = gdbus-connection-flush-helper.c
gdbus_connection_flush_helper_LDADD = $(progs_ldadd)

appinfo_SOURCES = appinfo.c
appinfo_LDADD   = $(progs_ldadd)

appinfo_test_SOURCES = appinfo-test.c
appinfo_test_LDADD   = $(progs_ldadd)

contenttype_SOURCES = contenttype.c
contenttype_LDADD   = $(progs_ldadd)

file_SOURCES = file.c
file_LDADD   = $(progs_ldadd)

schema_tests = \
	schema-tests/array-default-not-in-choices.gschema.xml		\
	schema-tests/bad-choice.gschema.xml				\
	schema-tests/bad-key.gschema.xml				\
	schema-tests/bad-key2.gschema.xml				\
	schema-tests/bad-key3.gschema.xml				\
	schema-tests/bad-key4.gschema.xml				\
	schema-tests/bad-type.gschema.xml				\
	schema-tests/bare-alias.gschema.xml				\
	schema-tests/choice-alias.gschema.xml				\
	schema-tests/choice-bad.gschema.xml				\
	schema-tests/choice-badtype.gschema.xml				\
	schema-tests/choice-invalid-alias.gschema.xml			\
	schema-tests/choice-missing-value.gschema.xml			\
	schema-tests/choice-shadowed-alias.gschema.xml			\
	schema-tests/choice-upside-down.gschema.xml			\
	schema-tests/choice.gschema.xml					\
	schema-tests/choices-wrong-type.gschema.xml			\
	schema-tests/default-in-aliases.gschema.xml			\
	schema-tests/default-not-in-choices.gschema.xml			\
	schema-tests/default-out-of-range.gschema.xml			\
	schema-tests/empty-key.gschema.xml				\
	schema-tests/enum-with-aliases.gschema.xml			\
	schema-tests/enum-with-bad-default.gschema.xml			\
	schema-tests/enum-with-chained-alias.gschema.xml		\
	schema-tests/enum-with-choice.gschema.xml			\
	schema-tests/enum-with-invalid-alias.gschema.xml		\
	schema-tests/enum-with-repeated-alias.gschema.xml		\
	schema-tests/enum-with-repeated-nick.gschema.xml		\
	schema-tests/enum-with-repeated-value.gschema.xml		\
	schema-tests/enum-with-shadow-alias.gschema.xml			\
	schema-tests/enum.gschema.xml					\
	schema-tests/flags-aliased-default.gschema.xml			\
	schema-tests/flags-bad-default.gschema.xml			\
	schema-tests/flags-more-than-one-bit.gschema.xml		\
	schema-tests/flags-with-enum-attr.gschema.xml			\
	schema-tests/flags-with-enum-tag.gschema.xml			\
	schema-tests/extend-and-shadow-indirect.gschema.xml		\
	schema-tests/extend-and-shadow.gschema.xml			\
	schema-tests/extend-missing.gschema.xml				\
	schema-tests/extend-nonlist.gschema.xml				\
	schema-tests/extend-self.gschema.xml				\
	schema-tests/extend-wrong-list-indirect.gschema.xml		\
	schema-tests/extend-wrong-list.gschema.xml			\
	schema-tests/extending.gschema.xml				\
	schema-tests/from-docs.gschema.xml				\
	schema-tests/incomplete-list.gschema.xml			\
	schema-tests/invalid-path.gschema.xml				\
	schema-tests/key-in-list-indirect.gschema.xml			\
	schema-tests/key-in-list.gschema.xml				\
	schema-tests/list-of-missing.gschema.xml			\
	schema-tests/missing-quotes.gschema.xml				\
	schema-tests/no-default.gschema.xml				\
	schema-tests/overflow.gschema.xml				\
	schema-tests/override-missing.gschema.xml			\
	schema-tests/override-range-error.gschema.xml			\
	schema-tests/override-then-key.gschema.xml			\
	schema-tests/override-twice.gschema.xml				\
	schema-tests/override-type-error.gschema.xml			\
	schema-tests/override.gschema.xml				\
	schema-tests/range-badtype.gschema.xml				\
	schema-tests/range-default-high.gschema.xml			\
	schema-tests/range-default-low.gschema.xml			\
	schema-tests/range-high-default.gschema.xml			\
	schema-tests/range-low-default.gschema.xml			\
	schema-tests/range-missing-max.gschema.xml			\
	schema-tests/range-missing-min.gschema.xml			\
	schema-tests/range-parse-error.gschema.xml			\
	schema-tests/range-wrong-type.gschema.xml			\
	schema-tests/range.gschema.xml					\
	schema-tests/wrong-category.gschema.xml

proxy_SOURCES = proxy.c
proxy_LDADD   = $(progs_ldadd) \
	$(top_builddir)/gthread/libgthread-2.0.la

EXTRA_DIST += \
	socket-common.c			\
	org.gtk.test.gschema		\
	org.gtk.test.gschema.xml	\
	testenum.h			\
	enums.xml.template		\
	de.po				\
	$(schema_tests)			\
	appinfo-test.desktop		\
	appinfo-test2.desktop		\
	appinfo-test-gnome.desktop	\
	appinfo-test-notgnome.desktop	\
	gdbus-testserver.py
 
MISC_STUFF = test.mo

test.mo: de.po
	$(MSGFMT) -o test.mo $(srcdir)/de.po; \
	$(MKDIR_P) de/LC_MESSAGES; \
	cp -f test.mo de/LC_MESSAGES

DISTCLEANFILES = \
	applications/mimeinfo.cache	\
	org.gtk.test.enums.xml		\
	de/LC_MESSAGES/test.mo		\
	test.mo				\
	gsettings.store			\
	gschemas.compiled
