#!/bin/bash
#
# Script de receita pode ser encontrado em:
# https://github.com/PerryWerneck/builder
#
#
PROJECTDIR=$(dirname $(dirname $(readlink -f ${0})))
echo $PROJECTDIR

export TOPDIR=$(mktemp -d)
export BUILD_ROOT=$(mktemp -d)

. ${PROJECTDIR}/win/build.conf
. ~/bin/build-recipe-win32

recipe_build_win32

#cd ${TOPDIR}
#/bin/bash


rm -fr "${TOPDIR}"
rm -fr "${BUILD_ROOT}"

