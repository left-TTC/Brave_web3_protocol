
#include <iostream>
#include "brave_web3_rpc.h"
#include "brave_web3_task.h"


namespace Brave_Web3_Solana_task{

    void update_root_domains(){

        Solana_Rpc::SolanaRootMap& root_map = Solana_Rpc::SolanaRootMap::instance();

        std::pair<std::vector<std::string>, std::vector<Solana_web3::Pubkey>> roots = Solana_Rpc::get_all_root_domain();

        root_map.set_all(roots.first);   
        root_map.set_all_pubkey(roots.second);  
    }

    std::vector<std::string> get_root_domains(){
        Solana_Rpc::SolanaRootMap& root_map = Solana_Rpc::SolanaRootMap::instance();

        std::vector<std::string> roots = root_map.get_all();

        if(roots.size() == 0){
            update_root_domains();
            roots = root_map.get_all();
        }
        return roots;
    }

    std::vector<Solana_web3::Pubkey> get_root_domains_Pubkey(){
        Solana_Rpc::SolanaRootMap& root_map = Solana_Rpc::SolanaRootMap::instance();
        return root_map.get_all_Pubkey();
    }

    int if_web3_domain(std::string domain_host){
        const std::vector<std::string> roots = get_root_domains();
        
        for (size_t i = 0; i < roots.size(); ++i) {
            const std::string& raw_root = roots[i];
            std::string clean_root = raw_root.size() > 4 ? raw_root.substr(4) : raw_root;

            if (clean_root == domain_host) {
                return static_cast<int>(i);  
            }
        }
        return -1;
    }

    void replace_domain_tocid(const test_class& domain){
        const int if_need_replace = if_web3_domain(domain.host);

        if(if_need_replace == -1){
            return;
        }

        const Solana_web3::Pubkey root_pubkey = get_root_domains_Pubkey()[if_need_replace];
        Solana_web3::PDA domain_solana_account = Solana_web3::Solana_web3_interface::get_account_from_root(domain.domain, root_pubkey);

        std::cout << "ipfs pubkey:" << domain_solana_account.publickey.toBase58() << std::endl;
        const std::string domain_cid = domain_solana_account.publickey.get_pubkey_ipfs();

        if(domain_cid.size() == 0){
            return;
        }

        const std::string new_url = "http://ipfs.io/ipfs/" + domain_cid;
        
        std::cout << "new url:" << new_url << std::endl;
    }
}







