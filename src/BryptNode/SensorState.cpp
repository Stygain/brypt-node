//------------------------------------------------------------------------------------------------
#include "SensorState.hpp"
//------------------------------------------------------------------------------------------------

CSensorState::CSensorState()
    : m_mutex()
    , m_pin()
{
}

//------------------------------------------------------------------------------------------------

std::uint8_t CSensorState::GetPin() const
{
    std::shared_lock lock(m_mutex);
    return m_pin;
}

//------------------------------------------------------------------------------------------------

void CSensorState::SetPin(std::uint8_t pin)
{
    std::unique_lock lock(m_mutex);
    m_pin = pin;
}

//------------------------------------------------------------------------------------------------