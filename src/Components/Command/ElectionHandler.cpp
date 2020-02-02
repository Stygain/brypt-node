//------------------------------------------------------------------------------------------------
// File: ElectionHandler.cpp
// Description:
//------------------------------------------------------------------------------------------------
#include "ElectionHandler.hpp"
//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
// Description:
//------------------------------------------------------------------------------------------------
Command::CElection::CElection(CNode& instance, std::weak_ptr<CState> const& state)
    : CHandler(instance, state, NodeUtils::CommandType::ELECTION)
{
}

//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
// Description: Election message handler, drives each of the message responses based on the phase
// Returns: Status of the message handling
//------------------------------------------------------------------------------------------------
bool Command::CElection::HandleMessage(CMessage const& message) {
    bool status = false;

    auto const phase = static_cast<CElection::Phase>(message.GetPhase());
    switch (phase) {
        case Phase::PROBE: {
            status = ProbeHandler();
        } break;
        case Phase::PRECOMMIT: {
            status = PrecommitHandler();
        } break;
        case Phase::VOTE: {
            status = VoteHandler();
        } break;
        case Phase::ABORT: {
            status = AbortHandler();
        } break;
        case Phase::RESULTS: {
            status = ResultsHandler();
        } break;
        case Phase::CLOSE: {
            status = CloseHandler();
        } break;
        default: break;
    }

    return status;
}

//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
// Description:
// Returns: Status of the message handling
//------------------------------------------------------------------------------------------------
bool Command::CElection::ProbeHandler()
{
    return false;
}

//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
// Description:
// Returns: Status of the message handling
//------------------------------------------------------------------------------------------------
bool Command::CElection::PrecommitHandler()
{
    return false;
}

//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
// Description:
// Returns: Status of the message handling
//------------------------------------------------------------------------------------------------
bool Command::CElection::VoteHandler()
{
    return false;
}

//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
// Description:
// Returns: Status of the message handling
//------------------------------------------------------------------------------------------------
bool Command::CElection::AbortHandler()
{
    return false;
}

//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
// Description:
// Returns: Status of the message handling
//------------------------------------------------------------------------------------------------
bool Command::CElection::ResultsHandler()
{
    return false;
}

//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
// Description:
// Returns: Status of the message handling
//------------------------------------------------------------------------------------------------
bool Command::CElection::CloseHandler()
{
    return false;
}

//------------------------------------------------------------------------------------------------
