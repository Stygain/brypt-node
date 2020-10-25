//------------------------------------------------------------------------------------------------
// File: SecurityDefinitions.hpp
// Description: 
//------------------------------------------------------------------------------------------------
#pragma once
//------------------------------------------------------------------------------------------------
#include <cstdint>
//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
namespace Security {
//------------------------------------------------------------------------------------------------

enum class Strategy : std::uint16_t { Invalid = 0x0000, PQNISTL3 = 0x1003 };

enum class Context : std::uint8_t { Unique, Application };

enum class Role : std::uint8_t { Initiator, Acceptor };

enum class SynchronizationStatus : std::uint8_t { Error, Processing, Ready };

enum class VerificationStatus : std::uint8_t { Failed, Success };

//------------------------------------------------------------------------------------------------
} // Security namespace
//------------------------------------------------------------------------------------------------
