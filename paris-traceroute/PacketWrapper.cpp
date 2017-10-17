//
// Created by root on 10/13/17.
//

#include "PacketWrapper.hpp"


PacketWrapper::PacketWrapper(const Tins::IP &packet, PacketWrapper::type packet_type) : pdu(packet),
                                                                                        pdu_type(packet_type) {

}

const Tins::IP &PacketWrapper::getPdu()const  {
    return pdu;
}

const PacketWrapper::type PacketWrapper::getPduType()const  {
    return pdu_type ;
}
