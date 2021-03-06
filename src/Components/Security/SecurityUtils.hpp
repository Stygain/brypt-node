//------------------------------------------------------------------------------------------------
// File: SecurityUtils.hpp
// Description: 
//------------------------------------------------------------------------------------------------
#pragma once
//------------------------------------------------------------------------------------------------
#include "SecurityDefinitions.hpp"
#include "Interfaces/SecurityStrategy.hpp"
//------------------------------------------------------------------------------------------------
#include <cstdint>
#include <memory>
#include <type_traits>
//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
namespace Security {
//------------------------------------------------------------------------------------------------

Security::Strategy ConvertToStrategy(std::underlying_type_t<Security::Strategy> strategy);
Security::Strategy ConvertToStrategy(std::string strategy);
std::unique_ptr<ISecurityStrategy> CreateStrategy(
    Security::Strategy strategy, Security::Role role, Security::Context context);

void EraseMemory(void* begin, std::size_t size);

//------------------------------------------------------------------------------------------------
} // Security namespace
//------------------------------------------------------------------------------------------------