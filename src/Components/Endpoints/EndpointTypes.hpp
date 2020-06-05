//------------------------------------------------------------------------------------------------
// File: EndpointTypes.hpp
// Description: Defines types that users of the CEndpoint may use
//------------------------------------------------------------------------------------------------
#pragma once
//------------------------------------------------------------------------------------------------
#include "TechnologyType.hpp"
//------------------------------------------------------------------------------------------------
#include <memory>
#include <unordered_map>
//------------------------------------------------------------------------------------------------

class CEndpoint;

//------------------------------------------------------------------------------------------------
namespace Endpoints {
//------------------------------------------------------------------------------------------------

enum class OperationType : std::uint8_t { Server, Client, Invalid };

//------------------------------------------------------------------------------------------------
} // Endpoint namespace
//------------------------------------------------------------------------------------------------