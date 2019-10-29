//------------------------------------------------------------------------------------------------
// File: Message.hpp
// Description:
//------------------------------------------------------------------------------------------------
#pragma once
//------------------------------------------------------------------------------------------------
#include <cstdint>
#include <vector>
//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
namespace Message {
//------------------------------------------------------------------------------------------------
using Buffer = std::vector<std::uint8_t>;
using Token = std::vector<std::uint8_t>;

enum class VerificationStatus : std::uint8_t { SUCCESS, UNAUTHORIZED };
//------------------------------------------------------------------------------------------------
} // Message namespace
//------------------------------------------------------------------------------------------------