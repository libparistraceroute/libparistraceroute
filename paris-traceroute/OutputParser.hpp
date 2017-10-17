//
// Created by root on 10/13/17.
//

#ifndef PARIS_TRACEROUTE_FROM_MAKEFILE_OUTPUTPARSER_HPP
#define PARIS_TRACEROUTE_FROM_MAKEFILE_OUTPUTPARSER_HPP

#define RAPIDJSON_HAS_STDSTRING 1

#include <string>
#include <vector>
#include <regex>
#include <rapidjson/document.h>
#include <tins/rawpdu.h>
#include <tins/udp.h>
#include <tins/icmp.h>
#include <tins/tcp.h>

#include "PacketWrapper.hpp"

class OutputParser {
public:
    rapidjson::Document toJson(const std::string &);

private:


    std::pair<std::vector<PacketWrapper>, std::vector<PacketWrapper>> parseProbesAndResponses(const std::string &);

    Tins::RawPDU parsePayload(std::stringstream &stream);

    Tins::UDP parseUdpPacket(std::stringstream &stream);

    Tins::IP parseIpPacket(std::stringstream &stream);

    Tins::ICMP parseIcmpPacket(std::stringstream &stream);

    std::string extractValue(const std::string &line, const std::regex &lineType);

};


#endif //PARIS_TRACEROUTE_FROM_MAKEFILE_OUTPUTPARSER_HPP
