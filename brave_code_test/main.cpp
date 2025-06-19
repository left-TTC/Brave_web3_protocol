
#include <iostream>
#include "brave_web3_service.h"
#include "brave_web3_rpc.h"
#include <iomanip> 

#include "json.hpp"

using json = nlohmann::json;


using namespace std;

void test_function1(){
    cout << "This is the frist test" << endl;

    cout << "[1] test start" << endl;

    cout << "[1] test decode" <<endl;

    const std::array<uint8_t, 32> known_pubkey_bytes_arr = {
        0x4d, 0xd9, 0x13, 0x8a, 0xea, 0x98, 0x42, 0xc2,
        0xd4, 0x37, 0xad, 0x12, 0x5a, 0x6f, 0xb2, 0x5a,
        0x7b, 0x1e, 0xd3, 0x63, 0x29, 0xc6, 0x66, 0xe9,
        0x3d, 0x4c, 0xdc, 0x66, 0x4e, 0xc9, 0x54, 0x0a
    };

    std::vector<uint8_t> pubkey_vector(known_pubkey_bytes_arr.begin(), known_pubkey_bytes_arr.end());

    const string test_pubkey = "6EtNpLmx3n133UhLvNSkXyCXkkRCZqNrp37kNmw41urq";
    cout << "output should be " << test_pubkey << endl;

    const Solana_web3::Pubkey test_Pubkey = Solana_web3::Pubkey(test_pubkey); 
    cout << "test pubkey's base58:" << test_Pubkey.toBase58() << endl;
    
    cout << "now dncode" <<endl;
    const vector<uint8_t> pubkey_byte = Solana_web3::DecodeBase58(test_Pubkey.toBase58());
    const string encode = Solana_web3::EncodeBase58(pubkey_vector);
    cout << "result:" << (pubkey_vector == pubkey_byte) << endl;
    cout << "result:" << encode << endl;
}


void print_seeds_json_like(const std::vector<std::vector<uint8_t>>& seeds) {
    std::cout << "[";
    for (size_t i = 0; i < seeds.size(); ++i) {
        std::cout << "[";
        for (size_t j = 0; j < seeds[i].size(); ++j) {
            std::cout << (int)seeds[i][j];
            if (j != seeds[i].size() - 1) std::cout << ", ";
        }
        std::cout << "]";
        if (i != seeds.size() - 1) std::cout << ", ";
    }
    std::cout << "]" << std::endl;
}


void test_function2(){
    cout << "this is the second test -- test curve and sha256" << endl;

    string test_domain = "test";
    const string Pre = "WEB3 Name Service";
    const std::string combined = Pre + test_domain;
    std::vector<uint8_t> bytes(combined.begin(), combined.end());
    std::array<uint8_t, Solana_web3::Pubkey::LENGTH> hash_result;
    Solana_web3::Solana_web3_interface::sha_256(bytes, hash_result);

    std::cout << "[";
    for (size_t i = 0; i < hash_result.size(); ++i) {
        std::cout << static_cast<int>(hash_result[i]);
        if (i != hash_result.size() - 1) {
            std::cout << ", ";
        }
    }
    std::cout << "]" << std::endl;

    Solana_web3::Pubkey test_program_id = Solana_web3::Pubkey("8YXaA8pzJ4xVPjYY8b5HkxmPWixwpZu7gVcj8EvHxRDC");

    std::vector<std::vector<uint8_t>> seeds;
    for(size_t i = 0; i < hash_result.size(); i += 32) {
        size_t end = std::min(i + 32, hash_result.size());
        seeds.push_back(std::vector<uint8_t>(hash_result.begin() + i, hash_result.begin() + end));
    }

    //[106, 242, 169, 57, 75, 100, 138, 26, 118, 8, 20, 106, 171, 69, 13, 171, 169, 12, 255, 215, 88, 30, 151, 173, 172, 58, 4, 203, 162, 69, 55, 83]
    //[106, 242, 169, 57, 75, 100, 138, 26, 118, 8, 20, 106, 171, 69, 13, 171, 169, 12, 255, 215, 88, 30, 151, 173, 172, 58, 4, 203, 162, 69, 55, 83]
    print_seeds_json_like(seeds);

    auto result = Solana_web3::Solana_web3_interface::try_find_program_address_cxx(seeds, test_program_id);

    if (result.has_value()) {
        Solana_web3::Pubkey pda = result->first;
        uint8_t bump = result->second;
        std::cout << "PDA: " << pda.toBase58() << ", bump: " << static_cast<int>(bump) << std::endl;
        //F6XgnHw3HVffBJLhhLnuu4XGQtKYz2TxYP2oJUCuxBfQ
        //F6XgnHw3HVffBJLhhLnuu4XGQtKYz2TxYP2oJUCuxBfQ
    } else {
        std::cerr << "Failed to find PDA." << std::endl;
    }
}


void test_function3(){
    Solana_web3::Pubkey test_publickey = Solana_web3::Pubkey("8YXaA8pzJ4xVPjYY8b5HkxmPWixwpZu7gVcj8EvHxRDC");

    json pubkey_list = json::array();
    pubkey_list.push_back(test_publickey.toBase58());

    const std::optional<Solana_Rpc::commitment> confirm_level = Solana_Rpc::commitment();

    json request_params = Solana_Rpc::build_request_args(
        pubkey_list, confirm_level
    );

    const std::string get_account_info = "getAccountInfo";

    json rpc_request = Solana_Rpc::build_rpc_request(
        get_account_info, request_params
    );

    std::cout << rpc_request.dump(4) << std::endl;

}

void test_function4(){
    Solana_web3::Pubkey test_publickey = Solana_web3::Pubkey("8YXaA8pzJ4xVPjYY8b5HkxmPWixwpZu7gVcj8EvHxRDC");
    string a = test_publickey.get_pubkey_ipfs();
}



int main(){ 
    test_function4();

    return 0;
}

