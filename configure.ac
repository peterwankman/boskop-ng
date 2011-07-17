AC_INIT(boskop-ng, 0.12, bugs@localhost)
AC_CONFIG_SRCDIR(src/main.c)
AM_INIT_AUTOMAKE
AC_CONFIG_HEADERS([config.h])

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_YACC
AC_PROG_LEX

AC_CHECK_FUNC(dlopen,,[AC_CHECK_LIB(dl, [dlopen], [libdl=true;], [AC_ERROR([Misssing library libdl])])])
AC_CHECK_FUNC(socket,,[AC_CHECK_LIB(socket, [socket], [libsocket=true;], [AC_ERROR([Misssing library libsocket])])])
AC_CHECK_FUNC(inet_aton,,[AC_CHECK_LIB(nsl, [inet_aton], [libnsl=true;], [AC_ERROR([Misssing library libnsl])])])

AC_CHECK_HEADERS([stdio.h], [], [AC_ERROR([Missing header stdio.h])])
AC_CHECK_HEADERS([stdarg.h], [], [AC_ERROR([Missing header stdarg.h])])
AC_CHECK_HEADERS([stdlib.h], [], [AC_ERROR([Missing header stdlib.h])])
AC_CHECK_HEADERS([string.h], [], [AC_ERROR([Missing header string.h])])
AC_CHECK_HEADERS([errno.h], [], [AC_ERROR([Missing header errno.h])])
AC_CHECK_HEADERS([time.h], [], [AC_ERROR([Missing header time.h])])
AC_CHECK_HEADERS([ctype.h], [], [AC_ERROR([Missing header ctype.h])])
AC_CHECK_HEADERS([unistd.h], [], [AC_ERROR([Missing header unistd.h])])
AC_CHECK_HEADERS([pwd.h], [], [AC_ERROR([Missing header pwd.h])])
AC_CHECK_HEADERS([dlfcn.h], [], [AC_ERROR([Missing header dlfcn.h])])
AC_CHECK_HEADERS([fcntl.h], [], [AC_ERROR([Missing header fcntl.h])])
AC_CHECK_HEADERS([sys/types.h], [], [AC_ERROR([Missing header sys/types.h])])
AC_CHECK_HEADERS([sys/poll.h], [], [AC_ERROR([Missing header sys/poll.h])])
AC_CHECK_HEADERS([sys/socket.h], [], [AC_ERROR([Missing header sys/socket.h])])
AC_CHECK_HEADERS([arpa/inet.h], [], [AC_ERROR([Missing header arpa/inet.h])])
AC_CHECK_HEADERS([netinet/in.h], [], [AC_ERROR([Missing header netinet/in.h])])
AC_CHECK_HEADERS([netdb.h], [], [AC_ERROR([Missing header netdb.h])])
AC_CHECK_HEADERS([regex.h], [], [AC_ERROR([Missing header regex.h])])

AC_ARG_ENABLE([xml], [  --enable-xml  Enable XML], [
   AC_CHECK_LIB(xml2, [xmlReadMemory], [], [AC_ERROR([Misssing library libmxl2])])
   AC_CHECK_HEADERS([libxml/tree.h], [], [AC_ERROR([Missing header libxml/tree.h])])
   AC_CHECK_HEADERS([libxml/parser.h], [], [AC_ERROR([Missing header libxml/parser.h])])
   enxml=true; 
])
AC_ARG_ENABLE([ipv6], [  --enable-ipv6  Enable IPV6], [enipv6=true;])

AC_ARG_ENABLE([tls], [  --enable-tls  Enable TLS], [
  AC_CHECK_LIB(gnutls, [gnutls_global_init], [], [AC_ERROR([Misssing library libgnutls])])
  AC_CHECK_HEADERS([gnutls/gnutls.h], [], [AC_ERROR([Missing header gnutls/gnutls.h])])
  entls=true;
])

kern="$(uname -s)"
echo "running on ${kern}"
case "${kern}" in
SunOS)
  rdynamic=false
  ;;
*)
  rdynamic=true
  ;;
esac

AM_CONDITIONAL([WITH_LIBDL], [test x$libdl = xtrue])
AM_CONDITIONAL([WITH_LIBSOCKET], [test x$libsocket = xtrue])
AM_CONDITIONAL([WITH_LIBNSL], [test x$libnsl = xtrue])
AM_CONDITIONAL([WITH_RDYNAMIC], [test x$rdynamic = xtrue])

AM_CONDITIONAL([ENABLE_XML], [test x$enxml = xtrue])
AM_CONDITIONAL([ENABLE_IPV6], [test x$enipv6 = xtrue])
AM_CONDITIONAL([ENABLE_TLS], [test x$entls = xtrue])

AC_PREFIX_DEFAULT(`pwd`)
AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT

