//
// Created by Bors Kopin on 12.04.16.
//

#ifndef P3_PROXYCONFIG_H
#define P3_PROXYCONFIG_H

#include <netinet/in.h>

#include <vector>

class ProxyConfig {
public:
    sockaddr_in master_sockaddr;
    std::vector<sockaddr_in> servers_sockaddrs;

    void from_config_file(const char *path_to_config);
};

#endif //P3_PROXYCONFIG_H
