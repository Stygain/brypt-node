//------------------------------------------------------------------------------------------------
// File: ConnectionState.hpp
// Description: Enum class for the connection states of peers
//------------------------------------------------------------------------------------------------
#pragma once
//------------------------------------------------------------------------------------------------
#include <cstdint>
//------------------------------------------------------------------------------------------------

enum class ConnectionState : std::uint8_t {
    Connected,
    Disconnected,
    Resolving,
    Unknown
};

//------------------------------------------------------------------------------------------------