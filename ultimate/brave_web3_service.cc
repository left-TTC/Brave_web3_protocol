


#include "brave_web3_service.h"
#include "brave_web3_rpc.h"



namespace Solana_web3{

    GURL RpcUrl() {
        return GURL("https://api.devnet.solana.com");
    }


    /**
     * @brief convert bytes to readable string
     */
    std::string EncodeBase58(const std::vector<uint8_t> &input){
        std::vector<uint8_t> digits;
        digits.push_back(0);

        const std::string BASE58_ALPHABET = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
        
        for (size_t i = 0; i < input.size(); i++) {
            uint32_t carry = input[i];
            for (size_t j = 0; j < digits.size(); j++) {
                carry += digits[j] << 8;
                digits[j] = carry % 58;
                carry = carry / 58;
            }   
            while (carry > 0) {
                digits.push_back(carry % 58);
                carry = carry / 58;
            }
        }
        
        std::string result;
        for (size_t i = 0; i < input.size() && input[i] == 0; i++) {
            result.push_back(BASE58_ALPHABET[0]);
        }
        for (auto it = digits.rbegin(); it != digits.rend(); it++) {
            result.push_back(BASE58_ALPHABET[*it]);
        }
        
        return result;
    }

    /**
     * @brief convert string to uint8 bytes
     */
    absl::optional<std::vector<uint8_t>> DecodeBase58(const std::string& input) {
        std::vector<uint8_t> bytes;
        bytes.push_back(0);

        constexpr char kBase58Alphabet[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
        const std::string BASE58_ALPHABET(kBase58Alphabet);

        for (size_t i = 0; i < input.size(); i++) {
            size_t index = BASE58_ALPHABET.find(input[i]);
            if (index == std::string::npos) {
                // Invalid character
                return absl::nullopt;
            }

            uint32_t carry = static_cast<uint32_t>(index);
            for (size_t j = 0; j < bytes.size(); j++) {
                carry += bytes[j] * 58;
                bytes[j] = carry & 0xff;
                carry >>= 8;
            }
            while (carry > 0) {
                bytes.push_back(carry & 0xff);
                carry >>= 8;
            }
        }

        for (size_t i = 0; i < input.size() && input[i] == BASE58_ALPHABET[0]; i++) {
            bytes.push_back(0);
        }

        std::reverse(bytes.begin(), bytes.end());
        return bytes;
    }


    // ===========================================================
    //         ██████  SECTION 2: Pubkey class  ██████
    // ===========================================================
    /**
     * function: Pubkey class's method and implementation
     */

    Pubkey::Pubkey() {
        std::fill(bytes.begin(), bytes.end(), 0);
    }

    //construct Pubkey by byte arrays
    Pubkey::Pubkey(const std::array<uint8_t, LENGTH>& b) : bytes(b) {}

    //construct Pubkey from string
    Pubkey::Pubkey(const std::string& pubkey_b58) {
        absl::optional<std::vector<uint8_t>> decoded_bytes = DecodeBase58(pubkey_b58);
        if (!decoded_bytes.has_value()) {
            return;
        }

        if (decoded_bytes.value().size() != LENGTH) {
            return;
        }
        std::copy(decoded_bytes.value().begin(), decoded_bytes.value().end(), bytes.begin());
    }

    static std::vector<uint8_t> array_to_vector(const std::array<uint8_t, Pubkey::LENGTH>& arr) {
        return std::vector<uint8_t>(arr.begin(), arr.end());
    }

    std::string Pubkey::toBase58() const{
        return EncodeBase58(array_to_vector(this->bytes));
    }

    std::size_t Pubkey::size() const{
        return this->bytes.size();
    }

    bool Pubkey::is_on_curve() const{
        return Web3_libsodium::is_solana_PDA_valid(bytes.data()) != 0;
    }

    void Pubkey::get_pubkey_ipfs(
        scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
        base::OnceCallback<void(const GURL&, bool is_web3_domain)> restart_callback,
        const std::string maybe_domain,
        const GURL& original_url
    ) const {
        base::Value::List pubkeys;
        pubkeys.Append(this->toBase58());

        Solana_Rpc::get_account_info_and_restart(pubkeys, url_loader_factory, std::move(restart_callback), std::move(maybe_domain), original_url);
    }

    //-------------------------------------------

    Pubkey return_NAMESERVICE(){
        return Pubkey("BEUuE7dmW6yyTVhyecrHktKF1necQvfiZdFyBCBjCXQm");
    }

    Pubkey return_RECORDSTATE(){
        return Pubkey("7rQ3CCVi46zRrFMrY5nnQX2G6qvQrMKJcMcbyoCUyJDB");
    }

    Pubkey return_REGISTERSTATE(){
        return Pubkey("MLUCvnLm7HHBiHnr8bHA6XMZQJbMXewocDRvpaVhFcf");
    }


    namespace Solana_web3_interface{

        void sha_256(const std::vector<uint8_t>& input_data, std::array<uint8_t, Pubkey::LENGTH>& output_hash){
            static_assert(Pubkey::LENGTH == crypto_hash_sha256_BYTES, "SHA256 output must be 32 bytes");

            if (Web3_libsodium::crypto_hash_sha256(output_hash.data(), input_data.data(), input_data.size()) != 0) {
                return;
            }
        }

        absl::optional<Pubkey> create_program_address_cxx(
            const std::vector<std::vector<uint8_t>>& seeds,
            const Pubkey& program_id
        ){
            //Determining the seeds' amounts
            if(seeds.size() > MAX_SEEDS){
                return std::nullopt;
            }
            
            //Determining the length of the seed 
            for(const std::vector<uint8_t> &seed: seeds){
                if(seed.size() > MAX_SEED_LEN){
                    return std::nullopt;
                }
            }

            //Combine all the seeds to one data
            std::vector<uint8_t> combined_data;
            size_t total_seeds_len = 0;
            for (const auto& seed: seeds){
                total_seeds_len += seed.size();
            }
            //Allocate capacity for data
            combined_data.reserve(total_seeds_len + Pubkey::LENGTH + PDA_MARKER.size());
            for (const auto& seed : seeds) {
                combined_data.insert(combined_data.end(), seed.begin(), seed.end());
            }

            // Append program_id bytes
            combined_data.insert(combined_data.end(), program_id.bytes.begin(), program_id.bytes.end());
            // Append the magic PDA marker
            combined_data.insert(combined_data.end(), PDA_MARKER.begin(), PDA_MARKER.end());

            std::array<uint8_t, Pubkey::LENGTH> hash_result;
            sha_256(combined_data, hash_result);

            Pubkey publickey(hash_result);

            if (publickey.is_on_curve()) {
                return std::nullopt;
            }

            return publickey;
        }

        PDA try_find_program_address_cxx(
            const std::vector<std::vector<uint8_t>>& seeds,
            const Pubkey& program_id
        ) {

            for(uint8_t bump = 255; bump >= 0; --bump){
                std::vector<std::vector<uint8_t>> seeds_with_bump = seeds;
                seeds_with_bump.push_back({ static_cast<uint8_t>(bump) });

                const absl::optional<Pubkey> create_res = create_program_address_cxx(seeds_with_bump, program_id);

                if(create_res.has_value()){
                    const PDA result = PDA{
                        create_res.value(),
                        bump,
                    };
                    return result;
                }
            }

            return empty_PDA;
        }

        PDA get_account_from_root(const std::string& domain, const Pubkey &root_domain_account){
            const std::string combined_domain = PREFIX + domain;
            const std::vector<uint8_t> combined_domain_bytes(combined_domain.begin(), combined_domain.end());
            
            std::array<uint8_t, Pubkey::LENGTH> hash_domain;
            sha_256(combined_domain_bytes, hash_domain);
            std::vector<std::vector<uint8_t>> domain_account_seeds;

            domain_account_seeds.push_back(std::vector<uint8_t>(hash_domain.begin(), hash_domain.end()));
            domain_account_seeds.push_back(std::vector<uint8_t>(32, 0));
            domain_account_seeds.push_back(std::vector<uint8_t>(root_domain_account.bytes.begin(), root_domain_account.bytes.end()));
            //get domain account key
            const Pubkey name_service = return_NAMESERVICE();
            auto domain_account_PDA = try_find_program_address_cxx(domain_account_seeds, name_service);
        
            //now we can calculate the ipfs account
            std::vector<std::vector<uint8_t>> domain_ipfs_seeds;
            const std::string combined_ipfs = PREFIX + "IPFS";
            const std::vector<uint8_t> ipfs_account_bytes(combined_ipfs.begin(), combined_ipfs.end());

            std::array<uint8_t, Pubkey::LENGTH> hash_ipfs;
            sha_256(ipfs_account_bytes, hash_ipfs);
            std::vector<std::vector<uint8_t>> ipfs_account_seeds;

            const Pubkey record_central = return_RECORDSTATE();
            ipfs_account_seeds.push_back(std::vector<uint8_t>(hash_ipfs.begin(), hash_ipfs.end()));
            ipfs_account_seeds.push_back(std::vector<uint8_t>(record_central.bytes.begin(), record_central.bytes.end()));
            ipfs_account_seeds.push_back(std::vector<uint8_t>(domain_account_PDA.publickey.bytes.begin(), domain_account_PDA.publickey.bytes.end()));

            auto ipfs_account_PDA = try_find_program_address_cxx(ipfs_account_seeds, std::move(name_service));

            return ipfs_account_PDA;
            
        }
    }

    // if this is a strange web3 domain
    // it will turn to a search result
    // such as origin: "https://search.brave.com/search?q=x.web3&source=desktop"
    std::string extract_target_domain(const GURL& original_url) {
        std::string host = original_url.host();

        if (host.find("google.") != std::string::npos ||
            host.find("bing.") != std::string::npos ||
            host.find("brave.") != std::string::npos) {
            
            std::string query = original_url.query();
            size_t pos = query.find("q=");
            if (pos != std::string::npos) {
                std::string target = query.substr(pos + 2);
                size_t amp_pos = target.find('&');
                if (amp_pos != std::string::npos)
                    target = target.substr(0, amp_pos);
                return target;
            }
        }

        return host; 
    }

    // We only support first-level domain names
    std::vector<std::string> split_host_by_dots(const std::string& url_host){
        std::vector<std::string> parts;
        size_t start = 0;
        size_t end = url_host.find('.');

        while (end != std::string::npos) {
            parts.push_back(url_host.substr(start, end - start));
            start = end + 1;
            end = url_host.find('.', start);
        }
        
        parts.push_back(url_host.substr(start));
        
        return parts;
    }

    std::tuple<int, bool, std::string> fast_find(
        const std::string maybe_domain, 
        const std::vector<std::string>& vec
    ){
        const std::vector<std::string> host_parts = split_host_by_dots(std::move(maybe_domain));

        if (host_parts.size() != 2) {
            return std::make_tuple(-1, false, "");
        }

        const std::string& tld = host_parts.back();     // root
        const std::string& name = host_parts.front();   // domain

        for (size_t i = 0; i < vec.size(); ++i) {
            if (vec[i] == tld) {
                return std::make_tuple(static_cast<int>(i), true, name);
            }
        }

        return std::make_tuple(-1, false, "");
    }




}