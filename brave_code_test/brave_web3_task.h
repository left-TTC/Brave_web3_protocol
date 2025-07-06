

#ifndef BRAVE_WEB3_TASK_H
#define BRAVE_WEB3_TASK_H

#include "json.hpp" 
#include "brave_web3_service.h"

using json = nlohmann::json;

namespace Brave_Web3_Solana_task{

    class test_class{
    public:
        std::string domain;
        std::string host;

        test_class(std::string domain, std::string host){
            this->domain = domain;
            this->host = host;
        };
    };

    void update_root_domains();

    std::vector<std::string> get_root_domains();

    std::vector<Solana_web3::Pubkey> get_root_domains_Pubkey();

    int if_web3_domain(std::string domain_host);

    void replace_domain_tocid(const test_class& domain);
}


#endif

