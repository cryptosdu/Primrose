#pragma once

#ifdef PRIMROSE_ENABLE_GPU

#include <memory>
#include <vector>
#include <cstddef>
#include <cstdint>

namespace fhe {

    void check_cuda_error_last();

    void synchronizeCurrentStream();

    enum class scheme_type : std::uint8_t {
        none = 0,
        bgv = 1,
        bfv = 2,
        ckks = 3
    };

    class CKKSEncoder;

    class Plaintext {
    public:
        Plaintext();

        ~Plaintext();

    private:
        class Impl;

        std::unique_ptr<Impl> p_;

        friend class CKKSEncoder;
        friend class Encryptor;
        friend class Decryptor;
        friend class Evaluator;
    };

//    class Ciphertext {
//    public:
//        Ciphertext();
//        Ciphertext(const Ciphertext &);
//        Ciphertext(Ciphertext &&) noexcept ;
//        ~Ciphertext();
//
//        void save(std::ostream &stream) const;
//        void load(std::istream &stream);
//
//    private:
//        class Impl;
//
//        std::unique_ptr<Impl> p_;
//
//        friend class Encryptor;
//        friend class Decryptor;
//        friend class Evaluator;
//    };

    class Modulus {
    public:
        Modulus();
        ~Modulus();
    private:
        class Impl;
        std::unique_ptr<Impl> p_;
        friend class EncryptionParameters;
        friend class CoeffModulus;
    };

    class CoeffModulus {
    public:
        static std::vector<Modulus> Create(std::size_t poly_modulus_degree, const std::vector<int> &bit_sizes);
    };

    class EncryptionParameters {
    public:
        EncryptionParameters(scheme_type type);

        ~EncryptionParameters();

        void set_poly_modulus_degree(std::size_t poly_modulus_degree);

        void set_coeff_modulus(const std::vector<Modulus> &coeff_modulus);

    private:
        class Impl;

        std::unique_ptr<Impl> p_;

        friend class Context;
    };

    class SecretKey;

    class Context {
    public:
        explicit Context(const EncryptionParameters &);

        ~Context();

    private:
        class Impl;

        std::unique_ptr<Impl> p_;

        friend class CKKSEncoder;
        friend class SecretKey;
        friend class Encryptor;
        friend class Decryptor;
        friend class Evaluator;
    };

    class Ciphertext {
    public:
        Ciphertext();
        Ciphertext(const Ciphertext &);
        Ciphertext(Ciphertext &&) noexcept ;
        ~Ciphertext();

        void save(std::ostream &) const;
        void load(const Context &, std::istream &);

    private:
        class Impl;

        std::unique_ptr<Impl> p_;

        friend class Encryptor;
        friend class Decryptor;
        friend class Evaluator;
    };

    class SecretKey {
    public:
        explicit SecretKey(const Context &);

        ~SecretKey();

    private:
        class Impl;

        std::unique_ptr<Impl> p_;

        friend class Encryptor;
        friend class Decryptor;
    };

    class CKKSEncoder {
    public:
        explicit CKKSEncoder(const Context &);

//        CKKSEncoder(CKKSEncoder &&) noexcept = default;
        CKKSEncoder(CKKSEncoder &&) noexcept;

        ~CKKSEncoder();

        void encode(float value, double scale, Plaintext &destination) const;

        void encode(const float *values, std::size_t values_size, double scale, Plaintext &destination) const;

        void decode(const Plaintext &plain, std::vector<double> &destination) const;

    private:

        class Impl;

        std::unique_ptr<Impl> p_;
        const Context &context_;
    };

    class Encryptor {
    public:
        Encryptor(const Context &, const SecretKey &);

        ~Encryptor() = default;

        void encrypt_symmetric(const Plaintext &plain, Ciphertext &destination) const;
    private:
        const Context &context_;
        const SecretKey &secret_key_;
    };

    class Decryptor {
    public:
        Decryptor(const Context &, const SecretKey &);

        ~Decryptor() = default;

        void decrypt(const Ciphertext &encrypted, Plaintext &destination);
    private:
        const Context &context_;
        const SecretKey &secret_key_;
    };

    class KeyGenerator {
    public:
        explicit KeyGenerator(const Context &);

        ~KeyGenerator() = default;

        [[nodiscard]]
        SecretKey secret_key() const;

    private:
        const Context &context_;
    };

    class Evaluator {
    public:
        explicit Evaluator(const Context &);

        ~Evaluator() = default;

        void multiply_plain(const Ciphertext &encrypted, const Plaintext &plain, Ciphertext &destination) const;

        void add_inplace(Ciphertext &encrypted1, const Ciphertext &encrypted2) const;

        void sub_plain(const Ciphertext &encrypted, const Plaintext &plain, Ciphertext &destination) const;

        void square_inplace(Ciphertext &encrypted) const;

    private:
        const Context &context_;
    };

}
#else

#include <seal/seal.h>

namespace fhe {

    using Plaintext = seal::Plaintext;
    using Ciphertext = seal::Ciphertext;
    using SecretKey = seal::SecretKey;
    using Context = seal::SEALContext;
    using CKKSEncoder = seal::CKKSEncoder;
    using Encryptor = seal::Encryptor;
    using Decryptor = seal::Decryptor;
    using Evaluator = seal::Evaluator;
    using KeyGenerator = seal::KeyGenerator;
    using EncryptionParameters = seal::EncryptionParameters;
    using scheme_type = seal::scheme_type;
    using CoeffModulus = seal::CoeffModulus;

}
#endif

