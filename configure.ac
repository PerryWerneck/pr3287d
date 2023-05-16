
dnl Process this file with autoconf to produce a configure script.

dnl The minimum version of autoconf required.
AC_PREREQ([2.69])

dnl Initialise automake with the package name, version and
dnl bug-reporting address.
AC_INIT([pr3287d],[1.1],[perry.werneck@gmail.com],[libudjat],[https://github.com/PerryWerneck/pr3287d])

dnl Place auxilliary scripts here.
AC_CONFIG_AUX_DIR([scripts])

dnl Compute the canonical host-system type
AC_CANONICAL_HOST

dnl Put macro definitions here (though they aren't used).
AC_CONFIG_HEADER([src/include/config.h])

dnl Initialise automake stuff.
AM_INIT_AUTOMAKE

dnl Check for iconv
AM_ICONV

LT_INIT
AC_CONFIG_MACRO_DIRS([m4])

dnl Set gettext version
AM_GNU_GETTEXT_VERSION([0.14])

dnl Checks for programs.
AC_PROG_CC
AC_PROG_SED
AC_PROG_LN_S

AC_LANG([C])

dnl ---------------------------------------------------------------------------
dnl Version info - https://semver.org/
dnl ---------------------------------------------------------------------------

app_vrs_major=$(echo $VERSION | cut -d. -f1)
app_vrs_minor=$(echo $VERSION | cut -d. -f2)
app_vrs_patch=0

AC_ARG_WITH([product-name], [AS_HELP_STRING([--with-product-name], [Set product name])], [ app_cv_product="$withval" ],[ app_cv_product="pr3287d" ])
AC_ARG_WITH([product-id], [AS_HELP_STRING([--with-product-domain], [Set product domain])], [ app_cv_domain="$withval" ],[ app_cv_domain="br.eti.werneck" ])

AC_ARG_WITH([module-debug-path], [AS_HELP_STRING([--with-module-debug-path], [Setup module-debug-path])], [ app_cv_module_debug_path="$withval" ],[ app_cv_module_debug_path="$ac_pwd/.bin/Debug/modules/" ])
AC_SUBST(MODULE_DEBUG_PATH,$app_cv_module_debug_path)

AC_DEFINE_UNQUOTED(PRODUCT_NAME, $app_cv_product,[The product name])
AC_SUBST(PRODUCT_NAME,$app_cv_product)

AC_DEFINE_UNQUOTED(PRODUCT_DOMAIN, $app_cv_domain,[The product domain])
AC_SUBST(PRODUCT_DOMAIN,$app_cv_domain)

app_cv_title="Remote 3287 printer service"
AC_DEFINE_UNQUOTED(PRODUCT_TITLE,"$app_cv_title",[The product title])
AC_SUBST(PRODUCT_TITLE,"$app_cv_title")

AC_DEFINE_UNQUOTED(PACKAGE_MAJOR_VERSION, $app_vrs_major, [The library major version])
AC_DEFINE_UNQUOTED(PACKAGE_MINOR_VERSION, $app_vrs_minor, [The library minor version])
AC_DEFINE_UNQUOTED(PACKAGE_PATCH_LEVEL, $app_vrs_patch, [The patch level])

AC_SUBST(PACKAGE_MAJOR_VERSION,$app_vrs_major)
AC_SUBST(PACKAGE_MINOR_VERSION,$app_vrs_minor)
AC_SUBST(PACKAGE_PATCH_LEVEL,$app_vrs_patch)

AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"libudjat-$app_vrs_major.$app_vrs_minor",[The gettext package name.])
AC_SUBST(GETTEXT_PACKAGE,"pr3287d-$app_vrs_major.$app_vrs_minor")

date="date"
test -z "$SOURCE_DATE_EPOCH" || date="$date -u -d@$SOURCE_DATE_EPOCH"
rpq_build_date=`LANG=C $date`
rpq_timestamp=`$date +%Y%m%d%H%M%S`
rpq_revision=`$date +"%Y%m%d"`
sccs_date=`$date +%Y/%m/%d`
sccs_user=$USER

AC_DEFINE_UNQUOTED(RPQ_BUILD_DATE,"$rpq_build_date",[The RPQ build date])
AC_DEFINE_UNQUOTED(RPQ_TIMESTAMP_VALUE,"$rpq_timestamp",[The RPQ timestamp])
AC_DEFINE_UNQUOTED(SCCS_USER,"$USER",[The build user])
AC_DEFINE_UNQUOTED(SCCS_DATE,"$sccs_date",[The build date])
AC_DEFINE_UNQUOTED(RPQ_REVISION,"$rpq_revision",[The RPQ revision])

AC_SUBST(RPQ_BUILD_DATE,"$rpq_build_date")
AC_SUBST(RPQ_TIMESTAMP_VALUE,"$rpq_timestamp")
AC_SUBST(SCCS_USER,"$USER")
AC_SUBST(SCCS_DATE,"$sccs_date")
AC_SUBST(RPQ_REVISION,"$rpq_revision")

dnl ---------------------------------------------------------------------------
dnl Check for OS specifics
dnl ---------------------------------------------------------------------------

CFLAGS="$CFLAGS -DUSE_ICONV=1 -I$ac_pwd/src/include"
OPTIONAL_MODULES=""

case "$host" in
	*-mingw32|*-pc-msys)
		# https://learn.microsoft.com/en-us/cpp/porting/modifying-winver-and-win32-winnt?view=msvc-170
		CFLAGS="$CFLAGS -pthread -D_WIN32_WINNT=0x0600"
		LIBS="$LIBS -lws2_32 -lwtsapi32 -lcomdlg32 -lwinhttp -lwinspool"
		LDFLAGS="$LDFLAGS -pthread"
		AC_SUBST(EXEEXT,.exe)
		AC_SUBST(OSNAME,windows)
		AC_CONFIG_FILES(src/windows/resources.rc)
		AC_CONFIG_FILES(win/pr3287d.nsi)
		;;

 	*)
		CFLAGS="$CFLAGS -pthread -DDATADIR=\"\$(datarootdir)\""
		LDFLAGS="$LDFLAGS -pthread"
		AC_SUBST(EXEEXT)
		AC_SUBST(OSNAME,linux)

esac

dnl ---------------------------------------------------------------------------
dnl Check for tools
dnl ---------------------------------------------------------------------------

AC_PATH_TOOL([VALGRIND], [valgrind], [no])
AC_PATH_TOOL([AR], [ar], [ar])
AC_PATH_TOOL([DLLTOOL], [dlltool], [true])
AC_PATH_TOOL([STRIP], [strip], [true])
AC_PATH_TOOL([STRIP], [strip], [true])
AC_PATH_TOOL([WINDRES], [windres], [true])
AC_PATH_TOOL([DOXYGEN], [doxygen], [true])

dnl ---------------------------------------------------------------------------
dnl Check for headers
dnl ---------------------------------------------------------------------------

AC_CHECK_FUNC(localtime_r, AC_DEFINE(HAVE_LOCALTIME_R, [], [Do we have localtime_r?]) )
AC_CHECK_FUNC(localtime_s, AC_DEFINE(HAVE_LOCALTIME_S, [], [Do we have localtime_s?]) )
AC_CHECK_FUNC(strcasestr, AC_DEFINE(HAVE_STRCASESTR, [], [Do we have strcasestr?]) )

dnl ---------------------------------------------------------------------------
dnl Check for libintl
dnl ---------------------------------------------------------------------------

AC_PATH_TOOL([XGETTEXT], [xgettext], [true])
AC_PATH_TOOL([MSGCAT], [msgcat], [true])
AC_PATH_TOOL([MSGINIT], [msginit], [true])
AC_PATH_TOOL([MSGMERGE], [msgmerge], [true])
AC_PATH_TOOL([MSGFMT], [msgfmt], [true])

AC_CHECK_HEADER(libintl.h, [AC_DEFINE(HAVE_LIBINTL, 1, [Libintl is available])], [AC_MSG_ERROR([Can't find required file libintl.h])])

dnl ---------------------------------------------------------------------------
dnl Set the ELF symbol visibility support
dnl ---------------------------------------------------------------------------
AC_DEFINE(HAVE_GNUC_VISIBILITY, 1, [supports GCC visibility attributes])

dnl ---------------------------------------------------------------------------
dnl Check for compiler visibility flag
dnl ---------------------------------------------------------------------------
AC_MSG_CHECKING(whether $CC accepts -fvisibility=hidden)
AC_CACHE_VAL(app_cv_fvisibility_ok,
[ac_save_cc="$CC"
CC="$CC -fvisibility=hidden"
AC_RUN_IFELSE([AC_LANG_SOURCE([[int main() { return 0; }]])],[app_cv_fvisibility_ok=yes],[app_cv_fvisibility_ok=no],[app_cv_fvisibility_ok=no])
CC="$ac_save_cc"])
AC_MSG_RESULT($app_cv_fvisibility_ok)
if test $app_cv_fvisibility_ok = yes; then
  CFLAGS="$CFLAGS -fvisibility=hidden"
fi

dnl ---------------------------------------------------------------------------
dnl Libraries
dnl ---------------------------------------------------------------------------
PKG_CHECK_EXISTS

dnl ---------------------------------------------------------------------------
dnl Check for SSL
dnl ---------------------------------------------------------------------------

PKG_CHECK_MODULES( [LIBSSL], [libssl libcrypto], AC_DEFINE(HAVE_LIBSSL,[],[Do we have libssl?]), AC_MSG_ERROR([libssl not present.]) )

CFLAGS="$CFLAGS -DHAVE_LIBSSL=1"
AC_SUBST(LIBSSL_LIBS)
AC_SUBST(LIBSSL_CFLAGS)

dnl ---------------------------------------------------------------------------
dnl Check for PDFGEN
dnl ---------------------------------------------------------------------------

PKG_CHECK_MODULES( [PDFGEN], [libpdfgen], AC_DEFINE(HAVE_PDFGEN,[],[Do we have pdfgen?]), AC_MSG_NOTICE([libpdfgen not present.]) )

AC_SUBST(PDFGEN_LIBS)
AC_SUBST(PDFGEN_CFLAGS)

dnl ---------------------------------------------------------------------------
dnl Output config
dnl ---------------------------------------------------------------------------

AC_SUBST(SRCDIR,$ac_pwd)

AC_CONFIG_FILES(Makefile)

dnl ---------------------------------------------------------------------------
dnl Output the generated config.status script.
dnl ---------------------------------------------------------------------------

AC_OUTPUT

