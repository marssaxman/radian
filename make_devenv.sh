#!/usr/bin/env bash
#
# Downloads, builds, and "installs" all of the files needed to begin working
# on the Radian project
#

SCRIPT_DIR="$( cd "$( dirname "$0" )" && pwd )"
SETUP_DEST="$SCRIPT_DIR/devenv"
SETUP_TMP="$SETUP_DEST/tmp"
FFI_URL='ftp://sourceware.org/pub/libffi/libffi-3.1.tar.gz'
CPU_COUNT=1

function print_err {
    echo "Exiting due to errors"
    exit 1
}

function download {
    echo "Downloading $1..."
    if [[ -t 1 ]]; then
        curl --progress-bar -O $2
    else
        curl --show-error -s -O $2
    fi
}

set -e
trap print_err ERR

if [[ "$(uname)" == "Darwin" ]]; then
    CPU_COUNT=`sysctl -n hw.ncpu`
elif [[ "$(uname)" == "Linux" ]]; then
    CPU_COUNT=`grep 'physical id' /proc/cpuinfo | sort -u | wc -l`
fi

if [[ -d "$SETUP_DEST" ]]; then
	rm -r "$SETUP_DEST"
fi
mkdir "$SETUP_DEST"
mkdir "$SETUP_TMP"

cd "$SETUP_TMP"
download 'libffi' $FFI_URL
tar xfz "$(basename $FFI_URL)"
cd libffi*
./configure --prefix="$SETUP_DEST"
make -j $CPU_COUNT
make install

# Leave the temp directory around -- it cound be useful!
