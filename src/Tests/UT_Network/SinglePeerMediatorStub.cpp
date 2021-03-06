//------------------------------------------------------------------------------------------------
// File: SinglePeerMediatorStub.cpp
// Description: 
//------------------------------------------------------------------------------------------------
#include "SinglePeerMediatorStub.hpp"
#include "BryptMessage/NetworkMessage.hpp"
#include "Components/BryptPeer/BryptPeer.hpp"
#include "Components/Security/SecurityMediator.hpp"
#include "Components/Security/SecurityDefinitions.hpp"
#include "Interfaces/SecurityStrategy.hpp"
//------------------------------------------------------------------------------------------------
#include <cassert>
//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
namespace {
namespace local {
//------------------------------------------------------------------------------------------------

class SecurityStrategyStub;

//------------------------------------------------------------------------------------------------
} // local namespace
} // namespace
//------------------------------------------------------------------------------------------------

class local::SecurityStrategyStub : public ISecurityStrategy
{
public:
    SecurityStrategyStub();

    virtual Security::Strategy GetStrategyType() const override;
    virtual Security::Role GetRoleType() const override;
    virtual Security::Context GetContextType() const override;
    virtual std::size_t GetSignatureSize() const override;

    virtual std::uint32_t GetSynchronizationStages() const override;
    virtual Security::SynchronizationStatus GetSynchronizationStatus() const override;
    virtual Security::SynchronizationResult PrepareSynchronization() override;
    virtual Security::SynchronizationResult Synchronize(Security::ReadableView) override;

    virtual Security::OptionalBuffer Encrypt(
        Security::ReadableView, std::uint64_t) const override;
    virtual Security::OptionalBuffer Decrypt(
        Security::ReadableView, std::uint64_t) const override;

    virtual std::int32_t Sign(Security::Buffer&) const override;
    virtual Security::VerificationStatus Verify(Security::ReadableView) const override;

private: 
    virtual std::int32_t Sign( Security::ReadableView, Security::Buffer&) const override;

    virtual Security::OptionalBuffer GenerateSignature(
        Security::ReadableView, Security::ReadableView) const override;
};

//------------------------------------------------------------------------------------------------

SinglePeerMediatorStub::SinglePeerMediatorStub(
    BryptIdentifier::SharedContainer const& spBryptIdentifier,
    IMessageSink* const pMessageSink)
    : m_spBryptIdentifier(spBryptIdentifier)
    , m_spBryptPeer()
    , m_pMessageSink(pMessageSink)
{
}

//------------------------------------------------------------------------------------------------

void SinglePeerMediatorStub::RegisterObserver([[maybe_unused]] IPeerObserver* const observer)
{
}

//------------------------------------------------------------------------------------------------

void SinglePeerMediatorStub::UnpublishObserver([[maybe_unused]] IPeerObserver* const observer)
{
}

//------------------------------------------------------------------------------------------------

SinglePeerMediatorStub::OptionalRequest SinglePeerMediatorStub::DeclareResolvingPeer(
    [[maybe_unused]] Network::RemoteAddress const& address,
    [[maybe_unused]] BryptIdentifier::SharedContainer const& spIdentifier)
{
    auto const optHeartbeatRequest = NetworkMessage::Builder()
        .MakeHeartbeatRequest()
        .SetSource(*m_spBryptIdentifier)
        .ValidatedBuild();
    assert(optHeartbeatRequest);

    return optHeartbeatRequest->GetPack();
}

//------------------------------------------------------------------------------------------------

void SinglePeerMediatorStub::UndeclareResolvingPeer(
    [[maybe_unused]] Network::RemoteAddress const& address)
{
}

//------------------------------------------------------------------------------------------------

std::shared_ptr<BryptPeer> SinglePeerMediatorStub::LinkPeer(
    BryptIdentifier::Container const& identifier,
    [[maybe_unused]] Network::RemoteAddress const& address)
{
    m_spBryptPeer = std::make_shared<BryptPeer>(identifier, this);

    auto upSecurityStrategy = std::make_unique<local::SecurityStrategyStub>();
    auto upSecurityMediator = std::make_unique<SecurityMediator>(
            m_spBryptIdentifier, std::move(upSecurityStrategy));

    m_spBryptPeer->AttachSecurityMediator(std::move(upSecurityMediator));

    m_spBryptPeer->SetReceiver(m_pMessageSink);

    return m_spBryptPeer;
}

//------------------------------------------------------------------------------------------------

void SinglePeerMediatorStub::DispatchPeerStateChange(
    [[maybe_unused]] std::weak_ptr<BryptPeer> const& wpBryptPeer,
    [[maybe_unused]] Network::Endpoint::Identifier identifier,
    [[maybe_unused]] Network::Protocol protocol,
    [[maybe_unused]] ConnectionState change)
{
}

//------------------------------------------------------------------------------------------------

std::shared_ptr<BryptPeer> SinglePeerMediatorStub::GetPeer() const
{
    return m_spBryptPeer;
}

//------------------------------------------------------------------------------------------------


local::SecurityStrategyStub::SecurityStrategyStub()
{
}

//------------------------------------------------------------------------------------------------

Security::Strategy local::SecurityStrategyStub::GetStrategyType() const
{
    return Security::Strategy::Invalid;
}

//------------------------------------------------------------------------------------------------

Security::Role local::SecurityStrategyStub::GetRoleType() const
{
    return Security::Role::Initiator;
}

//------------------------------------------------------------------------------------------------

Security::Context local::SecurityStrategyStub::GetContextType() const
{
    return Security::Context::Unique;
}

//------------------------------------------------------------------------------------------------

std::size_t local::SecurityStrategyStub::GetSignatureSize() const
{
    return 0;
}

//------------------------------------------------------------------------------------------------

std::uint32_t local::SecurityStrategyStub::GetSynchronizationStages() const
{
    return 0;
}

//------------------------------------------------------------------------------------------------

Security::SynchronizationStatus local::SecurityStrategyStub::GetSynchronizationStatus() const
{
    return Security::SynchronizationStatus::Processing;
}

//------------------------------------------------------------------------------------------------

Security::SynchronizationResult local::SecurityStrategyStub::PrepareSynchronization()
{
    return { Security::SynchronizationStatus::Processing, {} };
}

//------------------------------------------------------------------------------------------------

Security::SynchronizationResult local::SecurityStrategyStub::Synchronize(Security::ReadableView)
{
    return { Security::SynchronizationStatus::Processing, {} };
}

//------------------------------------------------------------------------------------------------

Security::OptionalBuffer local::SecurityStrategyStub::Encrypt(
    Security::ReadableView buffer, [[maybe_unused]] std::uint64_t) const
{
    return Security::Buffer(buffer.begin(), buffer.end());
}

//------------------------------------------------------------------------------------------------

Security::OptionalBuffer local::SecurityStrategyStub::Decrypt(
    Security::ReadableView buffer, [[maybe_unused]] std::uint64_t) const
{
    return Security::Buffer(buffer.begin(), buffer.end());
}


//------------------------------------------------------------------------------------------------

std::int32_t local::SecurityStrategyStub::Sign([[maybe_unused]] Security::Buffer&) const
{
    return 0;
}

//------------------------------------------------------------------------------------------------

Security::VerificationStatus local::SecurityStrategyStub::Verify(
    [[maybe_unused]] Security::ReadableView) const
{
    return Security::VerificationStatus::Success;
}

//------------------------------------------------------------------------------------------------

std::int32_t local::SecurityStrategyStub::Sign(
    [[maybe_unused]] Security::ReadableView, [[maybe_unused]] Security::Buffer&) const
{
    return 0;
}

//------------------------------------------------------------------------------------------------

Security::OptionalBuffer local::SecurityStrategyStub::GenerateSignature(
    [[maybe_unused]] Security::ReadableView, [[maybe_unused]] Security::ReadableView) const
{
    return {};
}

//------------------------------------------------------------------------------------------------
