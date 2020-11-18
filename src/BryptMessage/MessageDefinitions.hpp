//------------------------------------------------------------------------------------------------
// File: MessageDefinitions.hpp
// Description:
//------------------------------------------------------------------------------------------------
#pragma once
//------------------------------------------------------------------------------------------------
#include <cstdint>
//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
namespace Message {
//------------------------------------------------------------------------------------------------

enum class Protocol : std::uint8_t { Invalid = 0x00, Network = 0x01, Application = 0x10 };
enum class Destination : std::uint8_t { Invalid = 0x00, Node = 0x01 , Cluster = 0x02, Network = 0x03};

constexpr std::uint8_t const MajorVersion = 0x00;
constexpr std::uint8_t const MinorVersion = 0x00;

enum class ValidationStatus : std::uint8_t { Success, Error };

//------------------------------------------------------------------------------------------------
} // Message namespace
//------------------------------------------------------------------------------------------------