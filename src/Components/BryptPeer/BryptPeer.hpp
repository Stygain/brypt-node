//------------------------------------------------------------------------------------------------
// File: BryptPeer.hpp
// Description: 
//------------------------------------------------------------------------------------------------
#pragma once
//------------------------------------------------------------------------------------------------
#include "EndpointRegistration.hpp"
#include "../Endpoints/EndpointIdentifier.hpp"
#include "../Endpoints/MessageScheduler.hpp"
#include "../Endpoints/TechnologyType.hpp"
#include "../../BryptIdentifier/IdentifierTypes.hpp"
#include "../../Interfaces/MessageSink.hpp"
#include "../../BryptMessage/MessageTypes.hpp"
//------------------------------------------------------------------------------------------------
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
//------------------------------------------------------------------------------------------------

class CMessageContext;
class IPeerMediator;

//------------------------------------------------------------------------------------------------

class CBryptPeer : public std::enable_shared_from_this<CBryptPeer>
{
public:
    explicit CBryptPeer(
        BryptIdentifier::CContainer const& identifier,
        IPeerMediator* const pPeerMediator = nullptr);

    BryptIdentifier::SharedContainer GetBryptIdentifier() const;
    BryptIdentifier::Internal::Type GetInternalIdentifier() const;
    std::string GetLocation() const;

    void SetLocation(std::string_view location);
    void SetReceiver(IMessageSink* const pMessageSink);

    bool IsActive() const;
    void RegisterEndpoint(CEndpointRegistration const& registration);
    void RegisterEndpoint(
        Endpoints::EndpointIdType identifier,
        Endpoints::TechnologyType technology,
        MessageScheduler const& scheduler = {},
        std::string_view uri = {});
    void WithdrawEndpoint(
        Endpoints::EndpointIdType identifier,
        Endpoints::TechnologyType technology);

    bool IsEndpointRegistered(Endpoints::EndpointIdType identifier) const;
    std::optional<std::string> GetRegisteredEntry(Endpoints::EndpointIdType identifier) const;
    std::uint32_t RegisteredEndpointCount() const;

    bool ScheduleSend(
        CMessageContext const& context,
        BryptIdentifier::CContainer const& destination,
        std::string_view const& message) const;

    bool ScheduleReceive(
        CMessageContext const& context,
        std::string_view const& buffer);
    bool ScheduleReceive(
        CMessageContext const& context,
        Message::Buffer const& buffer);

private:
    using RegisteredEndpoints = std::unordered_map<Endpoints::EndpointIdType, CEndpointRegistration>;

    IPeerMediator* const m_pPeerMediator;

    mutable std::recursive_mutex m_dataMutex;
    BryptIdentifier::SharedContainer m_spBryptIdentifier;
    std::string m_location;

    mutable std::recursive_mutex m_endpointsMutex;
    RegisteredEndpoints m_endpoints;

    mutable std::recursive_mutex m_receiverMutex;
    IMessageSink* m_pMessageSink;
};

//------------------------------------------------------------------------------------------------
