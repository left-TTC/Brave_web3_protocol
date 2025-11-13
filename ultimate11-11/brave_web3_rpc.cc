

#include "brave_web3_rpc.h"
#include "brave/browser/net/brave_web3_task.h"



namespace Solana_Rpc{

    int base64_char_value(char c) {
        if ('A' <= c && c <= 'Z') return c - 'A';
        if ('a' <= c && c <= 'z') return c - 'a' + 26;
        if ('0' <= c && c <= '9') return c - '0' + 52;
        if (c == '+') return 62;
        if (c == '/') return 63;
        return -1; 
    }

    int base64_decode(const char* input, std::vector<uint8_t>& output) {
        size_t input_len = strlen(input);
        size_t estimated_len = input_len * 3 / 4;
        output.clear();
        output.reserve(estimated_len);

        int val = 0;
        int valb = -8;

        for (size_t i = 0; i < input_len; ++i) {
            char c = input[i];
            if (c == '=') break;
            int d = base64_char_value(c);
            if (d < 0) continue;
            val = (val << 6) + d;
            valb += 6;
            if (valb >= 0) {
                output.push_back((val >> valb) & 0xFF);
                valb -= 8;
            }
        }

        return 0;
    }

    /**
     * @brief   get account's on chain datas, and extract cid from the datas
     *
     * @param   publickey    accounts that needs to get data
     * @return  json         obtained data
     *          nullopt      no data has been retrieved
     */
    std::string decodeAndStripPubkeys(const std::string& base64_str, const DecodeType type) {
        std::vector<uint8_t> decoded_bytes;
        if (base64_decode(base64_str.c_str(), decoded_bytes) != 0 || decoded_bytes.size() <= 96) {
            return "";
        }

        size_t cut_length;
        switch (type) {
            case DecodeType::Domain:
                cut_length = 108;
                break;
            case DecodeType::Cid:
                cut_length = 104;
                break;
        }

        if (decoded_bytes.size() <= cut_length) {
            return "";
        }

        return std::string(decoded_bytes.begin() + cut_length, decoded_bytes.end());
    }


    base::Value::List build_common_request_args(
        const base::Value::List& pubkey_array,
        bool multiple
    ){
        base::Value::List args;
        if(pubkey_array.size() > 1 || multiple){
            base::Value::List pubkeys;
            for(const auto& pubkey: pubkey_array){
                if (pubkey.is_string()) {
                    pubkeys.Append(pubkey.GetString());
                }
            }
            args.Append(std::move(pubkeys));
        }else{
            args = pubkey_array.Clone();
        }

        base::Value::Dict option_args;
        option_args.Set("commitment", "confirmed");
        option_args.Set("encoding", "base64");

        args.Append(std::move(option_args));

        return args;
    }

    base::Value::List build_root_filters(){
        base::Value::List filters;

        {
            base::Value::Dict memcmp_one_params;
            const std::string central_state_register = Solana_web3::return_REGISTERSTATE().toBase58();
            memcmp_one_params.Set("bytes", central_state_register);
            memcmp_one_params.Set("offset", 32);

            base::Value::Dict memcmp_one;
            memcmp_one.Set("memcmp", std::move(memcmp_one_params));
            filters.Append(std::move(memcmp_one));
        }

        {
            base::Value::Dict memcmp_two_params;
            memcmp_two_params.Set("bytes", "11111111111111111111111111111111");
            memcmp_two_params.Set("offset", 0);

            base::Value::Dict memcmp_two;
            memcmp_two.Set("memcmp", std::move(memcmp_two_params));
            filters.Append(std::move(memcmp_two));
        }

        {
            base::Value::Dict memcmp_three_params;
            memcmp_three_params.Set("bytes", "11111111111111111111111111111111");
            memcmp_three_params.Set("offset", 64);

            base::Value::Dict memcmp_three;
            memcmp_three.Set("memcmp", std::move(memcmp_three_params));
            filters.Append(std::move(memcmp_three));
        }

        return filters;
    }

    base::Value::Dict build_request_json(
        const std::string& method,
        const base::Value::List& params,
        const absl::optional<int> request_id,
        const bool filters_enabled 
    ) {
        base::Value::Dict request_json;
        request_json.Set("jsonrpc", "2.0");
        if (request_id.has_value())
            request_json.Set("id", *request_id);
        request_json.Set("method", method);

        if ((method == "getAccountInfo") || (method == "getMultipleAccounts")) {
            request_json.Set("params", base::Value(params.Clone()));
        } else if (method == "getProgramAccounts" && filters_enabled) {
            base::Value::List params_list;

            // Append program ID
            params_list.Append(Solana_web3::return_NAMESERVICE().toBase58());

            // Build param options
            base::Value::Dict options;

            base::Value::Dict data_slice;
            data_slice.Set("length", 0);
            data_slice.Set("offset", 0);
            options.Set("dataSlice", std::move(data_slice));

            options.Set("encoding", "base64");

            // Filters: expects caller to pass already-built filter list
            options.Set("filters", base::Value(params.Clone()));

            // Add full dict
            params_list.Append(std::move(options));

            request_json.Set("params", std::move(params_list));
        }

        return request_json;
    }


    SolanaRootMap::SolanaRootMap() = default;
    SolanaRootMap::~SolanaRootMap() = default;

    SolanaRootMap& SolanaRootMap::instance() {
        static base::NoDestructor<SolanaRootMap> instance;
        return *instance;
    }

    void update_root_map(
        std::string content
    ){
        LOG(INFO) << "root name info: " << content;
        absl::optional<base::Value> parsed = base::JSONReader::Read(content);
        if (!parsed.has_value()) {
            LOG(ERROR) << "Failed to parse JSON";
            return;
        }
        if (!parsed->is_dict()) {
            LOG(ERROR) << "Parsed JSON is not a dictionary";
            return;
        }

        const base::Value::Dict& roots_json = parsed->GetDict();
        //array of root domain's info
        const base::Value::Dict* result_dict = roots_json.FindDict("result");
        if (!result_dict) {
            LOG(ERROR) << "No 'result' field or not a dictionary";
            return;
        }
        const base::Value::List* value_list = std::move(result_dict)->FindList("value");
        if (!value_list) {
            LOG(ERROR) << "No 'value' field or not a list";
            return;
        }

        std::vector<std::string> roots;
        for(const auto& account_value: *value_list){
            if (!account_value.is_dict()) continue;

            const base::Value::Dict& account_dict = account_value.GetDict();
            const base::Value::List* data_list = account_dict.FindList("data");
            if (!data_list) continue;

            for(const auto& account_data: *data_list){
                std::string data = account_data.GetString();
                if(data.size() <= 96){
                    continue;
                }
                roots.push_back(decodeAndStripPubkeys(data, Solana_Rpc::DecodeType::Domain));
            }
        }

        SolanaRootMap& rootMap = SolanaRootMap::instance();
        rootMap.set_all(roots);
    }

    

    GURL getCidUrlFromContent(std::string content, std::string maybe_domain){
        LOG(INFO) << "ipfs query result: " << content;

        absl::optional<base::Value> parsed = base::JSONReader::Read(content);
        if (!parsed || !parsed->is_dict()) {
            LOG(ERROR) << "Failed to parse JSON";
            return GURL("chrome://newtab/");
        }

        const base::Value::Dict* result_dict = parsed->GetDict().FindDict("result");
        if (!result_dict) {
            LOG(ERROR) << "No 'result' field found";
            return GURL("chrome://newtab/");
        }

        const base::Value::Dict* value_dict = result_dict->FindDict("value");
        if (!value_dict) {
            LOG(ERROR) << "No 'value' field found";
            return GURL("chrome://newtab/");
        }

        const base::Value::List* data_list = value_dict->FindList("data");
        if (!data_list || data_list->size() < 2) {
            LOG(ERROR) << "Invalid 'data' field";
            return GURL("chrome://newtab/");
        }

        const std::string* encoded_data = (*data_list)[0].GetIfString();

        if(encoded_data){
            const std::string cid = decodeAndStripPubkeys(*encoded_data, DecodeType::Cid);
            const std::string ultimate_url_str = "https://ipfs.io/ipns/" + cid + "/#/";

            Brave_web3_solana_task::DomainCidMap& domain_cid_map = Brave_web3_solana_task::DomainCidMap::instance();
            domain_cid_map.insert_or_update(maybe_domain, cid);

            LOG(INFO) << "url ipfs: " << ultimate_url_str;

            return GURL(ultimate_url_str);
        }

        return GURL("chrome://newtab/");
    }


    void get_account_info_and_restart(
        base::Value::List& publickey,
        scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
        base::OnceCallback<void(const GURL&, bool is_web3_domain)> restart_callback,
        std::string maybe_domain
    ){
        const std::string method = "getAccountInfo";

        base::Value::List param = build_common_request_args(publickey);
        base::Value::Dict request_json = build_request_json(method, std::move(param));
        LOG(INFO) << "get ipfs acount info request json: " << request_json;

        auto request_sender = std::make_unique<SolanaApiRequest>();

        request_sender->SendJsonRequestWithIpfsStart(request_json, url_loader_factory, std::move(restart_callback), std::move(maybe_domain));
    }


    void get_all_root_pubkey(
        std::string contents,
        scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory
    ){
        LOG(INFO)<<"update raw source: "<< contents; 
        
        absl::optional<base::Value> parsed = base::JSONReader::Read(contents);
        if (!parsed.has_value()) {
            LOG(ERROR) << "Failed to parse JSON";
            return;
        }
        if (!parsed->is_dict()) {
            LOG(ERROR) << "Parsed JSON is not a dictionary";
            return;
        }

        const base::Value::Dict& roots_json = parsed->GetDict();

        const base::Value::List* result_list = roots_json.FindList("result");
        if(!result_list){
            LOG(ERROR) << "No 'result' field or not a list";
            return;
        }

        std::vector<Solana_web3::Pubkey> pubkeys;
        base::Value::List pubkey_list;
        for (const auto& pubket_info: *result_list){
            if (!pubket_info.is_dict()) continue;

            const base::Value::Dict& item = pubket_info.GetDict();

            const std::string* pubkey = item.FindString("pubkey");
            if (pubkey){
                const std::string combined_string = Solana_web3::PREFIX + *pubkey ;
                const std::vector<uint8_t> combined_domain_bytes(combined_string.begin(), combined_string.end());  
                
                std::array<uint8_t, Solana_web3::Pubkey::LENGTH> hash_domain; 
                Solana_web3::Solana_web3_interface::sha_256(combined_domain_bytes, hash_domain);

                std::vector<std::vector<uint8_t>> domain_account_seeds;
                domain_account_seeds.push_back(std::vector<uint8_t>(hash_domain.begin(), hash_domain.end()));
                
                const Solana_web3::Pubkey central_state_auction = Solana_web3::return_REGISTERSTATE();
                domain_account_seeds.push_back(std::vector<uint8_t>(central_state_auction.bytes.begin(), central_state_auction.bytes.end()));
                domain_account_seeds.push_back(std::vector<uint8_t>(32, 0));

                const Solana_web3::PDA root_reverse_key = Solana_web3::Solana_web3_interface::try_find_program_address_cxx(domain_account_seeds, Solana_web3::return_NAMESERVICE());
                
                pubkey_list.Append(root_reverse_key.publickey.toBase58());
                pubkeys.push_back(Solana_web3::Pubkey(*pubkey));
            }
        }

        if(pubkeys.size() == 0){
            return;
        }
        SolanaRootMap& rootMap = SolanaRootMap::instance();
        rootMap.set_all_pubkey(pubkeys);

        const std::string method = "getMultipleAccounts";

        base::Value::List params = build_common_request_args(pubkey_list, true);
        base::Value::Dict request_json = build_request_json(method, params);

        std::cout << "multiple request: " << request_json << std::endl;

        auto request_sender = std::make_unique<SolanaApiRequest>();
        request_sender->SendJsonRequestWithContent(request_json, url_loader_factory, base::BindOnce(&update_root_map));
    }



    void get_all_root_domain(
        scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory
    ){
        
        const base::Value::List root_filters = build_root_filters();
        LOG(INFO) << "fliters:" << root_filters;

        const std::string method = "getProgramAccounts";
        base::Value::Dict request = build_request_json(method, root_filters, 1, true);
        LOG(INFO) << "request: " << request;

        auto request_sender = std::make_unique<SolanaApiRequest>();

        request_sender->SendJsonRequestWithFactory(request, url_loader_factory, base::BindOnce(&get_all_root_pubkey));
    }



    //======================= rpc ==========================



    SolanaApiRequest::SolanaApiRequest() = default;


    SolanaApiRequest::~SolanaApiRequest() = default;


    void SolanaApiRequest::SendJsonRequestWithFactory(
        const base::Value::Dict& request_json, 
        scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
        base::OnceCallback<void(std::string, scoped_refptr<network::SharedURLLoaderFactory>)> call_back
    ) {

        net::NetworkTrafficAnnotationTag traffic_annotation =
            net::DefineNetworkTrafficAnnotation("solana_api_request", R"(
                semantics {
                sender: "Chromium Browser"
                description: "Sends a JSON RPC request to the Solana Devnet API."
                trigger: "User initiated action or internal browser process."
                data: "JSON RPC request payload, typically method name and parameters for blockchain interaction."
                destination: OTHER
                destination_other: "Solana Devnet API"
                }
                policy {
                cookies_allowed: NO
                setting_and_preference_disabled_by_policy: false
                policy_exception_justification: "This request is part of a feature that interacts with a public blockchain API. No sensitive user data is sent unless explicitly provided by the user within the JSON payload."
                }
            )");

        auto resource_request = std::make_unique<network::ResourceRequest>();
        const GURL Solana_api = Solana_web3::RpcUrl();

        resource_request->url = Solana_api;
        resource_request->method = "POST";
        resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;

        std::unique_ptr<network::SimpleURLLoader> loader =
            network::SimpleURLLoader::Create(std::move(resource_request),
                                            traffic_annotation);

        std::string json_string;
        base::JSONWriter::Write(request_json, &json_string);

        loader->AttachStringForUpload(json_string, "application/json");

        auto* loader_ptr = loader.get();
        loader_ptr->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
            url_loader_factory.get(),
            base::BindOnce(
                [](std::unique_ptr<network::SimpleURLLoader> loader,
                    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
                    base::OnceCallback<void(std::string, scoped_refptr<network::SharedURLLoaderFactory>)> call_back,
                    std::unique_ptr<std::string> response) {
                        std::string content = response ? *response : "";
                        scoped_refptr<network::SharedURLLoaderFactory> factory = url_loader_factory;
                        std::move(call_back).Run(content, factory);
                    },
                std::move(loader), url_loader_factory, std::move(call_back)));
    }



    void SolanaApiRequest::SendJsonRequestWithContent(
        const base::Value::Dict& request_json, 
        scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
        base::OnceCallback<void(std::string)> call_back
    ) {

        net::NetworkTrafficAnnotationTag traffic_annotation =
            net::DefineNetworkTrafficAnnotation("solana_api_request", R"(
                semantics {
                sender: "Chromium Browser"
                description: "Sends a JSON RPC request to the Solana Devnet API."
                trigger: "User initiated action or internal browser process."
                data: "JSON RPC request payload, typically method name and parameters for blockchain interaction."
                destination: OTHER
                destination_other: "Solana Devnet API"
                }
                policy {
                cookies_allowed: NO
                setting_and_preference_disabled_by_policy: false
                policy_exception_justification: "This request is part of a feature that interacts with a public blockchain API. No sensitive user data is sent unless explicitly provided by the user within the JSON payload."
                }
            )");

        auto resource_request = std::make_unique<network::ResourceRequest>();
        const GURL Solana_api = Solana_web3::RpcUrl();

        resource_request->url = Solana_api;
        resource_request->method = "POST";
        resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;

        std::unique_ptr<network::SimpleURLLoader> loader =
            network::SimpleURLLoader::Create(std::move(resource_request),
                                            traffic_annotation);

        std::string json_string;
        base::JSONWriter::Write(request_json, &json_string);

        loader->AttachStringForUpload(json_string, "application/json");

        auto* loader_ptr = loader.get();
        loader_ptr->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
            url_loader_factory.get(),
            base::BindOnce(
                [](std::unique_ptr<network::SimpleURLLoader> loader,
                    base::OnceCallback<void(std::string)> call_back,
                    std::unique_ptr<std::string> response) {
                        std::string content = response ? *response : "";
                        std::move(call_back).Run(content);
                    },
                std::move(loader), std::move(call_back)));
    }

    void SolanaApiRequest::SendJsonRequestWithIpfsStart(
        const base::Value::Dict& request_json, 
        scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
        base::OnceCallback<void(const GURL&, bool is_web3_domain)> restart_callback,
        std::string maybe_domain
    ){

        net::NetworkTrafficAnnotationTag traffic_annotation =
            net::DefineNetworkTrafficAnnotation("solana_api_request", R"(
                semantics {
                sender: "Chromium Browser"
                description: "Sends a JSON RPC request to the Solana Devnet API."
                trigger: "User initiated action or internal browser process."
                data: "JSON RPC request payload, typically method name and parameters for blockchain interaction."
                destination: OTHER
                destination_other: "Solana Devnet API"
                }
                policy {
                cookies_allowed: NO
                setting_and_preference_disabled_by_policy: false
                policy_exception_justification: "This request is part of a feature that interacts with a public blockchain API. No sensitive user data is sent unless explicitly provided by the user within the JSON payload."
                }
            )");

        auto resource_request = std::make_unique<network::ResourceRequest>();
        const GURL Solana_api = Solana_web3::RpcUrl();

        resource_request->url = Solana_api;
        resource_request->method = "POST";
        resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;

        std::unique_ptr<network::SimpleURLLoader> loader =
            network::SimpleURLLoader::Create(std::move(resource_request),
                                            traffic_annotation);

        std::string json_string;
        base::JSONWriter::Write(request_json, &json_string);

        loader->AttachStringForUpload(json_string, "application/json");

        auto* loader_ptr = loader.get();
        loader_ptr->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
            url_loader_factory.get(),
            base::BindOnce(
                [](std::unique_ptr<network::SimpleURLLoader> loader,
                    base::OnceCallback<void(const GURL&, bool is_web3_domain)> restart_callback,
                    std::string maybe_domain,
                    std::unique_ptr<std::string> response) {
                        std::string content = response ? *response : "";
                        GURL ultimate_url = getCidUrlFromContent(content, maybe_domain);
                        std::move(restart_callback).Run(ultimate_url, true);
                    },
                std::move(loader), std::move(restart_callback), std::move(maybe_domain)));
    }

}