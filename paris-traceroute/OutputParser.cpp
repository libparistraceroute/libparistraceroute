//
// Created by root on 10/13/17.
//

#include <sstream>
#include <regex>
#include <algorithm>
#include <unordered_map>

#include "OutputParser.hpp"

using namespace rapidjson;
using namespace Tins;

namespace {
    const std::string lineFormat = "\\s*[a-z_]+\\s+";
    const std::string valueFormat = "([0-9]+)";
    const std::string dataFormat = "([0-9a-f]{2}\\s)+)";
    const std::string ipAddressFormat = "((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?).(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?).(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?).(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?))";
    const std::string anyCharacter = ".*";
    const std::regex fieldLine{lineFormat + valueFormat + anyCharacter};
    const std::regex ipAddressLine{lineFormat + ipAddressFormat + anyCharacter};

    enum class protocols {
        UDP = 17,
        TCP = 6,
        ICMP = 1,
    };

}

std::string OutputParser::extractValue(const std::string &line, const std::regex &lineType) {
    std::smatch baseMatch;
    if (std::regex_match(line, baseMatch, lineType)) {
        //Extract the value from the regex group
        std::ssub_match subMatch = baseMatch[1];
        return subMatch.str();
    } else {
        throw std::exception{};
    }
}

Tins::RawPDU OutputParser::parsePayload(std::stringstream &stream) {
    //TODO Handle payload
    Tins::RawPDU raw("");
    std::string line;
    while (std::getline(stream, line)) {
        if (line.find("size") != std::string::npos) {

        } else if (line.find("data") != std::string::npos) {
            //TODO do something with payload
            break;
        }

    }
    return raw;
}

Tins::UDP OutputParser::parseUdpPacket(std::stringstream &stream) {
    Tins::UDP udp;
    std::string line;
    while (std::getline(stream, line)) {
        if (line.find("src_port") != std::string::npos) {
            udp.sport(std::stoi(extractValue(line, fieldLine)));
        } else if (line.find("dst_port") != std::string::npos) {
            udp.dport(std::stoi(extractValue(line, fieldLine)));
        } else if (line.find("length") != std::string::npos) {

        } else if (line.find("checksum") != std::string::npos) {
            //TODO In libtins, the checksum field is not settable.
            //We should contribute to change it. However, at the moment, use the length field as a checksum.
            udp.length(std::stoi(extractValue(line, fieldLine)));
        } else if (line.find("PAYLOAD") != std::string::npos) {
            Tins::RawPDU raw = parsePayload(stream);
            udp.inner_pdu(raw);
            break;
        }
    }
    return udp;
}


Tins::IP OutputParser::parseIpPacket(std::stringstream &stream) {
    Tins::IP ip;
    std::string line;
    while (std::getline(stream, line)) {
        if (line.find("version") != std::string::npos) {
            auto version = std::stoi(extractValue(line, fieldLine));
            ip.version(version);
        } else if (line.find("ihl") != std::string::npos) {
            auto ihl = std::stoi(extractValue(line, fieldLine));
        } else if (line.find("tos") != std::string::npos) {
            ip.tos(std::stoi(extractValue(line, fieldLine)));
        } else if (line.find("length") != std::string::npos) {

        } else if (line.find("identification") != std::string::npos) {
            ip.id(std::stoi(extractValue(line, fieldLine)));
        } else if (line.find("fragoff") != std::string::npos) {
            ip.fragment_offset(std::stoi(extractValue(line, fieldLine)));
        } else if (line.find("ttl") != std::string::npos) {
            ip.ttl(std::stoi(extractValue(line, fieldLine)));
        } else if (line.find("protocol") != std::string::npos) {
            ip.protocol(std::stoi(extractValue(line, fieldLine)));
        } else if (line.find("checksum") != std::string::npos) {

        } else if (line.find("src_ip") != std::string::npos) {
            ip.src_addr(extractValue(line, ipAddressLine));
        } else if (line.find("dst_ip") != std::string::npos) {
            ip.dst_addr(extractValue(line, ipAddressLine));
        } else if (line.find("LAYER: udp") != std::string::npos) {
            Tins::UDP udp = parseUdpPacket(stream);
            ip.inner_pdu(udp);
            break;
        } else if (line.find("LAYER: icmp") != std::string::npos) {
            Tins::ICMP icmp = parseIcmpPacket(stream);
            ip.inner_pdu(icmp);
            break;
        }
    }
    return ip;
}

Tins::ICMP OutputParser::parseIcmpPacket(std::stringstream &stream) {
    Tins::ICMP icmp;
    std::string line;
    while (std::getline(stream, line)) {
        if (line.find("type") != std::string::npos) {
            icmp.type(Tins::ICMP::Flags(std::stoi(extractValue(line, fieldLine))));
        } else if (line.find("code") != std::string::npos) {
            icmp.code(std::stoi(extractValue(line, fieldLine)));
        } else if (line.find("LAYER: ipv") != std::string::npos) {
            Tins::IP ip = parseIpPacket(stream);
            icmp.inner_pdu(ip);
            break;
        }
    }

    return icmp;
}


Document OutputParser::toJson(const std::string &parisTracerouteOutput) {
    Document result;
    //First parsing step : Read where the probes are.
    auto packets = parseProbesAndResponses(parisTracerouteOutput);
    auto probes = packets.first;
    auto responses = packets.second;
    //Hopefully assume that the first packet from output is a PROBE and not a response
    auto probe1Pdu = probes[0].getPdu();
    //Retrieve some common infos before adding paris traceroute ones.
    //Take the json format of RIPE ATLAS and enrich it with some special infos.
    auto from = probe1Pdu.src_addr().to_string();
    //Destination address
    auto addr = probe1Pdu.dst_addr().to_string();
    //Protocol
    auto mode = probe1Pdu.protocol();
    // TODO Investigate what it corresponds to.
    auto measurementIdentitifer = 0;
    //Name of destination
    auto dstName = probe1Pdu.dst_addr().to_string();
    // TODO Investigate what it corresponds to.
    auto prb_id = 0;


    result.SetObject();

    result.AddMember(Value("addr", result.GetAllocator()),Value(addr, result.GetAllocator()),result.GetAllocator());
    result.AddMember(Value("mode", result.GetAllocator()),Value(mode),result.GetAllocator());
    result.AddMember(Value("from", result.GetAllocator()),Value(from, result.GetAllocator()),result.GetAllocator());
    result.AddMember(Value("dstName", result.GetAllocator()),Value(dstName, result.GetAllocator()),result.GetAllocator());


    std::unordered_map<int, std::vector<Value>> responsesPerHop;
    //First classify the responses by hop
    std::for_each(responses.begin(), responses.end(), [&responsesPerHop, &result, & probes](const PacketWrapper &packetWrapper) {
        const auto &ip = packetWrapper.getPdu().rfind_pdu<IP>();
        const auto &ipProbe = ip.inner_pdu()->rfind_pdu<IP>();
        //Retrieve the corresponding probe
        auto ttl = 0;
        //Generate the json responseFrom the packet wrapper
        Value responseFrom(ip.src_addr().to_string(), result.GetAllocator());
        int src_port;
        int dst_port;
        int checksum;
        if (ipProbe.protocol() == static_cast<int>(protocols::UDP)) {
            const auto &udp = ipProbe.rfind_pdu<UDP>();
            src_port = udp.sport();
            dst_port = udp.dport();
            //We have parsed the checksum as length as the checksum field is not settable.
            checksum = udp.length();
            auto correspondingProbe = std::find_if(probes.begin(), probes.end(), [src_port,dst_port, checksum](const PacketWrapper &pw) {
                const auto &udpProbe = pw.getPdu().rfind_pdu<UDP>();
                return udpProbe.sport() == src_port && udpProbe.dport() == dst_port && udpProbe.length() == checksum;
            });
            if (correspondingProbe == probes.end()) {
                throw std::exception();
            }
            ttl = correspondingProbe->getPdu().ttl();
            //May be we'll need to clean some memory in order to gain some performance

        } else if (ipProbe.protocol() == static_cast<int>(protocols::TCP)) {
            const auto &tcp = ipProbe.rfind_pdu<TCP>();
            src_port = tcp.sport();
            dst_port = tcp.dport();
            //We have parsed the checksum as length as the checksum field is not settable.
            //checksum = tcp.altchecksum();
            auto correspondingProbe = std::find_if(probes.begin(), probes.end(), [src_port, dst_port](const PacketWrapper &pw) {
                const auto &tcpProbe = pw.getPdu().rfind_pdu<TCP>();
                return tcpProbe.sport() == src_port && tcpProbe.dport() == dst_port;
            });
            if (correspondingProbe == probes.end()) {
                throw std::exception();
            }
            ttl = correspondingProbe->getPdu().ttl();
        } else if (ipProbe.protocol() == static_cast<int>(protocols::ICMP)) {
            //TODO handle this case
        }
        Value jsonResponse;
        jsonResponse.SetObject();
        jsonResponse.AddMember(Value("from", result.GetAllocator()), responseFrom, result.GetAllocator());
        jsonResponse.AddMember(Value("src_port", result.GetAllocator()), Value(src_port), result.GetAllocator());
        jsonResponse.AddMember(Value("dst_port", result.GetAllocator()), Value(dst_port), result.GetAllocator());

        responsesPerHop[ttl].push_back(std::move(jsonResponse));


    });

    std::vector<Value> jsonResponses;
    std::transform(responsesPerHop.begin(), responsesPerHop.end(), std::back_inserter(jsonResponses),
                   [&result](std::pair<const int, std::vector<Value>> &hopResponses) {
                       Value jsonHopResponse;
                       jsonHopResponse.SetObject();
                       jsonHopResponse.AddMember(Value("hop", result.GetAllocator()), Value(hopResponses.first),
                                                 result.GetAllocator());
                       Value jsonHopResults;
                       jsonHopResults.SetArray();
                       std::for_each(hopResponses.second.begin(), hopResponses.second.end(), [&result, &jsonHopResults](Value &response) {
                           jsonHopResults.PushBack(response, result.GetAllocator());
                       });
                       jsonHopResponse.AddMember(Value("results", result.GetAllocator()), jsonHopResults,
                                                 result.GetAllocator());
                       return jsonHopResponse;
                   });

    Value allResults;
    allResults.SetArray();
    std::for_each(jsonResponses.begin(),jsonResponses.end(),[&allResults, &result](Value & jsonResponse){
        allResults.PushBack(jsonResponse,result.GetAllocator());
    });
    result.AddMember(Value("results", result.GetAllocator()),allResults,result.GetAllocator());
    return result;
}

std::pair<std::vector<PacketWrapper>, std::vector<PacketWrapper>>
OutputParser::parseProbesAndResponses(const std::string &parisTracerouteOutput) {
    std::vector<PacketWrapper> probes;
    std::vector<PacketWrapper> responses;

    std::stringstream outputStream{parisTracerouteOutput};
    std::string line;

    PacketWrapper::type currentType;
    while (std::getline(outputStream, line)) {

        //Detect if we are in a probe or in a response
        if (line.find("Sending probe packet:") != std::string::npos) {
            currentType = PacketWrapper::type::PROBE;
        } else if (line.find("Got reply:") != std::string::npos) {
            //It's a new Probe, so add the previous one to the vector.
            currentType = PacketWrapper::type::RESPONSE;
        } else if (line.find("LAYER: ip") != std::string::npos) {
            Tins::IP ip = parseIpPacket(outputStream);
            //It's a new Probe, so add the previous one to the vector.
            if (currentType == PacketWrapper::type::PROBE) {
                probes.emplace_back(ip, currentType);
            } else {
                responses.emplace_back(ip, currentType);
            }
        }
    }

    return std::make_pair(probes, responses);
}
