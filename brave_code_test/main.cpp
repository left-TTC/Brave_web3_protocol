
#include <iostream>
#include "brave_web3_service.h"

using namespace std;

int main(){ 
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

    return 0;
}