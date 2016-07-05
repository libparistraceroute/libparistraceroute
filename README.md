# libparistraceroute

libparistraceroute is a library written in C dedicated to active network measurements.
Some example are also provided, such as paris-ping and the new implementation of paris-traceroute.

## Building

On Ubuntu Xenial 16.04 (and probably other Debian derived distributions):

    $ sudo apt-get install build-essential autoconf libtool
    $ ./autogen.sh
    $ ./configure
    $ make
