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
class Command::CInformation : public Command::CHandler {
public:
    CInformation(CNode& instance, std::weak_ptr<CState> const& state);
    ~CInformation() override {};

    void whatami() override;
    bool HandleMessage(CMessage const& message) override;

    bool FloodHandler(CMessage const& message);
    bool RespondHandler();
    bool CloseHandler();

private:
    enum class Phase { FLOOD, RESPOND, CLOSE };
};

//------------------------------------------------------------------------------------------------