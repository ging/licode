#!/bin/bash
LIB_DIR=libdeps
CURRENT_DIR=`pwd`
pause() {
  read -p "$*"
}

install_apt_deps(){
  sudo apt-get install python-software-properties
  sudo apt-get install software-properties-common
  sudo add-apt-repository ppa:chris-lea/node.js
  sudo apt-get update
  sudo apt-get install git npm make gcc g++ libssl-dev cmake libnice10 libnice-dev libglib2.0-dev pkg-config nodejs libboost-regex-dev libboost-thread-dev libboost-system-dev rabbitmq-server mongodb openjdk-6-jre
  sudo npm install -g node-gyp
}

pause "Installing deps via apt-get... [press Enter]"
install_apt_deps

