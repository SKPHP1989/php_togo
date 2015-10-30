dnl $Id$
dnl config.m4 for extension togo

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(togo, for togo support,
dnl Make sure that the comment is aligned:
dnl [  --with-togo             Include togo support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(togo, whether to enable togo support,
Make sure that the comment is aligned:
[  --enable-togo           Enable togo support])

if test "$PHP_TOGO" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-togo -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/togo.h"  # you most likely want to change this
  dnl if test -r $PHP_TOGO/$SEARCH_FOR; then # path given as parameter
  dnl   TOGO_DIR=$PHP_TOGO
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for togo files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       TOGO_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$TOGO_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the togo distribution])
  dnl fi

  dnl # --with-togo -> add include path
  dnl PHP_ADD_INCLUDE($TOGO_DIR/include)

  dnl # --with-togo -> check for lib and symbol presence
  dnl LIBNAME=togo # you may want to change this
  dnl LIBSYMBOL=togo # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $TOGO_DIR/lib, TOGO_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_TOGOLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong togo lib version or lib not found])
  dnl ],[
  dnl   -L$TOGO_DIR/lib -lm
  dnl ])
  dnl
  dnl PHP_SUBST(TOGO_SHARED_LIBADD)

  PHP_NEW_EXTENSION(togo, togo.c, $ext_shared)
fi
