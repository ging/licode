FROM ubuntu:14.04

MAINTAINER Lynckia

WORKDIR /opt

# Download latest version of the code and install dependencies
RUN  apt-get update && apt-get install -y git wget curl

COPY .nvmrc package.json /opt/licode/

COPY scripts/installUbuntuDeps.sh scripts/checkNvm.sh /opt/licode/scripts/

WORKDIR /opt/licode/scripts

RUN ./installUbuntuDeps.sh --cleanup --fast

WORKDIR /opt

COPY . /opt/licode

RUN mkdir /opt/licode/.git

# Clone and install licode
WORKDIR /opt/licode/scripts

RUN ./installErizo.sh -feacs && \
    ./../nuve/installNuve.sh && \
    ./installBasicExample.sh

WORKDIR /opt

ENTRYPOINT ["./licode/extras/docker/initDockerLicode.sh"]
