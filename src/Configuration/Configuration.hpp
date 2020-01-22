//------------------------------------------------------------------------------------------------
#pragma once
//------------------------------------------------------------------------------------------------
#include "../Utilities/NodeUtils.hpp"
//------------------------------------------------------------------------------------------------
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>
//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
namespace Configuration {
//------------------------------------------------------------------------------------------------

struct TSettings;
struct TDetailsOptions;
struct TConnectionOptions;
struct TSecurityOptions;

//------------------------------------------------------------------------------------------------
} // Configuration namespace
//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------

struct Configuration::TDetailsOptions
{
    TDetailsOptions()
        : version(NodeUtils::NODE_VERSION)
        , name()
        , description()
        , location()
        , operation(NodeUtils::DeviceOperation::NONE)
    {
    }

    TDetailsOptions(
        std::string_view name,
        std::string_view description,
        std::string_view location)
        : version(NodeUtils::NODE_VERSION)
        , name(name)
        , description(description)
        , location(location)
        , operation(NodeUtils::DeviceOperation::NONE)
    {
    }

    std::string version;
    std::string name;
    std::string description;
    std::string location;
    NodeUtils::DeviceOperation operation;
};

//------------------------------------------------------------------------------------------------

struct Configuration::TConnectionOptions
{
    TConnectionOptions()
        : id(0)
        , technology(NodeUtils::TechnologyType::NONE)
        , technology_name()
        , operation(NodeUtils::ConnectionOperation::NONE)
        , interface()
        , binding()
        , entry_address()
    {
    }

    TConnectionOptions(
        NodeUtils::NodeIdType id,
        std::string_view technology_name,
        std::string_view interface,
        std::string_view binding,
        std::string_view entryAddress = std::string())
        : id(id)
        , technology(NodeUtils::TechnologyType::NONE)
        , technology_name(technology_name)
        , operation(NodeUtils::ConnectionOperation::NONE)
        , interface(interface)
        , binding(binding)
        , entry_address(entryAddress)
    {
        technology = NodeUtils::ParseTechnologyType(technology_name.data());
    }

    TConnectionOptions(
        NodeUtils::NodeIdType id,
        NodeUtils::TechnologyType technology,
        std::string_view interface,
        std::string_view binding,
        std::string_view entryAddress = std::string())
        : id(id)
        , technology(technology)
        , technology_name()
        , operation(NodeUtils::ConnectionOperation::NONE)
        , interface(interface)
        , binding(binding)
        , entry_address(entryAddress)
    {
        technology_name = NodeUtils::TechnologyTypeToString(technology);
    }

    NodeUtils::AddressComponentPair GetBindingComponents() const
    {
        return NodeUtils::SplitAddressString(binding);
    }

    NodeUtils::AddressComponentPair GetEntryComponents() const
    {
        return NodeUtils::SplitAddressString(entry_address);
    }

    NodeUtils::NodeIdType id;
    NodeUtils::TechnologyType technology;
    std::string technology_name;
    NodeUtils::ConnectionOperation operation;
    std::string interface;
    std::string binding;
    std::string entry_address;
};

//------------------------------------------------------------------------------------------------

struct Configuration::TSecurityOptions
{
    TSecurityOptions()
        : standard()
        , token()
        , central_authority()
    {
    }

    TSecurityOptions(
        std::string_view standard,
        std::string_view token,
        std::string_view central_authority)
        : standard(standard)
        , token(token)
        , central_authority(central_authority)
    {
    }

    std::string standard;
    std::string token;
    std::string central_authority;
};

//------------------------------------------------------------------------------------------------

struct Configuration::TSettings
{
    TSettings()
        : details()
        , connections()
        , security()
    {
    }

    TSettings(
        TDetailsOptions const& detailsOptions,
        std::vector<TConnectionOptions> const& connectionsOptions,
        TSecurityOptions const& securityOptions)
        : details(detailsOptions)
        , connections(connectionsOptions)
        , security(securityOptions)
    {
    }

    TSettings(TSettings const& other)
        : details(other.details)
        , connections(other.connections)
        , security(other.security)
    {
    }

    TSettings& operator=(TSettings const& other)
    {
        details = other.details;
        connections = other.connections;
        security = other.security;
        return *this;
    }

    TDetailsOptions details;
    std::vector<TConnectionOptions> connections;
    TSecurityOptions security;
};

//------------------------------------------------------------------------------------------------