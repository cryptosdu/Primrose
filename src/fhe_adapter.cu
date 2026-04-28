
#ifdef PRIMROSE_ENABLE_GPU

#include "fhe_adapter.h"
#include <memory>
#include <phantom.h>


using namespace fhe;

void fhe::check_cuda_error_last() {
    PHANTOM_CHECK_CUDA_LAST_NO_THROW();
}

void fhe::synchronizeCurrentStream() {
    cudaStreamSynchronize(cudaStreamPerThread);
}

class Plaintext::Impl : public PhantomPlaintext {};
Plaintext::Plaintext() : p_(std::make_unique<Plaintext::Impl>()) {}
Plaintext::~Plaintext() = default;

class Ciphertext::Impl : public PhantomCiphertext {};
Ciphertext::Ciphertext() : p_(std::make_unique<Ciphertext::Impl>()) {}
Ciphertext::Ciphertext(const Ciphertext &other) : p_(std::make_unique<Ciphertext::Impl>(*other.p_)) {}
Ciphertext::Ciphertext(Ciphertext &&other) noexcept : p_(std::make_unique<Ciphertext::Impl>(*other.p_)) {}
Ciphertext::~Ciphertext() = default;

void Ciphertext::save(std::ostream &stream) const {
    p_->save(stream);
}
void Ciphertext::load(const Context &, std::istream &stream) {
    p_->load(stream);
}

class EncryptionParameters::Impl : public phantom::EncryptionParameters {
public:
    explicit Impl(phantom::scheme_type type) : phantom::EncryptionParameters(type) {}
};

phantom::scheme_type convert_to_phantom_scheme(scheme_type type) {
    switch (type) {
        case scheme_type::none:
            return phantom::scheme_type::none;
        case scheme_type::bgv:
            return phantom::scheme_type::bgv;
        case scheme_type::bfv:
            return phantom::scheme_type::bfv;
        case scheme_type::ckks:
            return phantom::scheme_type::ckks;
        default:
            throw std::out_of_range("Invalid fhe::scheme_type value");
    }
}

EncryptionParameters::EncryptionParameters(scheme_type type) : p_(std::make_unique<Impl>(convert_to_phantom_scheme(type))) {}

EncryptionParameters::~EncryptionParameters() = default;

void EncryptionParameters::set_poly_modulus_degree(std::size_t poly_modulus_degree) {
    p_->set_poly_modulus_degree(poly_modulus_degree);
}

class Modulus::Impl : public phantom::arith::Modulus {
public:
    Impl(phantom::arith::Modulus &m) : phantom::arith::Modulus(m) {}
};
Modulus::Modulus() {}
Modulus::~Modulus() = default;

std::vector<Modulus> CoeffModulus::Create(std::size_t poly_modulus_degree, const std::vector<int> &bit_sizes) {
    auto t = phantom::arith::CoeffModulus::Create(poly_modulus_degree, bit_sizes);
    std::vector<Modulus> result(t.size());
    for (int i = 0; i < t.size(); i++) {
        result[i].p_ = std::make_unique<Modulus::Impl>(t[i]);
    }
    return result;
}

void EncryptionParameters::set_coeff_modulus(const std::vector<Modulus> &coeff_modulus) {
    std::vector<phantom::arith::Modulus> m;
    m.reserve(coeff_modulus.size());
    for (auto &modulus : coeff_modulus) {
        m.push_back(*modulus.p_);
    }
    p_->set_coeff_modulus(m);
}

class Context::Impl : public PhantomContext {
public:
    Impl(const EncryptionParameters &params) : PhantomContext(*params.p_) {}
};

Context::Context(const EncryptionParameters &params) : p_(std::make_unique<Impl>(params)) {}

Context::~Context() = default;

class SecretKey::Impl : public PhantomSecretKey {
public:
    explicit Impl(const Context &context) : PhantomSecretKey(*context.p_) {}
};

SecretKey::SecretKey(const Context &context) : p_(std::make_unique<Impl>(context)) {}

SecretKey::~SecretKey() = default;

class CKKSEncoder::Impl : public PhantomCKKSEncoder {
public:
    explicit Impl(const Context &context) : PhantomCKKSEncoder(*context.p_) {}
};

CKKSEncoder::CKKSEncoder(const Context &context) : p_(std::make_unique<Impl>(context)), context_(context) {}

fhe::CKKSEncoder::CKKSEncoder(CKKSEncoder &&other) noexcept : p_(std::move(other.p_)) ,context_(other.context_) {
}

CKKSEncoder::~CKKSEncoder() = default;

void CKKSEncoder::encode(const float value, double scale, Plaintext &destination) const {
    p_->encode(*context_.p_, value, scale, *destination.p_);
}

void CKKSEncoder::encode(const float *values, std::size_t values_size, double scale, Plaintext &destination) const {
    std::vector<cuDoubleComplex> input(values_size);
    for (size_t i = 0; i < values_size; i++) {
        input[i] = make_cuDoubleComplex(values[i], 0.0);
    }
    p_->encode(*context_.p_, input, scale, *destination.p_);
}

void CKKSEncoder::decode(const Plaintext &plain, std::vector<double> &destination) const {
    p_->decode(*context_.p_, *plain.p_, destination);
}

Encryptor::Encryptor(const Context &context, const SecretKey &secret_key) : context_(context), secret_key_(secret_key) {}

void Encryptor::encrypt_symmetric(const Plaintext &plain, Ciphertext &destination) const {
    secret_key_.p_->encrypt_symmetric(*context_.p_, *plain.p_, *destination.p_);
}


Decryptor::Decryptor(const Context &context, const SecretKey &secret_key) : context_(context), secret_key_(secret_key) {}

void Decryptor::decrypt(const Ciphertext &encrypted, Plaintext &destination) {
    secret_key_.p_->decrypt(*context_.p_, *encrypted.p_, *destination.p_);
}

KeyGenerator::KeyGenerator(const Context &context) : context_(context) {}

SecretKey KeyGenerator::secret_key() const {
    return SecretKey(context_);
}

Evaluator::Evaluator(const Context &context) : context_(context) {}

void Evaluator::multiply_plain(const Ciphertext &encrypted, const Plaintext &plain, Ciphertext &destination) const {
    phantom::multiply_plain(*context_.p_, *encrypted.p_, *plain.p_, *destination.p_);
}

void Evaluator::add_inplace(Ciphertext &encrypted1, const Ciphertext &encrypted2) const {
    phantom::add_inplace(*context_.p_, *encrypted1.p_, *encrypted2.p_);
}

void Evaluator::sub_plain(const Ciphertext &encrypted, const Plaintext &plain, Ciphertext &destination) const {
    phantom::sub_plain(*context_.p_, *encrypted.p_, *plain.p_, *destination.p_);
}

void Evaluator::square_inplace(Ciphertext &encrypted) const {
    phantom::multiply_inplace(*context_.p_, *encrypted.p_, *encrypted.p_);
}

#else

#endif
