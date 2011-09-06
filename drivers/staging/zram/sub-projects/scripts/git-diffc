#!/bin/bash

##
# git-diffc: Graphical Git diff viewer
#	'c' -> comprehensive :)
# Author: Nitin Gupta <ngupta [at] vflare [dot] org>
# Last Modified (mm-dd-yy): 12/27/09
#

##
# Usage:
#	git-diffc [usual git diff arguments]
#
# This script recreates (sparse) trees containing original
# and modified files. These trees are given to specified
# diff viewer which should be able to handle arguments like:
#	$DIFF_BIN <olddir> <newdir>
# Following diff viewers are known to handle such input:
#	kdiff3, kompare, meld
#

##
# Config:
#	DIFF_BIN: diff viewer (default: kdiff3)
#	CLEANUP: remove temporary files on exit (default: 1)
#	TMPDIR: directory for dumping diff tree (default: /tmp)
#
DIFF_BIN="${DIFF_BIN-`which kdiff3`}"
CLEANUP="${CLEANUP-"1"}"
TMPDIR="${TMPDIR-"/tmp"}"

[ -x "${DIFF_BIN}" ] || {
	echo "Could not find diff viewer [${DIFF_BIN}]. Exiting." 1>&2
	exit 1;
}

function showDiffs {
	echo "source: ${TMP}"
	"${DIFF_BIN}" "${TMP}/old" "${TMP}/new"
}

function cleanup {
	# remove empty list.txt file
	rm ${TMP}/list.txt

	[ "${CLEANUP}" -eq "1" ] || exit 1;
	echo "deleting: ${TMP}"
	rm -rf "${TMP}"
}

function setupTempDirs {
	TMP="${TMPDIR}/.git_diffc"
	TMP="${TMP}/diff.$RANDOM"

	# echo "TMP=${TMP}"
	[ ! -d "${TMP}" ] || {
		echo "Temp directory [${TMP}] already exists! Exiting." 1>&2
		exit 1
	}

	mkdir -p "${TMP}" || {
		echo "Could not create temporary directory! Exiting." 1>&2
		exit 1
	}
}

if [ -n "${GIT_EXTERNAL_DIFF}" ]; then
	[ "${GIT_EXTERNAL_DIFF}" = "${0}" ] || {
		echo "GIT_EXTERNAL_DIFF set to unexpected value" 1>&2;
		exit 1;
	}

	FLIST="${TMP}/list.txt"
	FNAME=`head "${FLIST}" -n 1`
	sed -i '1 d' "${FLIST}"

	echo "[($2) : ($5) : ($FNAME)]"

	D=`dirname $FNAME`
	F=`basename $FNAME`

	OLD_DIR="${TMP}/old/${D}"
	NEW_DIR="${TMP}/new/${D}"
	[ -d "${OLD_DIR}" ] || mkdir -p "${OLD_DIR}"
	[ -d "${NEW_DIR}" ] || mkdir -p "${NEW_DIR}"

	cp "${2}" "${OLD_DIR}/${F}"
	cp "${5}" "${NEW_DIR}/${F}"
else
	setupTempDirs

	git diff "$@" --raw | awk '{ print $6 }' > ${TMP}/list.txt
	GIT_EXTERNAL_DIFF="${0}" TMP="${TMP}" git --no-pager diff "$@"

	showDiffs
	cleanup
fi
