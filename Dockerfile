FROM ubuntu:18.04 as buildstage

ENV DEBIAN_FRONTEND noninteractive
RUN apt-get update
RUN apt-get install -y bash vim
RUN apt-get install -y build-essential git-core make libtool automake autoconf cmake pkgconf
RUN apt-get install -y libsystemd-dev libusb-1.0-0-dev cmake
RUN apt-get install -y libev-dev libmosquitto-dev

COPY . /src
WORKDIR /src

RUN sh bootstrap.sh
RUN ./configure
RUN make
RUN make install
