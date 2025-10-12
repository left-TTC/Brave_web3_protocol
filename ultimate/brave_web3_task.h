#ifndef BRAVE_WEB3_TASK_H
#define BRAVE_WEB3_TASK_H

#include "url/gurl.h"
#include "content/public/browser/browser_context.h"
#include "brave/browser/net/brave_proxying_url_loader_factory.h"

#include "brave_web3_rpc.h"

namespace Brave_web3_solana_task{

    // a cid instance
    class DomainCidMap {
    public:
        static DomainCidMap& instance();
    
        void insert_or_update(const std::string& domain, const std::string& cid) {
            base::AutoLock lock(lock_);
            domain_cid_map_[domain] = cid;
        }
    
        absl::optional<std::string> get_cid(const std::string& domain) const {
            base::AutoLock lock(lock_);
            auto it = domain_cid_map_.find(domain);
            if (it != domain_cid_map_.end()) {
                return it->second;
            }
            return absl::nullopt;
        }
    
        void erase(const std::string& domain) {
            base::AutoLock lock(lock_);
            domain_cid_map_.erase(domain);
        }
    
    private:
        friend class base::NoDestructor<DomainCidMap>;
        DomainCidMap();
        ~DomainCidMap();
    
        mutable base::Lock lock_;
        std::map<std::string, std::string> domain_cid_map_;
    };

    void update_root_domains();

    void handle_web3_domain(
        const GURL& domain,
        base::OnceCallback<void(const GURL&, bool is_web3_domain)> restart_callback,
        content::BrowserContext* browser_context
    );

    void redirect_request(
        network::ResourceRequest* modified_request, 
        const GURL& new_url
    );

    GURL return_url_from_cid(const std::string& cid);

    std::string ExtractPathFromSearchURL(const GURL& url);
    
}

#endif
