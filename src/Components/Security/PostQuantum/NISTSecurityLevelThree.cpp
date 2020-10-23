//------------------------------------------------------------------------------------------------
// File: NISTSecurityLevelThree.cpp
// Description: 
//------------------------------------------------------------------------------------------------
#include "NISTSecurityLevelThree.hpp"
#include "../ConnectDefinitions.hpp"
#include "../../BryptMessage/PackUtils.hpp"
#include "../../Utilities/TimeUtils.hpp"
//------------------------------------------------------------------------------------------------
#include <openssl/conf.h>
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
//------------------------------------------------------------------------------------------------
#include <algorithm>
#include <array>
#include <cstring>
#include <memory>
#include <mutex>
#include <optional>
//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
namespace {
namespace local {
//------------------------------------------------------------------------------------------------

// Various size constants required for AES-256-CTR 
constexpr std::uint32_t EncryptionKeySize = 32; // In bytes, 256 bits. 
constexpr std::uint32_t EncryptionIVSize = 16; // In bytes, 128 bits. 
constexpr std::uint32_t EncryptionBlockSize = 16; // In bytes, 128 bits. 

using CipherContext = std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)>;

Security::OptionalBuffer GenerateRandomData(std::uint32_t size);

Security::Buffer GenerateEncapsulationMessage(
    Security::Buffer const& encapsulation, Security::Buffer const& data);
bool ParseEncapsulationMessage(
    Security::Buffer const& buffer, Security::Buffer& encapsulation, Security::Buffer& data);

//------------------------------------------------------------------------------------------------
} // local namespace
} // namespace
//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
// Declare the static shared context the strategy may use in a shared application context. 
//------------------------------------------------------------------------------------------------

std::shared_ptr<Security::PQNISTL3::CContext> Security::PQNISTL3::CStrategy::m_spSharedContext = nullptr;

//------------------------------------------------------------------------------------------------

Security::PQNISTL3::CContext::CContext(std::string_view kem)
    : m_kemMutex()
    , m_kem(kem.data())
    , m_publicKeyMutex()
    , m_publicKey()
{
    try {
        m_publicKey = m_kem.generate_keypair();
    }
    catch (...) {
        throw std::runtime_error("Security Context failed to generate public/private key pair!");
    }
}

//------------------------------------------------------------------------------------------------

std::uint32_t Security::PQNISTL3::CContext::GetPublicKeySize() const
{
    std::shared_lock lock(m_kemMutex);
    auto const& details = m_kem.get_details();
    return details.length_public_key;
}

//------------------------------------------------------------------------------------------------

Security::Buffer Security::PQNISTL3::CContext::GetPublicKey() const
{
    return m_publicKey;
}

//------------------------------------------------------------------------------------------------

std::uint32_t Security::PQNISTL3::CContext::GetPublicKey(Buffer& buffer) const
{
    std::shared_lock lock(m_publicKeyMutex);
    buffer.insert(buffer.end(), m_publicKey.begin(), m_publicKey.end());
    return m_publicKey.size();
}

//------------------------------------------------------------------------------------------------

bool Security::PQNISTL3::CContext::GenerateEncapsulatedSecret(
    Buffer const& publicKey, EncapsulationCallback const& callback) const
{
    std::shared_lock lock(m_kemMutex);
    try {
        auto [encaped, secret] = m_kem.encap_secret(publicKey);
        callback(std::move(encaped), std::move(secret));
        return true;
    }
    catch(...) {
        return false;
    }
}

//------------------------------------------------------------------------------------------------

bool Security::PQNISTL3::CContext::DecapsulateSecret(
    Buffer const& encapsulation, Buffer& decapsulation) const
{
    // Try to generate and decapsulate the shared secret. If the OQS method throws an error,
    // signal that the operation did not succeed. 
    try {
        std::shared_lock lock(m_kemMutex);
        decapsulation = std::move(m_kem.decap_secret(encapsulation));
        return true;
    }
    catch(...) {
        return false;
    }
}

//------------------------------------------------------------------------------------------------

Security::PQNISTL3::CSynchronizationTracker::CSynchronizationTracker()
    : m_status()
    , m_stage()
    , m_upTransactionHasher(nullptr, &EVP_MD_CTX_free)
{
}

//------------------------------------------------------------------------------------------------

Security::SynchronizationStatus Security::PQNISTL3::CSynchronizationTracker::GetStatus() const
{
    return m_status;
}

//------------------------------------------------------------------------------------------------

void Security::PQNISTL3::CSynchronizationTracker::SetError()
{
    m_status = SynchronizationStatus::Error;
}

//------------------------------------------------------------------------------------------------

void Security::PQNISTL3::CSynchronizationTracker::AddTransactionData(
    [[maybe_unused]] Buffer const& buffer)
{
}

//------------------------------------------------------------------------------------------------

Security::VerificationStatus Security::PQNISTL3::CSynchronizationTracker::VerifyTransaction(
    [[maybe_unused]] Buffer const& buffer)
{
    return VerificationStatus::Failed;
}

//------------------------------------------------------------------------------------------------

void Security::PQNISTL3::CSynchronizationTracker::ResetState()
{
    m_status = SynchronizationStatus::Processing;
    m_stage = 0;
    m_upTransactionHasher.reset();
}

//------------------------------------------------------------------------------------------------

template <typename EnumType>
EnumType Security::PQNISTL3::CSynchronizationTracker::GetStage() const
{
    return static_cast<EnumType>(m_stage);
}

//------------------------------------------------------------------------------------------------

template <typename EnumType>
void Security::PQNISTL3::CSynchronizationTracker::SetStage(EnumType type)
{
    using UnderlyingType = typename std::underlying_type_t<EnumType>;
    assert(sizeof(UnderlyingType) == sizeof(m_stage));
    m_stage = static_cast<decltype(m_stage)>(type);
}

//------------------------------------------------------------------------------------------------

template<typename EnumType>
void Security::PQNISTL3::CSynchronizationTracker::FinalizeTransaction(EnumType type)
{
    m_status = SynchronizationStatus::Ready;
    SetStage(type);
}

//------------------------------------------------------------------------------------------------

Security::PQNISTL3::CStrategy::CStrategy(Role role, Context context)
    : m_role(role)
    , m_synchronization()
    , m_spSessionContext()
    , m_kem(KeyEncapsulationSchme.data())
    , m_store()
{
    switch (context) {
        case Context::Unique: {
            m_spSessionContext = std::make_shared<CContext>(KeyEncapsulationSchme);
        } break;
        case Context::Application: {
            if (!m_spSharedContext) {
                throw std::runtime_error("Shared Application Context has not been initialized!");
            }
            m_spSessionContext = m_spSharedContext;
        } break;
        default: assert(false); break;
    }
}

//------------------------------------------------------------------------------------------------

Security::Strategy Security::PQNISTL3::CStrategy::GetStrategyType() const
{
    return Type;
}

//------------------------------------------------------------------------------------------------

Security::Role Security::PQNISTL3::CStrategy::GetRole() const
{
    return m_role;
}

//------------------------------------------------------------------------------------------------

std::uint32_t Security::PQNISTL3::CStrategy::SynchronizationStages() const
{
    switch (m_role) {
        case Security::Role::Initiator: return InitiatorSynchronizationStages;
        case Security::Role::Acceptor: return AcceptorSynchronizationStages;
        default: assert(false); return 0;
    }
}

//------------------------------------------------------------------------------------------------

Security::SynchronizationStatus Security::PQNISTL3::CStrategy::GetSynchronizationStatus() const
{
    return m_synchronization.GetStatus();
}

//------------------------------------------------------------------------------------------------

Security::SynchronizationResult Security::PQNISTL3::CStrategy::Synchronize(Buffer const& buffer)
{
    switch (m_role) {
        case Role::Initiator: return HandleInitiatorSynchronization(buffer);
        case Role::Acceptor: return HandleAcceptorSynchronization(buffer);
        default: assert(false); break; // What is this?
    }
}

//------------------------------------------------------------------------------------------------

Security::OptionalBuffer Security::PQNISTL3::CStrategy::Encrypt(
    Buffer const& buffer, std::uint32_t size, std::uint64_t nonce) const
{
    // Ensure the caller is able to encrypt the buffer with generated session keys. 
    if (!m_store.HasGeneratedKeys()) {
        throw std::runtime_error("Security Strategy cannot encrypt before synchronization is complete!");
    }

    // If the buffer contains no data or is less than the specified data to encrypt there is nothing
    // that can be done. 
    if (buffer.size() == 0 || buffer.size() < size) {
		return {};
	}

    // Create an OpenSSL encryption context. 
	local::CipherContext upCipherContext(EVP_CIPHER_CTX_new(), &EVP_CIPHER_CTX_free);
	if (ERR_get_error() != 0 || upCipherContext == nullptr) {
		return {};
	}

    // Get our content encryption key to be used in the cipher. 
    auto const optEncryptionKey = m_store.GetContentKey();
    if (!optEncryptionKey) {
		return {};
    }

    // Destructure the KeyStore result into meaniful names. 
    auto const& [pKey, keySize] = *optEncryptionKey;
    assert(keySize == local::EncryptionKeySize);

    // Setup the AES-256-CTR initalization vector from the given nonce. 
	std::array<std::uint8_t, local::EncryptionIVSize> iv = {};
	std::memcpy(iv.data(), &nonce, sizeof(nonce));

    // Initialize the OpenSSL cipher using AES-256-CTR with the encryption key and IV. 
	EVP_EncryptInit_ex(upCipherContext.get(), EVP_aes_256_ctr(), nullptr, pKey, iv.data());
	if (ERR_get_error() != 0) {
		return {};
	}

    // Sanity check that our encryption key and IV are the size expected by OpenSSL. 
    assert(std::int32_t(keySize) == EVP_CIPHER_CTX_key_length(upCipherContext.get()));
    assert(std::int32_t(iv.size()) == EVP_CIPHER_CTX_iv_length(upCipherContext.get()));

	Buffer ciphertext(size, 0x00); // Create a buffer to store the encrypted data. 
	auto pCiphertext = reinterpret_cast<std::uint8_t*>(ciphertext.data());
	auto const pPlaintext = reinterpret_cast<std::uint8_t const*>(buffer.data());

    // Encrypt the plaintext into the ciphertext buffer. 
	std::int32_t encrypted = 0;
	EVP_EncryptUpdate(upCipherContext.get(), pCiphertext, &encrypted, pPlaintext, size);
	if (ERR_get_error() != 0 || encrypted == 0) {
		return {};
	}

    // Cleanup the OpenSSL encryption cipher. 
	EVP_EncryptFinal_ex(upCipherContext.get(), pCiphertext + encrypted, &encrypted);
	if (ERR_get_error() != 0) {
		return {};
	}

	return ciphertext;
}

//------------------------------------------------------------------------------------------------

Security::OptionalBuffer Security::PQNISTL3::CStrategy::Decrypt(
    Buffer const& buffer, std::uint32_t size, std::uint64_t nonce) const
{
    // Ensure the caller is able to decrypt the buffer with generated session keys.
    if (!m_store.HasGeneratedKeys()) {
        throw std::runtime_error("Security Strategy cannot decrypt before synchronization is complete!");
    }

    // If the buffer contains no data or is less than the specified data to decrypt there is nothing
    // that can be done.
	if (size == 0) {
		return {};
	}

    // Create an OpenSSL decryption context.
	local::CipherContext upCipherContext(EVP_CIPHER_CTX_new(), &EVP_CIPHER_CTX_free);
	if (ERR_get_error() != 0 || upCipherContext == nullptr) {
		return {};
	}

    // Get the peer's content decryption key to be used in the cipher.
    auto const optDecryptionKey = m_store.GetPeerContentKey();
    if (!optDecryptionKey) {
		return {};
    }

    // Destructure the KeyStore result into meaniful names.
    auto const& [pKey, keySize] = *optDecryptionKey;
    assert(keySize == local::EncryptionKeySize);

    // Setup the AES-256-CTR initalization vector from the given nonce.
	std::array<std::uint8_t, local::EncryptionIVSize> iv = {};
	std::memcpy(iv.data(), &nonce, sizeof(nonce));

    // Initialize the OpenSSL cipher using AES-256-CTR with the decryption key and IV.
	EVP_DecryptInit_ex(upCipherContext.get(), EVP_aes_256_ctr(), nullptr, pKey, iv.data());
	if (ERR_get_error() != 0) {
		return {};
	}

    // Sanity check that our decryption key and IV are the size expected by OpenSSL.
    assert(std::int32_t(keySize) == EVP_CIPHER_CTX_key_length(upCipherContext.get()));
    assert(std::int32_t(iv.size()) == EVP_CIPHER_CTX_iv_length(upCipherContext.get()));

	Buffer plaintext(size, 0x00); // Create a buffer to store the decrypted data. 
	auto pPlaintext = reinterpret_cast<std::uint8_t*>(plaintext.data());
	auto const pCiphertext = reinterpret_cast<std::uint8_t const*>(buffer.data());

    // Decrypt the ciphertext into the plaintext buffer.
	std::int32_t decrypted = 0;
	EVP_DecryptUpdate(upCipherContext.get(), pPlaintext, &decrypted, pCiphertext, size);
	if (ERR_get_error() != 0 || decrypted == 0) {
		return {};
	}

    // Cleanup the OpenSSL decryption cipher.
	EVP_DecryptFinal_ex(upCipherContext.get(), pPlaintext + decrypted ,&decrypted);
	if (ERR_get_error() != 0) {
		return {};
	}

	return plaintext;
}

//------------------------------------------------------------------------------------------------

std::uint32_t Security::PQNISTL3::CStrategy::Sign(Buffer& buffer) const
{
    // Ensure the caller is able to sign the buffer with generated session keys.
    if (!m_store.HasGeneratedKeys()) {
        throw std::runtime_error("Security Strategy cannot decrypt before synchronization is complete!");
    }
    
    // Get our signature key to be used when generating the content siganture. .
    auto const optSignatureKey = m_store.GetSignatureKey();
    if (!optSignatureKey) {
		return 0;
    }

    // Destructure the KeyStore result into meaniful names. 
    auto const [pKey, keySize] = *optSignatureKey;
    assert(keySize == SignatureLength);

    // Sign the provided buffer with our signature key .
    OptionalBuffer optSignature = GenerateSignature(pKey, keySize, buffer.data(), buffer.size());
    if (!optSignature) {
        return 0;
    }

    // Insert the signature to create a verifiable buffer. 
    buffer.insert(buffer.end(), optSignature->begin(), optSignature->end());
    return optSignature->size(); // Tell the caller how many bytes have been inserted. 
}

//------------------------------------------------------------------------------------------------

Security::VerificationStatus Security::PQNISTL3::CStrategy::Verify(Buffer const& buffer) const
{
    // Ensure the caller is able to verify the buffer with generated session keys.
    if (!m_store.HasGeneratedKeys()) {
        throw std::runtime_error("Security Strategy cannot decrypt before synchronization is complete!");
    }

    // Determine the amount of non-signature data is packed into the buffer. 
    std::int64_t packContentSize = buffer.size() - SignatureLength;
	if (buffer.empty() || packContentSize <= 0) {
		return VerificationStatus::Failed;
	}

    // Get the peer's signature key to be used to generate the expected siganture..
    auto const optPeerSignatureKey = m_store.GetPeerSignatureKey();
    if (!optPeerSignatureKey) {
		return VerificationStatus::Failed;
    }

    // Destructure the KeyStore result into meaniful names. 
    auto const [pKey, keySize] = *optPeerSignatureKey;
    assert(keySize == SignatureLength);

    // Create the signature that peer should have provided. 
	auto const optGeneratedBuffer = GenerateSignature(pKey, keySize, buffer.data(), packContentSize);
	if (!optGeneratedBuffer) {
		return VerificationStatus::Failed;
	}

    // Compare the generated signature with the signature attached to the buffer. 
	auto const result = CRYPTO_memcmp(
		optGeneratedBuffer->data(), buffer.data() + packContentSize, optGeneratedBuffer->size());

    // If the signatures are not equal than the peer did not sign the buffer or the buffer was
    // altered in transmission. 
	if (result != 0) {
		return VerificationStatus::Failed;
	}

	return VerificationStatus::Success;
}

//------------------------------------------------------------------------------------------------

void Security::PQNISTL3::CStrategy::InitializeApplicationContext()
{
    Security::PQNISTL3::CStrategy::m_spSharedContext = std::make_shared<CContext>(
        KeyEncapsulationSchme);
}

//------------------------------------------------------------------------------------------------

std::weak_ptr<Security::PQNISTL3::CContext> Security::PQNISTL3::CStrategy::GetSessionContext() const
{
    return m_spSessionContext;
}

//------------------------------------------------------------------------------------------------

std::uint32_t Security::PQNISTL3::CStrategy::GetPublicKeySize() const
{
    return m_spSessionContext->GetPublicKeySize();
}

//------------------------------------------------------------------------------------------------

Security::OptionalBuffer Security::PQNISTL3::CStrategy::GenerateSignature(
    std::uint8_t const* pKey,
    std::uint32_t keySize,
    std::uint8_t const* pData,
    std::uint32_t dataSize) const
{
    // If there is no data to be signed, there is nothing to do. 
	if (dataSize == 0) {
		return {};
	}

    // Hash the provided buffer with the provided key to generate the signature. 
	std::uint32_t hashed = 0;
	auto const signature = HMAC(EVP_sha384(), pKey, keySize, pData, dataSize, nullptr, &hashed);
	if (ERR_get_error() != 0 || hashed == 0) {
		return {};
	}

    // Provide the caller a buffer created from the c-style signature. 
    return Buffer(&signature[0], &signature[hashed]);
}

//------------------------------------------------------------------------------------------------

Security::SynchronizationResult Security::PQNISTL3::CStrategy::HandleInitiatorSynchronization(
    Buffer const& buffer)
{
    //         Stage       |            Input            |           Output            |
    //     Initialization  | PublicKey + PrincipalRandom | PublicKey + PrincipalRandom |
    //     Decapsulation   | Encapsulation + SyncMessage |         SyncMessage         |
    switch (m_synchronization.GetStage<InitiatorSynchronizationStage>())
    {
        case InitiatorSynchronizationStage::Initialization: {
            return HandleInitiatorInitialization(buffer);
        }
        case InitiatorSynchronizationStage::Decapsulation: {
            return HandleInitiatorDecapsulation(buffer);
        }
        // It is an error to be called in all other synchrinization stages. 
        case InitiatorSynchronizationStage::Complete:
        case InitiatorSynchronizationStage::Invalid: {
            m_synchronization.SetError();
            return { m_synchronization.GetStatus(), {} };
        }
    }

    return {};
}

//------------------------------------------------------------------------------------------------

Security::SynchronizationResult Security::PQNISTL3::CStrategy::HandleInitiatorInitialization(
    Buffer const& buffer)
{
    // Handle processing the synchronization buffer.
    {
        auto const& details = m_kem.get_details();
        
        // If the provided buffer does not contain the 
        if (std::uint32_t const expected = (details.length_public_key + PrincipalRandomLength);
            buffer.size() != expected) {
            m_synchronization.SetError();
            return { m_synchronization.GetStatus(), {} };
        }

        m_store.SetPeerPublicKey({ buffer.begin(), buffer.begin() + details.length_public_key });
        m_store.ExpandSessionSeed({ buffer.end() - PrincipalRandomLength, buffer.end() });
    }

    auto const optPrincipalSeed = local::GenerateRandomData(PrincipalRandomLength);
    if (!optPrincipalSeed) {
        m_synchronization.SetError();
        return { m_synchronization.GetStatus(), {} };
    }

    m_store.ExpandSessionSeed(*optPrincipalSeed);

    // The session context should always exist after the constructor is called. 
    assert(m_spSessionContext);

    Buffer response;
    response.reserve(m_spSessionContext->GetPublicKeySize() + optPrincipalSeed->size());

    if (std::uint32_t fetched = m_spSessionContext->GetPublicKey(response); fetched == 0) {
        m_synchronization.SetError();
        return { m_synchronization.GetStatus(), {} };
    }

    response.insert(response.end(), optPrincipalSeed->begin(), optPrincipalSeed->end());

    // We expect to be provided the peer's shared secret encapsulation in the next 
    // synchronization stage. 
    m_synchronization.SetStage(InitiatorSynchronizationStage::Decapsulation);

    return { m_synchronization.GetStatus(), response };
}

//------------------------------------------------------------------------------------------------

Security::SynchronizationResult Security::PQNISTL3::CStrategy::HandleInitiatorDecapsulation(
    Buffer const& buffer)
{
    // The caller should provide us with a synchronization message generated by an acceptor
    // strategy. The encaped shared secret and verification data needs to be unpacked from 
    // the buffer. If for some reason the buffer cannot be parsed, it is an error. 
    Buffer encapsulation;
    Buffer peerVerificationData;
    if (!local::ParseEncapsulationMessage(buffer, encapsulation, peerVerificationData)) {
        m_synchronization.SetError();
        return { m_synchronization.GetStatus(), {} };
    }

    // Attempt to decapsulate the shared secret. If the shared secret could not be decapsulated
    // or the session keys fail to be generated, it is an error. 
    if (!DecapsulateSharedSecret(encapsulation)) {
        m_synchronization.SetError();
        return { m_synchronization.GetStatus(), {} };
    }

    // Now that the shared secret has been decapsulated, we can verify that the message has been
    // provided by an honest peer. If the provided buffer cannot be verified, it is an error. 
    if (auto const status = Verify(buffer); status != VerificationStatus::Success) {
        m_synchronization.SetError();
        return { m_synchronization.GetStatus(), {} };
    }

    // Given the buffer has been verified we can decrypt the provided verification data to 
    // ensure our the session keys align with the peer. 
    auto const optDecryptedData = Decrypt(peerVerificationData, peerVerificationData.size(), 0);
    if (!optDecryptedData) {
        m_synchronization.SetError();
        return { m_synchronization.GetStatus(), {} };
    }

    // Get our own verification data to verify the peer's session keys and provide the peer some 
    // data that they can verify that we have generated the proper session keys. 
    auto const& optVerificationData = m_store.GetVerificationData();
    if (!optVerificationData) {
        m_synchronization.SetError();
        return { m_synchronization.GetStatus(), {} };
    }

    // Verify the provided verification data matches the verification data we have generated. 
    // If the data does not match, it is an error. 
    bool const bMatchedVerificationData = std::equal(
        optVerificationData->begin(), optVerificationData->end(), optDecryptedData->begin());
    if (!bMatchedVerificationData) {
        m_synchronization.SetError();
        return { m_synchronization.GetStatus(), {} };
    }

    // Encrypt our own verification data to challenge the peer's session keys. 
    OptionalBuffer optVerificationMessage = Encrypt(
        *optVerificationData, optVerificationData->size(), 0);

    // If for some reason we cannot sign the verification data it is an error. 
    if (Sign(*optVerificationMessage) == 0) {
        m_synchronization.SetError();
        return { m_synchronization.GetStatus(), {} };
    }

    // The synchronization process is now complete. 
    m_synchronization.FinalizeTransaction(InitiatorSynchronizationStage::Complete);

    return {m_synchronization.GetStatus(), *optVerificationMessage };
}

//------------------------------------------------------------------------------------------------

Security::SynchronizationResult Security::PQNISTL3::CStrategy::HandleAcceptorSynchronization(
    Buffer const& buffer)
{
    //          Stage      |            Input            |           Output            |
    //     Initialization  |            Hello            | PublicKey + PrincipalRandom |
    //     Encapsulation   | PublicKey + PrincipalRandom | Encapsulation + SyncMessage |
    //      Verification   |          SyncMessage        |            Done             |
    switch (m_synchronization.GetStage<AcceptorSynchronizationStage>())
    {
        case AcceptorSynchronizationStage::Initialization: {
            return HandleAcceptorInitialization(buffer);
        }
        case AcceptorSynchronizationStage::Encapsulation: {
            return HandleAcceptorEncapsulation(buffer);
        }
        case AcceptorSynchronizationStage::Verification: {
            return HandleAcceptorVerification(buffer);
        }
        // It is an error to be called in all other synchrinization stages. 
        case AcceptorSynchronizationStage::Complete:
        case AcceptorSynchronizationStage::Invalid: {
            m_synchronization.SetError();
            return { m_synchronization.GetStatus(), {} };
        }
    }

    return {};
}

//------------------------------------------------------------------------------------------------

Security::SynchronizationResult Security::PQNISTL3::CStrategy::HandleAcceptorInitialization(
    Buffer const& buffer)
{
    // Verify that we have been provided a properly formatted acceptor initialization message. 
    bool const bMatchedVerificationMessage = std::equal(
        Security::InitializationMessage.begin(), Security::InitializationMessage.end(), buffer.begin());
    if (!bMatchedVerificationMessage) {
        m_synchronization.SetError();
        return { m_synchronization.GetStatus(), {} };
    }

    // Generate random data to be used to generate the session keys. 
    auto const optPrincipalSeed = local::GenerateRandomData(PrincipalRandomLength);
    if (!optPrincipalSeed) {
        m_synchronization.SetError();
        return { m_synchronization.GetStatus(), {} };
    }

    // Add the prinicpal random data to the store in order to use it when generating session keys. 
    m_store.ExpandSessionSeed(*optPrincipalSeed);

    // The session context should always exist after the constructor is called. 
    assert(m_spSessionContext);

    // Make an initialization message for the acceptor containing our public key and principal 
    // random seed. 
    Buffer response;
    response.reserve(m_spSessionContext->GetPublicKeySize() + optPrincipalSeed->size());

    if (std::uint32_t fetched = m_spSessionContext->GetPublicKey(response); fetched == 0) {
        m_synchronization.SetError();
        return { m_synchronization.GetStatus(), {} };
    }

    response.insert(response.end(), optPrincipalSeed->begin(), optPrincipalSeed->end());

    // We expect to be provided the peer's public key and principal random seed in the next stage.
    m_synchronization.SetStage(AcceptorSynchronizationStage::Encapsulation);

    return { m_synchronization.GetStatus(), response };
}

//------------------------------------------------------------------------------------------------

Security::SynchronizationResult Security::PQNISTL3::CStrategy::HandleAcceptorEncapsulation(
    Buffer const& buffer)
{
    // Process the synchronization message. 
    {
        auto const& details = m_kem.get_details();
        
        // If the provided buffer size is not exactly the size of message we expect, we can not
        // proceed with the synchronization process. 
        if (std::uint32_t const expected = (details.length_public_key + PrincipalRandomLength);
            buffer.size() != expected) {
            m_synchronization.SetError();
            return { m_synchronization.GetStatus(), {} };
        }

        // The first section of the buffer is expected to be the peer's public key. 
        m_store.SetPeerPublicKey({ buffer.begin(), buffer.begin() + details.length_public_key });

        // The second section of the buffer is expected to be random data to be used as part of 
        // session key derivation process. 
        m_store.ExpandSessionSeed({ buffer.end() - PrincipalRandomLength, buffer.end() });
    }

    // Create an encapsulated shared secret using the peer's public key. If the process fails,
    // the synchronization failed and we cannot proceed. The encapsulated secret will packaged
    // along with verification data and a signature. 
    auto optEncapsulation = EncapsulateSharedSecret();
    if (!optEncapsulation) {
        m_synchronization.SetError();
        return {m_synchronization.GetStatus(), {} };
    }

    // Get the verification data to provide as part of the message, such that the peer can verify
    // the decapsulation was successful. 
    auto const& optVerificationData = m_store.GetVerificationData();
    if (!optVerificationData) {
        m_synchronization.SetError();
        return { m_synchronization.GetStatus(), {} };
    }

    // Encrypt verification data, to both challenge the peer and so an honest peer can verify that
    // the verification data matches their own generated session keys. 
    Security::OptionalBuffer optVerificationMessage = Encrypt(
        *optVerificationData, optVerificationData->size(), 0);
    if (!optVerificationMessage) {
        m_synchronization.SetError();
        return { m_synchronization.GetStatus(), {} };
    }

    // Pack the encapsulation and verification message into a buffer the initator can parse. 
    Security::Buffer message = local::GenerateEncapsulationMessage(
        *optEncapsulation, *optVerificationMessage);

    // Sign the encapsulated secret and verification data, such that the peer can verify the message
    // after decapsulation has occured. If the message for some reason could not be signed, there 
    // is nothing else to do. 
    if(Sign(message) == 0) {
        m_synchronization.SetError();
        return { m_synchronization.GetStatus(), {} };
    }

    // We expect to be provided the peer's verification data in the next stage.
    m_synchronization.SetStage(AcceptorSynchronizationStage::Verification);

    return { m_synchronization.GetStatus(), message };
}

//------------------------------------------------------------------------------------------------

Security::SynchronizationResult Security::PQNISTL3::CStrategy::HandleAcceptorVerification(
    Buffer const& buffer)
{
    // In the acceptor's verification stage we expect to have been provided the initator's 
    // signed verfiection data. If the buffer could not be verified, it is an error. 
    if (auto const status = Verify(buffer); status != Security::VerificationStatus::Success) {
        m_synchronization.SetError();
        return { m_synchronization.GetStatus(), {} };
    }

    // Get our own verification data to verify the peer's verification data aligns with our
    // own. 
    auto const& optVerificationData = m_store.GetVerificationData();
    if (!optVerificationData) {
        m_synchronization.SetError();
        return { m_synchronization.GetStatus(), {} };
    }

    // Given the buffer has been verified we can decrypt the provided verification data to 
    // ensure our the session keys align with the peer. 
    auto const optDecryptedData = Decrypt(buffer, optVerificationData->size(), 0);
    if (!optDecryptedData) {
        m_synchronization.SetError();
        return { m_synchronization.GetStatus(), {} };
    }

    // Verify the provided verification data matches the verification data we have generated. 
    // If the data does not match, it is an error. 
    bool const bMatchedVerificationData = std::equal(
        optVerificationData->begin(), optVerificationData->end(), optDecryptedData->begin());
    if (!bMatchedVerificationData) {
        m_synchronization.SetError();
        return { m_synchronization.GetStatus(), {} };
    }

    // The synchronization process is now complete.
    m_synchronization.FinalizeTransaction(AcceptorSynchronizationStage::Complete);

    return { m_synchronization.GetStatus(), {} };
}

//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
// Description: Generate and encapsulates an ephemeral session key using NTRU-HPS-2048-677. The
// caller is provided the encapsulated shared secret to provide the peer. If an error is 
// encountered nullopt is provided instead. 
//------------------------------------------------------------------------------------------------
Security::OptionalBuffer Security::PQNISTL3::CStrategy::EncapsulateSharedSecret()
{
    // A shared secret cannot be generated and encapsulated without the peer's public key. 
    auto const& optPeerPublicKey = m_store.GetPeerPublicKey();
    if (!optPeerPublicKey) {
        return {};
    }

    // The session context should always exist after the constructor is called. 
    assert(m_spSessionContext);

    Security::Buffer encapsulation;
    // Use the session context to generate a secret for using the peer's public key. 
    bool const success = m_spSessionContext->GenerateEncapsulatedSecret(
        *optPeerPublicKey,
        [this, &encapsulation] (Buffer&& encaped, Buffer&& secret)
        {
            m_store.GenerateSessionKeys(
                m_role, std::move(secret), local::EncryptionKeySize, SignatureLength);

            encapsulation = std::move(encaped);
        });

    if (!success) {
        return {};
    }

    return encapsulation;
}

//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
// Description: Decapsulates an ephemeral session key using NTRU-HPS-2048-677 from the provided
// encapsulated ciphertext. 
//------------------------------------------------------------------------------------------------
bool Security::PQNISTL3::CStrategy::DecapsulateSharedSecret(Buffer const& encapsulation)
{
    // The session context should always exist after the constructor is called. 
    assert(m_spSessionContext);

    Buffer decapsulation;
    if (!m_spSessionContext->DecapsulateSecret(encapsulation, decapsulation)) {
        return false;
    }
    
    return m_store.GenerateSessionKeys(
        m_role, std::move(decapsulation), local::EncryptionKeySize, SignatureLength);
}

//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
// Description: Generate and return a buffer of the provided size filled with random data. 
//------------------------------------------------------------------------------------------------
Security::OptionalBuffer local::GenerateRandomData(std::uint32_t size)
{
    Security::Buffer buffer = std::vector<std::uint8_t>(size, 0x00);
    if (!RAND_bytes(buffer.data(), size) || ERR_get_error() != 0) {
        return {};
    }
    return buffer;
}

//------------------------------------------------------------------------------------------------

Security::Buffer local::GenerateEncapsulationMessage(
    Security::Buffer const& encapsulation, Security::Buffer const& data)
{
    std::uint32_t const size = 
        sizeof(std::uint32_t) + encapsulation.size() + sizeof(std::uint32_t) + data.size();

    Security::Buffer buffer; 
    buffer.reserve(size);

	PackUtils::PackChunk(buffer, encapsulation, sizeof(std::uint32_t));
	PackUtils::PackChunk(buffer, data, sizeof(std::uint32_t));

    return buffer;
}

//------------------------------------------------------------------------------------------------

bool local::ParseEncapsulationMessage(
    Security::Buffer const& buffer, Security::Buffer& encapsulation, Security::Buffer& data)
{
	Security::Buffer::const_iterator begin = buffer.begin();
	Security::Buffer::const_iterator end = buffer.end();

    std::uint32_t encapsulationSize = 0; 
    if (!PackUtils::UnpackChunk(begin, end, encapsulationSize)) {
        return false;
    }

    if (!PackUtils::UnpackChunk(begin, end, encapsulation, encapsulationSize)) {
        return false;
    }

    std::uint32_t dataSize = 0; 
    if (!PackUtils::UnpackChunk(begin, end, dataSize)) {
        return false;
    }

    if (!PackUtils::UnpackChunk(begin, end, data, dataSize)) {
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------------------------
