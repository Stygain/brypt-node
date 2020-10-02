//------------------------------------------------------------------------------------------------
// File: PeerManager.hpp
// Description:
//------------------------------------------------------------------------------------------------
#pragma once
//------------------------------------------------------------------------------------------------
#include "BryptPeer.hpp"
#include "../../BryptIdentifier/BryptIdentifier.hpp"
#include "../../Interfaces/PeerCache.hpp"
#include "../../Interfaces/PeerMediator.hpp"
#include "../../Interfaces/PeerObserver.hpp"
//------------------------------------------------------------------------------------------------
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/tag.hpp>
//------------------------------------------------------------------------------------------------
#include <shared_mutex>
//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
// Description:
//------------------------------------------------------------------------------------------------
class CPeerManager : public IPeerMediator, public IPeerCache
{
public:
    CPeerManager();

    // IPeerMediator {
    virtual void RegisterObserver(IPeerObserver* const observer) override;
    virtual void UnpublishObserver(IPeerObserver* const observer) override;

    virtual std::shared_ptr<CBryptPeer> LinkPeer(
        BryptIdentifier::CContainer const& identifier) override;

    virtual void DispatchPeerStateChange(
        std::weak_ptr<CBryptPeer> const& wpBryptPeer,
        Endpoints::EndpointIdType identifier,
        Endpoints::TechnologyType technology,
        ConnectionState change) override;
    // } IPeerMediator

    // IPeerCache {
    virtual bool ForEachCachedIdentifier(
      IdentifierReadFunction const& callback,
      Filter filter = Filter::Active) const override;
    virtual std::uint32_t ActivePeerCount() const override;
    virtual std::uint32_t InactivePeerCount() const override;
    virtual std::uint32_t ObservedPeerCount() const override;
    // } IPeerCache

private:
    using ObserverSet = std::set<IPeerObserver*>;

    using PeerTrackingMap = boost::multi_index_container<
        std::shared_ptr<CBryptPeer>,
        boost::multi_index::indexed_by<
            boost::multi_index::hashed_unique<
                boost::multi_index::const_mem_fun<
                    CBryptPeer,
                    BryptIdentifier::InternalType,
                    &CBryptPeer::GetInternalBryptIdentifier>>>>;

    std::uint32_t PeerCount(Filter filter) const;

    template<typename FunctionType, typename...Args>
    void NotifyObservers(FunctionType const& function, Args&&...args);

    template<typename FunctionType, typename...Args>
    void NotifyObserversConst(FunctionType const& function, Args&&...args) const;

    mutable std::shared_mutex m_peersMutex;
    PeerTrackingMap m_peers;

    mutable std::mutex m_observersMutex;
    ObserverSet m_observers;
    
};

//------------------------------------------------------------------------------------------------