//------------------------------------------------------------------------------------------------
#pragma once
//------------------------------------------------------------------------------------------------
#include "NodeUtils.hpp"
//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
enum class ReservedIdentifiers : NodeUtils::NodeIdType {
//------------------------------------------------------------------------------------------------

// Indicates that the destination identifier is unknown. The message should be treated as though
// it is intended for the receiving node. 
Unknown = 0x0,
// Indicates the message is a network-wide request and should be forwarded throughout the entire
// brypt network.
NetworkRequest = 0x1,
// Indicates the message is a cluster-wise request and should be forwarded throughout the current
// cluster. 
ClusterRequest = 0x2

//------------------------------------------------------------------------------------------------
}; // ReservedIdentifiers enum
//------------------------------------------------------------------------------------------------