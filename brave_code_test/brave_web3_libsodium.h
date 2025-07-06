

#include <array>
#include <cstdint>
#include <iostream>
#include <cstring>

#define crypto_hash_sha256_BYTES 32U
# define ACQUIRE_FENCE (void) 0

#define LOAD32_BE(SRC) load32_be(SRC);

#define ROTR32(X, B) rotr32((X), (B))

#define Ch(x, y, z) ((x & (y ^ z)) ^ z)
#define Maj(x, y, z) ((x & (y | z)) | (y & z))
#define SHR(x, n) (x >> n)
#define ROTR(x, n) ROTR32(x, n)
#define S0(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define S1(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define s0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ SHR(x, 3))
#define s1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ SHR(x, 10))

#define RND(a, b, c, d, e, f, g, h, k) \
    h += S1(e) + Ch(e, f, g) + k;      \
    d += h;                            \
    h += S0(a) + Maj(a, b, c);

#define RNDr(S, W, i, ii)                                                   \
    RND(S[(64 - i) % 8], S[(65 - i) % 8], S[(66 - i) % 8], S[(67 - i) % 8], \
        S[(68 - i) % 8], S[(69 - i) % 8], S[(70 - i) % 8], S[(71 - i) % 8], \
        W[i + ii] + Krnd[i + ii])

#define MSCH(W, ii, i) \
    W[i + ii + 16] =   \
        s1(W[i + ii + 14]) + W[i + ii + 9] + s0(W[i + ii + 1]) + W[i + ii]

namespace Web3_libsodium{

    int is_solana_PDA_valid(const unsigned char *p);

    int crypto_hash_sha256(
        unsigned char *out, const unsigned char *in,
        unsigned long long inlen
    );


    namespace Web3_ed25519{
        typedef int32_t fe25519[10];

        typedef struct {
            fe25519 X;
            fe25519 Y;
            fe25519 Z;
            fe25519 T;
        } ge25519_p3;

        typedef struct {
            uint32_t state[8];
            uint64_t count;
            uint8_t  buf[64];
        } crypto_hash_sha256_state;


        namespace Web3_ed25519_verify_PDA{
            //tool
            static inline uint64_t load_3(const unsigned char *in);
            static inline uint64_t load_4(const unsigned char *in);

            static void fe25519_reduce(fe25519 h, const fe25519 f);
            void fe25519_tobytes(unsigned char *s, const fe25519 h);

            //function
            void fe25519_frombytes(fe25519 h, const unsigned char *s);

            static inline void fe25519_1(fe25519 h);

            static void fe25519_sq(fe25519 h, const fe25519 f);

            static void fe25519_mul(fe25519 h, const fe25519 f, const fe25519 g);

            static void fe25519_sub(fe25519 h, const fe25519 f, const fe25519 g);

            static inline void fe25519_add(fe25519 h, const fe25519 f, const fe25519 g);

            static void fe25519_pow22523(fe25519 out, const fe25519 z);

            static inline int fe25519_iszero(const fe25519 f);

            static void fe25519_cmov(fe25519 f, const fe25519 g, unsigned int b);

            static inline void fe25519_neg(fe25519 h, const fe25519 f);

            static inline int fe25519_isnegative(const fe25519 f);

            int sodium_is_zero(const unsigned char *n, const size_t nlen);

            //interface
            int ge25519_frombytes_solana(ge25519_p3 *h, const unsigned char *s);

            int ge25519_is_on_curve(const ge25519_p3 *p);
        }

        
        namespace Web3_ed25519_sha256{

            static inline uint32_t load32_be(const uint8_t src[4]);

            static inline uint32_t
            rotr32(const uint32_t x, const int b);
        
            static void be32dec_vect(uint32_t *dst, const unsigned char *src, size_t len);

            void sodium_memzero(void * const pnt, const size_t len);

            static void SHA256_Pad(crypto_hash_sha256_state *state, uint32_t tmp32[64 + 8]);

            static void SHA256_Transform(uint32_t state[8], const uint8_t block[64], uint32_t W[64], uint32_t S[8]);

            static void be32enc_vect(unsigned char *dst, const uint32_t *src, size_t len);

            //interface
            int crypto_hash_sha256_init(crypto_hash_sha256_state *state);

            int crypto_hash_sha256_update(
                crypto_hash_sha256_state *state,
                const unsigned char *in, unsigned long long inlen
            );

            int crypto_hash_sha256_final(crypto_hash_sha256_state *state, unsigned char *out);

        }
        

        #define STORE64_BE(DST, W) store64_be((DST), (W))
        static inline void
        store64_be(uint8_t dst[8], uint64_t w)
        {
        #ifdef NATIVE_BIG_ENDIAN
            memcpy(dst, &w, sizeof w);
        #else
            dst[7] = (uint8_t) w; w >>= 8;
            dst[6] = (uint8_t) w; w >>= 8;
            dst[5] = (uint8_t) w; w >>= 8;
            dst[4] = (uint8_t) w; w >>= 8;
            dst[3] = (uint8_t) w; w >>= 8;
            dst[2] = (uint8_t) w; w >>= 8;
            dst[1] = (uint8_t) w; w >>= 8;
            dst[0] = (uint8_t) w;
        #endif
        }

        #define STORE32_BE(DST, W) store32_be((DST), (W))
        static inline void
        store32_be(uint8_t dst[4], uint32_t w)
        {
        #ifdef NATIVE_BIG_ENDIAN
            memcpy(dst, &w, sizeof w);
        #else
            dst[3] = (uint8_t) w; w >>= 8;
            dst[2] = (uint8_t) w; w >>= 8;
            dst[1] = (uint8_t) w; w >>= 8;
            dst[0] = (uint8_t) w;
        #endif
        }

        

        
        //------------------------------------------


        

    }
}