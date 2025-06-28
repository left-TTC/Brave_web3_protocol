

#ifndef BRAVE_WEB3_RPC_H_
#define BRAVE_WEB3_RPC_H_

#include "json.hpp" 
#include <curl/curl.h>

#include "brave_web3_service.h"
#include <mutex>

using json = nlohmann::json;

namespace Solana_Rpc{

    
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
            const json& request_json
        )const;

    private:
        std::string rpc_url_;
    };

    class SolanaRootMap{
    public:
        static SolanaRootMap& instance();

        void set_all(const std::vector<std::string>& values);

        std::vector<std::string> get_all() const;
    private:
        SolanaRootMap() = default;
        ~SolanaRootMap() = default;

        SolanaRootMap(const SolanaRootMap&) = delete;
        SolanaRootMap& operator=(const SolanaRootMap&) = delete;

        std::vector<std::string> data_;
        mutable std::mutex mutex_;
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
    const Solana_web3::Pubkey CENTRAL_REGISTER_STATE = Solana_web3::Pubkey("ERCnYDoLmfPCvmSWB52mubuDgPb7CwgUmfourQJ3sK7d");

    
    size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);

    std::string get_cid_from_json(std::optional<json> json_str);

    std::vector<std::string> get_all_root_domain();

    std::string decodeAndStripPubkeys(const std::string& base64_str);

    std::optional<json> get_account_info(json publickey);

    std::optional<json> get_multiple_account_info(json publickeys);

    json build_request_json(
        const std::string& method,
        const json& parms,
        const std::optional<int> request_id = 1,
        const bool fliters = false
    );
    json build_root_fliters();
    json build_common_request_args(
        const std::vector<json>& pubkey_array,
        const std::optional<commitment>& commitment = std::nullopt,
        const std::optional<std::string>& encoding = std::nullopt,
        const json& extra = {}
    );

}



#endif
