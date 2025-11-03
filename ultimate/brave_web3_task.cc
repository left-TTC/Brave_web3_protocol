

#include "brave_web3_task.h"

namespace Brave_web3_solana_task{

    DomainCidMap::DomainCidMap() = default;
    DomainCidMap::~DomainCidMap() = default;

    DomainCidMap& DomainCidMap::instance() {
        static base::NoDestructor<DomainCidMap> instance;
        return *instance;
    }

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

        // get the global root domains map
        Solana_Rpc::SolanaRootMap& rootMap = Solana_Rpc::SolanaRootMap::instance();
        std::vector<std::string> all_root_domains =  rootMap.get_all();

        for(const auto& root: all_root_domains){
            LOG(INFO) << "root:" << root;
        }

        // check the map state
        if(all_root_domains.size() == 0 && !rootMap.has_loaded){
            auto* storage_partition = browser_context->GetDefaultStoragePartition();
            scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory =
                storage_partition->GetURLLoaderFactoryForBrowserProcess();

            //init
            LOG(INFO) << "will init the root map !!!";

            rootMap.reverse_load_state();
            update_root_domains(url_loader_factory);

            std::move(restart_callback).Run(domain, false);
        }else{

            LOG(INFO) << "Normal domain name processing !!!";

            auto* storage_partition = browser_context->GetDefaultStoragePartition();
            scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory =
                storage_partition->GetURLLoaderFactoryForBrowserProcess();

            // get the domain content
            const std::string maybe_web3_domain = Solana_web3::extract_target_domain(domain);
            LOG(INFO) << maybe_web3_domain << " is the contained domain";

            // index => the indexof the rootDomain
            // found => if the web3 domain
            // pre_domain => the front part of the domain name
            auto [index, found, pre_domain] = Solana_web3::fast_find(maybe_web3_domain, all_root_domains);

            if(!std::move(found)){
                LOG(INFO) << "this is not a web3 domain";
                std::move(restart_callback).Run(domain, false);
                return;
            }

            DomainCidMap& domain_cid_map = DomainCidMap::instance();
            const absl::optional<std::string> schroding_cid = domain_cid_map.get_cid(maybe_web3_domain);
            if(schroding_cid.has_value()){
                LOG(INFO) << maybe_web3_domain << "directle open";
                std::move(restart_callback).Run(return_url_from_cid(std::move(schroding_cid.value())), true);
                return;
            }

            const std::vector<Solana_web3::Pubkey> roots = rootMap.get_all_pubkey();
            const Solana_web3::Pubkey this_root = roots[std::move(index)];

            LOG(INFO) << "root key: " << this_root.toBase58();
            LOG(INFO) << "pre domains: " << pre_domain;

            Solana_web3::PDA domain_ipfs_key = Solana_web3::Solana_web3_interface::get_account_from_root(std::move(pre_domain), std::move(this_root));

            LOG(INFO) << "domain ipfs key: " << domain_ipfs_key.publickey.toBase58();

            const Solana_web3::Pubkey ipfs_pubkey = domain_ipfs_key.publickey;
            ipfs_pubkey.get_pubkey_ipfs(url_loader_factory, std::move(restart_callback), maybe_web3_domain, domain);
        }
    }


    // https://search.brave.com/search?q=x.web3

    // this is a important function
    // if there are not correspond setting
    // the brower will crashed or the net service will crashed
    void redirect_request(
        network::ResourceRequest* modified_request,
        const GURL& ipfs_url
    ) {
        net::IsolationInfo::RequestType request_type;

        if (modified_request->trusted_params &&
            modified_request->trusted_params->isolation_info.request_type() ==
                net::IsolationInfo::RequestType::kMainFrame) {
            request_type = request_type = net::IsolationInfo::RequestType::kMainFrame;
            LOG(INFO) << "main frame";
        }else if (modified_request->resource_type ==
                static_cast<int>(blink::mojom::ResourceType::kMainFrame)) {
            request_type = request_type = net::IsolationInfo::RequestType::kMainFrame;
            LOG(INFO) << "main frame";
        }else {
            request_type = request_type = net::IsolationInfo::RequestType::kOther;
            LOG(INFO) << "other frame";
        }


        // this parameter means wheather the domain is a strange domain
        // strange => usr directly input the xxx.web3 on navibar
        // and the browser wasn'e ever seen the domain
        // it will trigger a search by search engine
        bool if_stranger_web3 = false;

        // Generally this will be xxx.web3
        const std::string origin_host = modified_request->url.host();
        if (origin_host.find("search.google.com") != std::string::npos ||
            origin_host.find("search.bing.com") != std::string::npos ||
            origin_host.find("search.brave.com") != std::string::npos) {
            // means strange domain and a KmainFrame

            if (request_type != net::IsolationInfo::RequestType::kMainFrame){
                return;
            }
            if_stranger_web3 = true;
        }

        std::string path = modified_request->url.path();
        // get the real IPFS url
        std::string full_ipfs_url = ipfs_url.spec();
        if (!path.empty()) {

            LOG(INFO) << "path" << path;
            LOG(INFO) << "full_ipfs_url" << full_ipfs_url;

            if (full_ipfs_url.back() != '/' && path.front() != '/') {
                full_ipfs_url += '/';
            }
            if (full_ipfs_url.back() == '/' && path.front() == '/'){
                full_ipfs_url.pop_back(); 
            }
            
            if (if_stranger_web3){
                path = ExtractPathFromSearchURL(modified_request->url);
            }

            LOG(INFO) << "this path: " << path;
            full_ipfs_url += path;
        }

        modified_request->url = GURL(full_ipfs_url);

        // when the type is mainframe => the origin must be the xxx.web3
        if (request_type == net::IsolationInfo::RequestType::kMainFrame) {
            if (!modified_request->trusted_params) {
                // if npos => create a trusted_params
                modified_request->trusted_params.emplace();
            }

            auto main_frame_origin = url::Origin::Create(GURL(ipfs_url));

            // main frame site_for_cookie set to null
            modified_request->site_for_cookies =
                net::SiteForCookies::FromOrigin(main_frame_origin);

            // by the policy => the request's isolation.cookies mut be equal to 
            // main frame cookie
            modified_request->trusted_params->isolation_info =
                net::IsolationInfo::Create(
                    request_type,
                    main_frame_origin,  // top_frame_origin
                    main_frame_origin,  // frame_origin
                    net::SiteForCookies::FromOrigin(main_frame_origin)
                );
        }
        
    }

    std::string ExtractPathFromSearchURL(const GURL& url) {
        std::string query = std::string(url.query_piece());
        size_t q_pos = query.find("q=");
        if (q_pos == std::string::npos)
            return "";

        std::string q_value = query.substr(q_pos + 2);

        size_t amp_pos = q_value.find('&');
        if (amp_pos != std::string::npos)
            q_value = q_value.substr(0, amp_pos);

        size_t slash_pos = q_value.find('/');
        if (slash_pos == std::string::npos)
            return "";

        return q_value.substr(slash_pos);
}


    GURL return_url_from_cid(const std::string& cid){
        const std::string new_url = "https://ipfs.io/ipns/" + cid;
        return GURL(new_url);
    }


    

}

