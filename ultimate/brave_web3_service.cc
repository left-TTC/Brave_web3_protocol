


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
        base::OnceCallback<void(const GURL&, bool is_web3_domain)> restart_callback
    ) const {
        base::Value::List pubkeys;
        pubkeys.Append(this->toBase58());

        Solana_Rpc::get_account_info_and_restart(pubkeys, url_loader_factory, std::move(restart_callback));
    }

    //-------------------------------------------

    Pubkey return_NAMESERVICE(){
        return Pubkey("8YXaA8pzJ4xVPjYY8b5HkxmPWixwpZu7gVcj8EvHxRDC");
    }

    Pubkey return_AUCTIONSERVICE(){
        return Pubkey("9qQuHLMAJEehtk47nKbY1cMAL1bVD7nQxno4SJRDth7");
    }

    Pubkey return_ACUTIONSTATE(){
        return Pubkey("2DnqJcAMA5LPXcQN1Ep1rJNbyXXSofmbXdcweLwyoKq7");
    }

    Pubkey return_RECORDSTATE(){
        return Pubkey("Ha57yHecoA8iepHAX4LY6wG8YWnr47MjcZaPGN3G7XAv");
    }

    Pubkey return_REGISTERSTATE(){
        return Pubkey("ERCnYDoLmfPCvmSWB52mubuDgPb7CwgUmfourQJ3sK7d");
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
        const GURL& original_url, 
        const std::vector<std::string>& vec
    ){
        const std::string domain_host = original_url.host();
        const std::vector<std::string> host_parts = split_host_by_dots(std::move(domain_host));

        for (size_t i = 0; i < vec.size(); ++i) {
            if (vec[i] == host_parts[host_parts.size() - 1]) {
                std::string domain;
                for(size_t k = 0; k < host_parts.size() - 2; ++k){
                    LOG(INFO) << "domain part: " << host_parts[k];
                    domain = domain + host_parts[k] + ".";
                }

                domain += host_parts[host_parts.size() - 2];

                return make_tuple(static_cast<int>(i), true, domain);
            }
        }
        return std::make_tuple(-1, false, "");
    }
}