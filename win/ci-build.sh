#!/bin/bash
#
# References:
#
#	* https://www.msys2.org/docs/ci/
#
#
echo "Running ${0}"

. "$(dirname $(readlink -f ${0}))/bundle.common"

if [ -z ${PACKAGE_NAME} ]; then
	exit -1
fi

#
# Build PDFGEN
#
echo "Building PDFGEN"
rm -fr ./pdfgen
git clone https://github.com/AndreRenaud/PDFGen ./pdfgen || die "clone PDFGEN failure"

sed -i -e "s@^EXE_SUFFIX=.*@EXE_SUFFIX=.exe@g" ./pdfgen/Makefile || die "PDFGEN patch failure"
sed -i -e "s@Windows_NT@DISABLED_OS@g" ./pdfgen/Makefile || die "PDFGEN patch failure"
sed -i -e "s@-fprofile-arcs@@g" ./pdfgen/Makefile || die "PDFGEN patch failure"
sed -i -e "s@-ftest-coverage@@g" ./pdfgen/Makefile || die "PDFGEN patch failure"

make -C pdfgen || die "Make failure"

install --mode=644

ar rcs ${MINGW_PREFIX}/lib/libpdfgen.a pdfgen/*.o
if [ "$?" != "0" ]; then
	exit -1
fi

install --mode=644 *.h ${MINGW_PREFIX}/include
if [ "$?" != "0" ]; then
	exit -1
fi

#
# Build PR3287
#
export PDFGEN_CFLAGS=""
export PDFGEN_LIBS="-lpdfgen"

./autogen.sh
if [ "$?" != "0" ]; then
	exit -1
fi

build_package
install_bin
install_license

#
# Bundle
#
echo zip -9 -r -j "${PACKAGE_NAME}-${PACKAGE_VERSION}.${MSYSTEM_CARCH}.zip" "${buildroot}"

zip -9 -r -j "${PACKAGE_NAME}-${PACKAGE_VERSION}.${MSYSTEM_CARCH}.zip" "${buildroot}"
if [ "$?" != "0" ]; then
	exit -1
fi

ls -l *.zip
echo "Build complete"


