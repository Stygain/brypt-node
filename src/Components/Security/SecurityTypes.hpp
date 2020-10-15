//------------------------------------------------------------------------------------------------
// File: SecurityTypes.hpp
// Description: 
//------------------------------------------------------------------------------------------------
#pragma once
//------------------------------------------------------------------------------------------------
#include "SecurityDefinitions.hpp"
//------------------------------------------------------------------------------------------------
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>
//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
namespace Security {
//------------------------------------------------------------------------------------------------

using Buffer = std::vector<std::uint8_t>;
using OptionalBuffer = std::optional<Buffer>;
using SynchronizationResult = std::pair<SynchronizationStatus, Buffer>;

//------------------------------------------------------------------------------------------------
} // Security namespace
//------------------------------------------------------------------------------------------------
