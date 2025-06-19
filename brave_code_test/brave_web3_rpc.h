

#ifndef BRAVE_WEB3_RPC_H_
#define BRAVE_WEB3_RPC_H_

#include "json.hpp" 
#include <curl/curl.h>

#include "brave_web3_service.h"

using json = nlohmann::json;

namespace Solana_Rpc{

    size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
    class SolanaRpcClient{
    public:
        SolanaRpcClient(){
            rpc_url_ = Solana_web3::rpc_url;
            curl_global_init(CURL_GLOBAL_DEFAULT);
        }

        ~SolanaRpcClient() {
            // Perform global libcurl cleanup.
            curl_global_cleanup();
        }

        std::optional<json> send_rpc_request(
            const std::string& method,
            const json& params,
            int request_id = 1
        )const;

    private:
        std::string rpc_url_;
    };

    enum class Commitment {
        Processed,
        Confirmed,
        Finalized,
    };

    class commitment{
    public:
        Commitment confirm_level;

        commitment();
        explicit commitment(const Commitment &commitment);

        std::string commitment_to_string() const;
    };

    const Solana_web3::Pubkey WEB3_AUCTION_SERVICE = Solana_web3::Pubkey("9qQuHLMAJEehtk47nKbY1cMAL1bVD7nQxno4SJRDth7");
    const Solana_web3::Pubkey WEB3_NAME_SERVICE = Solana_web3::Pubkey("8YXaA8pzJ4xVPjYY8b5HkxmPWixwpZu7gVcj8EvHxRDC");
    

    Solana_web3::Pubkey get_all_root_domain();

    std::string get_account_info(Solana_web3::Pubkey publickey);

    json build_request_args(
        const std::vector<json>& pubkey_array,
        const std::optional<commitment>& commitment = std::nullopt,
        const std::optional<std::string>& encoding = std::nullopt,
        const json& extra = {}
    );

    json build_rpc_request(
        const std::string& method,
        const json& params,
        int id = 1
    );

    

    

}



#endif
