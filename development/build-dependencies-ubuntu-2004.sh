#!/bin/bash

## Qt 6 build dependencies. Ubuntu 20.04 is too old for these package names;
## use Ubuntu 24.04 or another Debian/Ubuntu release with packaged Qt 6.


# preparation
# sudo apt update
# sudo apt upgrade

sudo apt-get update -qq

sudo apt-get -y install git-core build-essential \
                            wget curl make pkg-config \
                            libboost-dev libboost-test-dev libboost-program-options-dev libevent-dev libcurl4-openssl-dev \
                            libpoppler-qt6-dev libqt6sql6-sqlite libhunspell-dev libtidy-dev \
                            qt6-base-dev qt6-base-dev-tools qt6-declarative-dev qt6-tools-dev-tools \
                            qt6-webchannel-dev qt6-webengine-dev

# sudo apt install git
# cd $HOME
# git clone https://github.com/robert7/nixnote2.git
# cd nixnote2

# now build with e.g.
# ./development/build-with-qmake.sh debug noclean /usr/lib
# then binary is in: ./qmake-build-debug/nixnote2
# cd appdir
# ./usr/bin/nixnote2
