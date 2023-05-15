#!/bin/bash
#
# References:
#
#	* https://www.msys2.org/docs/ci/
#
#
echo "Running ${0}"

cd $(dirname $(dirname $(readlink -f ${0})))

#
# Build PDFGEN
#
echo "Building PDFGEN"
rm -fr ./pdfgen
git clone https://github.com/AndreRenaud/PDFGen ./pdfgen || die "clone PDFGEN failure"

sed -i -e "s@^EXE_SUFFIX=.*@EXE_SUFFIX=.exe@g" ./pdfgen/Makefile || die "PDFGEN patch failure"
sed -i -e "s@Windows_NT@DISABLED_OS@g" ./pdfgen/Makefile || die "PDFGEN patch failure"

make -C pdfgen || die "Make failure"

ar rcs ${MINGW_PREFIX}/libpdfgen.a pdfgen/*.o

#
# Build PR3287
#
export PDFGEN_CFLAGS=""
export PDFGEN_LIBS="-lpdfgen"

make all

echo "Build complete"


