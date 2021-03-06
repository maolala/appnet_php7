dnl $Id$
dnl config.m4 for extension appnet

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(appnet, for appnet support,
dnl Make sure that the comment is aligned:
dnl [  --with-appnet             Include appnet support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(appnet, whether to enable appnet support,
dnl Make sure that the comment is aligned:
[  --enable-appnet           Enable appnet support])

if test -z "$PHP_DEBUG" ; then 
	AC_ARG_ENABLE(debug, [--enable-debug compile with debugging system], [PHP_DEBUG=$enableval],[PHP_DEBUG=no] ) 
fi 

AC_MSG_CHECKING([if compiling with clang])
AC_COMPILE_IFELSE([
    AC_LANG_PROGRAM([], [[
        #ifndef __clang__
            not clang
        #endif
    ]])],
    [CLANG=yes], [CLANG=no]
)
AC_MSG_RESULT([$CLANG])

if test "$CLANG" = "yes"; then
    CFLAGS="$CFLAGS -std=gnu89"
fi

if test "$PHP_APPNET" != "no"; then
  dnl Write more examples of tests here...
  PHP_ADD_LIBRARY(pthread)
  dnl # --with-appnet -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/appnet.h"  # you most likely want to change this
  dnl if test -r $PHP_APPNET/$SEARCH_FOR; then # path given as parameter
  dnl   APPNET_DIR=$PHP_APPNET
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for appnet files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       APPNET_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$APPNET_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the appnet distribution])
  dnl fi

  dnl # --with-appnet -> add include path
  PHP_ADD_INCLUDE($APPNET_DIR/include)

  dnl # --with-appnet -> check for lib and symbol presence
  dnl LIBNAME=appnet # you may want to change this
  dnl LIBSYMBOL=appnet # you most likely want to change this 
  
  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $APPNET_DIR/$PHP_LIBDIR, APPNET_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_APPNETLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong appnet lib version or lib not found])
  dnl ],[
  dnl   -L$APPNET_DIR/$PHP_LIBDIR -lm
  dnl ])
  dnl
  dnl PHP_SUBST(APPNET_SHARED_LIBADD)

  CFLAGS="-Wall -pthread $CFLAGS"
  LDFLAGS="$LDFLAGS -lpthread"
  
  PHP_SUBST(APPNET_SHARED_LIBADD)
  PHP_ADD_LIBRARY(pthread, 1, APPNET_SHARED_LIBADD)  
  app_source="src/network/aeserver.c \
	src/network/ae_epoll.c \
	src/network/anet.c \
	src/network/worker.c \
	src/network/ae.c \
	src/network/zmalloc.c \
	src/network/sds.c \
	src/network/dict.c \
	src/network/adlist.c \
	src/network/http_request.c \
	src/network/sha1.c \
	src/network/base64.c \
	src/network/websocket.c \
	src/network/share_memory.c \
	src/network/http_response.c"
  PHP_NEW_EXTENSION(appnet, $app_source appnet.c appnet_server.c,  $ext_shared,, -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1)
fi
