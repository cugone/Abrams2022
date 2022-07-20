#include "Engine/Input/XboxController.hpp"

#include "Engine/Platform/Win.hpp"
#include "Engine/Math/MathUtils.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IFileLoggerService.hpp"

bool XboxController::WasAnyButtonJustPressed() const noexcept {
    return (m_previousButtonState.to_ulong() < m_currentButtonState.to_ulong());
}

bool XboxController::WasAnyButtonJustReleased() const noexcept {
    return (m_currentButtonState.to_ulong() < m_previousButtonState.to_ulong());
}

bool XboxController::IsAnyButtonDown() const noexcept {
    return m_currentButtonState.any();
}

const Vector2& XboxController::GetLeftThumbPosition() const noexcept {
    return m_leftThumbDistance;
}

const Vector2& XboxController::GetRightThumbPosition() const noexcept {
    return m_rightThumbDistance;
}

float XboxController::GetLeftTriggerPosition() const noexcept {
    return m_triggerDistances.x;
}

float XboxController::GetRightTriggerPosition() const noexcept {
    return m_triggerDistances.y;
}

bool XboxController::IsButtonUp(const Button& button) const noexcept {
    return !m_currentButtonState[(std::size_t)button];
}

bool XboxController::WasButtonJustPressed(const Button& button) const noexcept {
    return !m_previousButtonState[(std::size_t)button] && m_currentButtonState[(std::size_t)button];
}

bool XboxController::IsButtonDown(const Button& button) const noexcept {
    return m_currentButtonState[(std::size_t)button];
}

bool XboxController::WasButtonJustReleased(const Button& button) const noexcept {
    return m_previousButtonState[(std::size_t)button] && !m_currentButtonState[(std::size_t)button];
}

bool XboxController::WasJustConnected() const noexcept {
    return !m_previousActiveState[(std::size_t)ActiveState::Connected] && m_currentActiveState[(std::size_t)ActiveState::Connected];
}

bool XboxController::IsConnected() const noexcept {
    return m_previousActiveState[(std::size_t)ActiveState::Connected] && m_currentActiveState[(std::size_t)ActiveState::Connected];
}

bool XboxController::WasJustDisconnected() const noexcept {
    return m_previousActiveState[(std::size_t)ActiveState::Connected] && !m_currentActiveState[(std::size_t)ActiveState::Connected];
}

bool XboxController::IsDisconnected() const noexcept {
    return !m_previousActiveState[(std::size_t)ActiveState::Connected] && !m_currentActiveState[(std::size_t)ActiveState::Connected];
}

void XboxController::Update(int controller_number) noexcept {
    XINPUT_STATE state{};

    const auto error_status = ::XInputGetState(controller_number, &state);
    m_previousPacketNumber = m_currentPacketNumber;
    m_currentPacketNumber = state.dwPacketNumber;

    if(error_status == ERROR_DEVICE_NOT_CONNECTED) {
        m_previousActiveState = m_currentActiveState;
        m_currentActiveState[(std::size_t)ActiveState::Connected] = false;
        return;
    }

    if(error_status != ERROR_SUCCESS) {
        auto* logger = ServiceLocator::get<IFileLoggerService>();
        logger->LogErrorLine("XInputGetState returned: " + std::to_string(error_status));
        return;
    }

    m_previousActiveState[(std::size_t)ActiveState::Connected] = m_currentActiveState[(std::size_t)ActiveState::Connected];
    if(!m_currentActiveState[(std::size_t)ActiveState::Connected]) {
        m_currentActiveState[(std::size_t)ActiveState::Connected] = true;
    }

    if(m_previousPacketNumber == m_currentPacketNumber) {
        return;
    }

    m_previousRawInput = m_currentRawInput;
    m_currentRawInput = state.Gamepad.wButtons;

    UpdateState();

    m_leftThumbDistance = Vector2(state.Gamepad.sThumbLX, state.Gamepad.sThumbLY);
    m_rightThumbDistance = Vector2(state.Gamepad.sThumbRX, state.Gamepad.sThumbRY);
    m_triggerDistances = Vector2(state.Gamepad.bLeftTrigger, state.Gamepad.bRightTrigger);

    auto leftRadius = m_leftThumbDistance.CalcLength();

    leftRadius = MathUtils::RangeMap<float>(leftRadius, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, 32000, 0.0f, 1.0f);
    leftRadius = std::clamp(leftRadius, 0.0f, 1.0f);

    m_leftThumbDistance.SetLength(leftRadius);

    auto rightRadius = m_rightThumbDistance.CalcLength();

    rightRadius = MathUtils::RangeMap<float>(rightRadius, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE, 32000, 0.0f, 1.0f);
    rightRadius = std::clamp(rightRadius, 0.0f, 1.0f);

    m_rightThumbDistance.SetLength(rightRadius);

    m_triggerDistances.x = MathUtils::RangeMap<float>(m_triggerDistances.x, static_cast<float>(XINPUT_GAMEPAD_TRIGGER_THRESHOLD), 255.0f, 0.0f, 1.0f);
    m_triggerDistances.x = std::clamp(m_triggerDistances.x, 0.0f, 1.0f);
    m_triggerDistances.y = MathUtils::RangeMap<float>(m_triggerDistances.y, static_cast<float>(XINPUT_GAMEPAD_TRIGGER_THRESHOLD), 255.0f, 0.0f, 1.0f);
    m_triggerDistances.y = std::clamp(m_triggerDistances.y, 0.0f, 1.0f);

    if(DidMotorStateChange()) {
        SetMotorSpeed(controller_number, Motor::Left, m_leftMotorState);
        SetMotorSpeed(controller_number, Motor::Right, m_rightMotorState);
    }
}

void XboxController::StartLeftMotor(unsigned short speed) noexcept {
    SetLeftMotorSpeed(speed);
}

void XboxController::StartRightMotor(unsigned short speed) noexcept {
    SetRightMotorSpeed(speed);
}

void XboxController::StartMotors(unsigned short speed) noexcept {
    SetBothMotorSpeed(speed);
}

void XboxController::StartLeftMotor(float percent) noexcept {
    StartLeftMotor(static_cast<unsigned short>(percent * (std::numeric_limits<unsigned short>::max)()));
}

void XboxController::StartRightMotor(float percent) noexcept {
    StartRightMotor(static_cast<unsigned short>(percent * (std::numeric_limits<unsigned short>::max)()));
}

void XboxController::StartMotors(float percent) noexcept {
    StartLeftMotor(percent);
    StartRightMotor(percent);
}

void XboxController::StopLeftMotor() noexcept {
    SetLeftMotorSpeed(0);
    m_currentActiveState[(std::size_t)ActiveState::LMotor] = false;
}

void XboxController::StopRightMotor() noexcept {
    SetRightMotorSpeed(0);
    m_currentActiveState[(std::size_t)ActiveState::RMotor] = false;
}

void XboxController::StopMotors() noexcept {
    StopLeftMotor();
    StopRightMotor();
}

void XboxController::SetLeftMotorSpeed(unsigned short speed) noexcept {
    if(speed == m_leftMotorState) {
        return;
    }
    m_leftMotorState = speed;
    m_previousActiveState[(std::size_t)ActiveState::LMotor] = m_currentActiveState[(std::size_t)ActiveState::LMotor];
    m_currentActiveState[(std::size_t)ActiveState::LMotor] = true;
}

void XboxController::SetRightMotorSpeed(unsigned short speed) noexcept {
    if(speed == m_rightMotorState) {
        return;
    }
    m_rightMotorState = speed;
    m_previousActiveState[(std::size_t)ActiveState::RMotor] = m_currentActiveState[(std::size_t)ActiveState::RMotor];
    m_currentActiveState[(std::size_t)ActiveState::RMotor] = true;
}

void XboxController::SetBothMotorSpeed(unsigned short speed) noexcept {
    SetLeftMotorSpeed(speed);
    SetRightMotorSpeed(speed);
}

void XboxController::SetLeftMotorSpeedToMax() noexcept {
    SetLeftMotorSpeedAsPercent(1.0f);
}

void XboxController::SetRightMotorSpeedToMax() noexcept {
    SetRightMotorSpeedAsPercent(1.0f);
}

void XboxController::SetBothMotorSpeedToMax() noexcept {
    SetLeftMotorSpeedToMax();
    SetRightMotorSpeedToMax();
}

void XboxController::SetLeftMotorSpeedAsPercent(float speed) noexcept {
    speed = std::clamp(speed, 0.0f, 1.0f);
    SetLeftMotorSpeed(static_cast<unsigned short>(static_cast<float>((std::numeric_limits<unsigned short>::max)()) * speed));
}

void XboxController::SetRightMotorSpeedAsPercent(float speed) noexcept {
    speed = std::clamp(speed, 0.0f, 1.0f);
    SetRightMotorSpeed(static_cast<unsigned short>(static_cast<float>((std::numeric_limits<unsigned short>::max)()) * speed));
}

void XboxController::SetBothMotorSpeedAsPercent(float speed) noexcept {
    SetLeftMotorSpeedAsPercent(speed);
    SetRightMotorSpeedAsPercent(speed);
}

void XboxController::UpdateConnectedState(int controller_number) noexcept {
    XINPUT_STATE state{};
    const auto error_status = ::XInputGetState(controller_number, &state);

    if(error_status == ERROR_DEVICE_NOT_CONNECTED) {
        m_previousActiveState[(std::size_t)ActiveState::Connected] = m_currentActiveState[(std::size_t)ActiveState::Connected];
        m_currentActiveState[(std::size_t)ActiveState::Connected] = false;
    } else if(error_status == ERROR_SUCCESS) {
        m_previousActiveState[(std::size_t)ActiveState::Connected] = m_currentActiveState[(std::size_t)ActiveState::Connected];
        if(!m_currentActiveState[(std::size_t)ActiveState::Connected]) {
            m_currentActiveState[(std::size_t)ActiveState::Connected] = true;
        }
    }
}

void XboxController::UpdateState() noexcept {
    m_previousButtonState = m_currentButtonState;

    m_currentButtonState[(std::size_t)Button::Up] = (m_currentRawInput & XINPUT_GAMEPAD_DPAD_UP) != 0;
    m_currentButtonState[(std::size_t)Button::Down] = (m_currentRawInput & XINPUT_GAMEPAD_DPAD_DOWN) != 0;
    m_currentButtonState[(std::size_t)Button::Left] = (m_currentRawInput & XINPUT_GAMEPAD_DPAD_LEFT) != 0;
    m_currentButtonState[(std::size_t)Button::Right] = (m_currentRawInput & XINPUT_GAMEPAD_DPAD_RIGHT) != 0;

    m_currentButtonState[(std::size_t)Button::Start] = (m_currentRawInput & XINPUT_GAMEPAD_START) != 0;
    m_currentButtonState[(std::size_t)Button::Back] = (m_currentRawInput & XINPUT_GAMEPAD_BACK) != 0;
    m_currentButtonState[(std::size_t)Button::LeftThumb] = (m_currentRawInput & XINPUT_GAMEPAD_LEFT_THUMB) != 0;
    m_currentButtonState[(std::size_t)Button::RightThumb] = (m_currentRawInput & XINPUT_GAMEPAD_RIGHT_THUMB) != 0;

    m_currentButtonState[(std::size_t)Button::LeftBumper] = (m_currentRawInput & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0;
    m_currentButtonState[(std::size_t)Button::RightBumper] = (m_currentRawInput & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0;

    m_currentButtonState[(std::size_t)Button::A] = (m_currentRawInput & XINPUT_GAMEPAD_A) != 0;
    m_currentButtonState[(std::size_t)Button::B] = (m_currentRawInput & XINPUT_GAMEPAD_B) != 0;
    m_currentButtonState[(std::size_t)Button::X] = (m_currentRawInput & XINPUT_GAMEPAD_X) != 0;
    m_currentButtonState[(std::size_t)Button::Y] = (m_currentRawInput & XINPUT_GAMEPAD_Y) != 0;
}

void XboxController::SetMotorSpeed(int controller_number, const Motor& motor, unsigned short value) noexcept {
    XINPUT_VIBRATION vibration{};
    switch(motor) {
    case Motor::Left:
        vibration.wLeftMotorSpeed = value;
        vibration.wRightMotorSpeed = m_rightMotorState;
        break;
    case Motor::Right:
        vibration.wRightMotorSpeed = value;
        vibration.wLeftMotorSpeed = m_leftMotorState;
        break;
    case Motor::Both:
        vibration.wLeftMotorSpeed = value;
        vibration.wRightMotorSpeed = value;
        break;
    default:
        /* DO NOTHING */
        break;
    }
    DWORD errorStatus = ::XInputSetState(controller_number, &vibration);
    if(errorStatus == ERROR_SUCCESS) {
        return;
    } else if(errorStatus == ERROR_DEVICE_NOT_CONNECTED) {
        m_currentActiveState[(std::size_t)ActiveState::Connected] = false;
        return;
    } else {
        return;
    }
}

bool XboxController::DidMotorStateChange() const noexcept {
    const bool r = m_previousActiveState[(std::size_t)ActiveState::RMotor] ^ m_currentActiveState[(std::size_t)ActiveState::RMotor];
    const bool l = m_previousActiveState[(std::size_t)ActiveState::LMotor] ^ m_currentActiveState[(std::size_t)ActiveState::LMotor];
    return r || l;
}
