#ifndef BRAVE_WEB3_RPC_H_
#define BRAVE_WEB3_RPC_H_


#include "third_party/abseil-cpp/absl/types/optional.h"

#include "base/json/json_reader.h"     
#include "base/json/json_writer.h"    
#include "base/logging.h"             
#include "base/values.h"
#include "base/functional/callback.h"

#include "base/json/json_reader.h" 

#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/memory/scoped_refptr.h"
#include "base/values.h"
#include "net/base/net_errors.h" // For net::Error and net::OK
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "url/gurl.h"

#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"

#include "net/http/http_status_code.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "base/functional/callback.h"
#include "net/base/isolation_info.h"
#include "net/base/network_isolation_key.h"
#include "content/public/browser/storage_partition.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

#include "base/no_destructor.h"
#include "base/synchronization/lock.h"

#include "brave/browser/net/brave_web3_service.h"

namespace Solana_Rpc{

    class SolanaApiRequest{

    public:
        SolanaApiRequest();

        ~SolanaApiRequest();

        void SendJsonRequestWithFactory(
            const base::Value::Dict& request_json, 
            scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
            base::OnceCallback<void(std::string, scoped_refptr<network::SharedURLLoaderFactory>)> call_back
        );

        void SendJsonRequestWithContent(
            const base::Value::Dict& request_json, 
            scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
            base::OnceCallback<void(std::string)> call_back
        );

        void SendJsonRequestWithIpfsStart(
            const base::Value::Dict& request_json, 
            scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
            base::OnceCallback<void(const GURL&, bool is_web3_domain)> restart_callback,
            std::string maybe_domain
        );

    private:
        base::WeakPtrFactory<SolanaApiRequest> weak_factory_{this};
    };



    class SolanaRootMap{
    public:
        static SolanaRootMap& instance();

        bool has_loaded = false;

        void set_all(const std::vector<std::string>& values) {
            base::AutoLock locker(lock_);
            data_ = values;
        }

        void set_all_pubkey(const std::vector<Solana_web3::Pubkey>& pubkeys) {
            base::AutoLock locker(lock_);
            pubkeys_ = pubkeys;
        }

        std::vector<std::string> get_all() const {
            base::AutoLock locker(lock_);
            return data_;
        }

        std::vector<Solana_web3::Pubkey> get_all_pubkey() const {
            base::AutoLock locker(lock_);
            return pubkeys_;
        }

        void reverse_load_state(){
            this->has_loaded = true;
        }

        private:
        friend class base::NoDestructor<SolanaRootMap>;
        SolanaRootMap();
        ~SolanaRootMap(); 

        mutable base::Lock lock_;
        std::vector<std::string> data_;
        std::vector<Solana_web3::Pubkey> pubkeys_;
    };

    enum class DecodeType {
        Domain,
        Cid,
    };

    int base64_char_value(char c);

    int base64_decode(const char *input, unsigned char **output, size_t *output_len);

    std::string decodeAndStripPubkeys(const std::string& base64_str, const DecodeType type);

    base::Value::Dict build_request_json(
        const std::string& method,
        const base::Value::List& params,
        const absl::optional<int> request_id = 1,
        const bool filters = false
    );

    base::Value::List build_root_filters();
    
    base::Value::List build_common_request_args(
        const base::Value::List& pubkey_array,
        bool multiple = false
    );

    GURL getCidUrlFromContent(
        std::string content, std::string maybe_domain
    );

    void update_root_map(
        std::string content
    );

    void get_account_info_and_restart(
        base::Value::List& publickey,
        scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
        base::OnceCallback<void(const GURL&, bool is_web3_domain)> restart_callback,
        std::string maybe_domain
    );

    void get_all_root_pubkey(
        std::string contents,
        scoped_refptr<network::SharedURLLoaderFactory> factory
    );

    void get_all_root_domain(
        scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory
    );
}








#endif
