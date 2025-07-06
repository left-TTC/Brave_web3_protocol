

#include "brave_web3_task.h"

namespace Brave_web3_solana_task{

    void update_root_domains(
        scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory
    ){
        Solana_Rpc::get_all_root_domain(url_loader_factory);
    }

    void handle_web3_domain(
        const GURL& domain,
        base::OnceCallback<void(const GURL&, bool is_web3_domain)> restart_callback,
        content::BrowserContext* browser_context
    ){

        LOG(INFO) << "domain: " << domain;
        LOG(INFO) << "domain host: " << domain.host();

        Solana_Rpc::SolanaRootMap& rootMap = Solana_Rpc::SolanaRootMap::instance();
        std::vector<std::string> all_root_domains =  rootMap.get_all();

        for(const auto& root: all_root_domains){
            LOG(INFO) << "root:" << root;
        }

        if(all_root_domains.size() == 0 && !rootMap.has_loaded){

            auto* storage_partition = browser_context->GetDefaultStoragePartition();
            scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory =
                storage_partition->GetURLLoaderFactoryForBrowserProcess();
            //init
            LOG(INFO) << "need init the root map";
            rootMap.reverse_load_state();
            update_root_domains(url_loader_factory);

            std::move(restart_callback).Run(domain, false);
        }else{
            auto* storage_partition = browser_context->GetDefaultStoragePartition();
            scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory =
                storage_partition->GetURLLoaderFactoryForBrowserProcess();

            auto [index, found, pre_domain] = Solana_web3::fast_find(domain, all_root_domains);

            if(!std::move(found)){
                std::move(restart_callback).Run(domain, false);
                return;
            }

            const std::vector<Solana_web3::Pubkey> roots = rootMap.get_all_pubkey();
            const Solana_web3::Pubkey this_root = roots[std::move(index)];

            LOG(INFO) << "root key: " << this_root.toBase58();
            LOG(INFO) << "pre domains: " << pre_domain;

            Solana_web3::PDA domain_ipfs_key = Solana_web3::Solana_web3_interface::get_account_from_root(std::move(pre_domain), std::move(this_root));

            LOG(INFO) << "domain ipfs key: " << domain_ipfs_key.publickey.toBase58();
            //8ZdaNzfVYUdFpJH8sCkWy59SxN1UXP5JrBJZwxtWQTRB

            const Solana_web3::Pubkey ipfs_pubkey = domain_ipfs_key.publickey;
            ipfs_pubkey.get_pubkey_ipfs(url_loader_factory, std::move(restart_callback));
        }
    }

    void redirct_request(
        network::ResourceRequest* modified_request, 
        const GURL& new_url
    ){
        //check referrer
        net::IsolationInfo::RequestType request_type;
        if(modified_request->referrer.is_empty()){
            request_type = net::IsolationInfo::RequestType::kMainFrame;
        }else{
            request_type = net::IsolationInfo::RequestType::kOther;
        }

        //get a new url
        modified_request->url = new_url;

        //create new site_for_cookie for mainFrame style
        if (request_type == net::IsolationInfo::RequestType::kMainFrame) {
            if (!modified_request->trusted_params){
                modified_request->trusted_params.emplace();
            }

            modified_request->site_for_cookies =
                net::SiteForCookies::FromOrigin(url::Origin::Create(new_url));

            modified_request->trusted_params->isolation_info =
                net::IsolationInfo::Create(
                    request_type,
                    url::Origin::Create(new_url),  // top_frame_origin
                    url::Origin::Create(new_url),  // frame_origin
                    modified_request->site_for_cookies
                );
        }
    }


}

