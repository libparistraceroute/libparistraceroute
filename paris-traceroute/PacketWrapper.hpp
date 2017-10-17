//
// Created by root on 10/13/17.
//

#ifndef PARIS_TRACEROUTE_FROM_MAKEFILE_PACKETWRAPPER_HPP
#define PARIS_TRACEROUTE_FROM_MAKEFILE_PACKETWRAPPER_HPP

#include <tins/ip.h>

class PacketWrapper {
public:
    enum class type {
        PROBE, RESPONSE
    };

    PacketWrapper(const Tins::IP & packet, type packet_type);
    const Tins::IP& getPdu() const;
    const type getPduType() const;
private:
    Tins::IP pdu;
    type pdu_type;
};


#endif //PARIS_TRACEROUTE_FROM_MAKEFILE_PACKETWRAPPER_HPP
