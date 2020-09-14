//------------------------------------------------------------------------------------------------
// File: InformationHandler.hpp
// Description:
//------------------------------------------------------------------------------------------------
#pragma once
//------------------------------------------------------------------------------------------------
#include "Handler.hpp"
//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
// Description: Handles the flood phase for the Information type command
//------------------------------------------------------------------------------------------------
class Command::CInformationHandler : public Command::IHandler {
public:
    enum class Phase { Flood, Respond, Close };

    explicit CInformationHandler(CBryptNode& instance);

    // IHandler{
    bool HandleMessage(AssociatedMessage const& associatedMessage) override;
    // }IHandler

    bool FloodHandler(std::weak_ptr<CBryptPeer> const& wpBryptPeer, CMessage const& message);
    bool RespondHandler();
    bool CloseHandler();

};

//------------------------------------------------------------------------------------------------
