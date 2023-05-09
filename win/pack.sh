#!/bin/bash

PROJECT_NAME="pr3287"

. $(dirname $(readlink -f ${0}))/build.conf

if [ -e ~/.config/user-dirs.dirs ]; then
	. ~/.config/user-dirs.dirs
fi

if [ -z ${XDG_PUBLICSHARE_DIR} ]; then
	XDG_PUBLICSHARE_DIR=~/public_html
fi

PUBLISH=0

until [ -z "$1" ]
do
        if [ ${1:0:2} = '--' ]; then
                tmp=${1:2}
                parameter=${tmp%%=*}
                parameter=$(echo $parameter | tr "[:lower:]" "[:upper:]")
                value=${tmp##*=}

        case "$parameter" in
		NOPUBLISH)
			PUBLISH=0
			;;

		NO-PUBLISH)
			PUBLISH=0
			;;

		PUBLISH)
			PUBLISH=1
			;;

		CLEAR)
			if [ -d ~/${XDG_PUBLICSHARE_DIR}/win/${PROJECT_NAME} ]; then
				rm -fr ~/${XDG_PUBLICSHARE_DIR}/win/${PROJECT_NAME}/*
			fi

			;;
		HELP)
			echo "${0} [opções]"
			echo ""
			echo "Opções:"
			echo ""
			echo "  --no-publish	Não publica binários"
			echo "  --publish	Publica binários em http://pkgserver.desenv.bb.com.br/win/${PROJECT_NAME}/unstable"

			echo ""
			exit 0
			
			;;
		
		esac
	fi

	shift

done

#
# Monta o pacote
#

export TOPDIR=$(mktemp -d)
export BUILD_ROOT=$(mktemp -d)

git clone git@github.com:PerryWerneck/pr3287d.git ${TOPDIR}/SOURCES
if [ "$?" != "0" ]; then
	exit -1
fi

. ~/bin/build-recipe-win32

recipe_build_win32
if [ "$?" != "0" ]; then
	exit -1
fi

RESULTDIR=$(recipe_resultdirs_win32)
if [ -z ${RESULTDIR} ]; then
	echo "No Result dir"
	exit -1
fi

if [ "${PACKAGE_TYPE}" == "nsis" ]; then

	if [ -d ${XDG_PUBLICSHARE_DIR}/win/packages ]; then
		ln -f ${PKGDIR}/*.exe ${XDG_PUBLICSHARE_DIR}/win/packages
	fi

	if [ "${PUBLISH}" == "1" ]; then
		rclone \
			--verbose copy \
			--sftp-host pkgserver.desenv.bb.com.br \
			$(readlink -f ${PKGDIR})/*.exe \
			:sftp:/srv/www/htdocs/win/${PROJECT_NAME}/unstable

		if [ "$?" == "0" ]; then
			for FILE in ${PKGDIR}/*.exe; 
			do 
				echo "http://pkgserver.desenv.bb.com.br/win/${PROJECT_NAME}/unstable/$(basename ${FILE})"
			done
		fi
	fi

fi 

rm -fr "${TOPDIR}"
rm -fr "${BUILD_ROOT}"

