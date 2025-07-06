#ifndef BRAVE_WEB3_TASK_H
#define BRAVE_WEB3_TASK_H

#include "url/gurl.h"
#include "content/public/browser/browser_context.h"
#include "brave/browser/net/brave_proxying_url_loader_factory.h"

#include "brave_web3_rpc.h"

namespace Brave_web3_solana_task{

    void update_root_domains();

    void handle_web3_domain(
        const GURL& domain,
        base::OnceCallback<void(const GURL&, bool is_web3_domain)> restart_callback,
        content::BrowserContext* browser_context
    );


    // void restart_domain_navigation(
    //     const GURL& chekcing_url,
    //     Solana_web3::Pubkey root_key,
    //     base::OnceCallback<void(const GURL&)> restart_callback
    // );

    void redirct_request(
        network::ResourceRequest* modified_request, 
        const GURL& new_url
    );

}

#endif
