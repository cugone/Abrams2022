#include "Engine/Input/XboxController.hpp"

#include "Engine/Platform/Win.hpp"
#include "Engine/Math/MathUtils.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IFileLoggerService.hpp"

bool XboxController::WasAnyButtonJustPressed() const noexcept {
    return (_previousButtonState.to_ulong() < _currentButtonState.to_ulong());
}

bool XboxController::WasAnyButtonJustReleased() const noexcept {
    return (_currentButtonState.to_ulong() < _previousButtonState.to_ulong());
}

bool XboxController::IsAnyButtonDown() const noexcept {
    return _currentButtonState.any();
}

const Vector2& XboxController::GetLeftThumbPosition() const noexcept {
    return _leftThumbDistance;
}

const Vector2& XboxController::GetRightThumbPosition() const noexcept {
    return _rightThumbDistance;
}

float XboxController::GetLeftTriggerPosition() const noexcept {
    return _triggerDistances.x;
}

float XboxController::GetRightTriggerPosition() const noexcept {
    return _triggerDistances.y;
}

bool XboxController::IsButtonUp(const Button& button) const noexcept {
    return !_currentButtonState[(std::size_t)button];
}

bool XboxController::WasButtonJustPressed(const Button& button) const noexcept {
    return !_previousButtonState[(std::size_t)button] && _currentButtonState[(std::size_t)button];
}

bool XboxController::IsButtonDown(const Button& button) const noexcept {
    return _currentButtonState[(std::size_t)button];
}

bool XboxController::WasButtonJustReleased(const Button& button) const noexcept {
    return _previousButtonState[(std::size_t)button] && !_currentButtonState[(std::size_t)button];
}

bool XboxController::WasJustConnected() const noexcept {
    return !_previousActiveState[(std::size_t)ActiveState::Connected] && _currentActiveState[(std::size_t)ActiveState::Connected];
}

bool XboxController::IsConnected() const noexcept {
    return _previousActiveState[(std::size_t)ActiveState::Connected] && _currentActiveState[(std::size_t)ActiveState::Connected];
}

bool XboxController::WasJustDisconnected() const noexcept {
    return _previousActiveState[(std::size_t)ActiveState::Connected] && !_currentActiveState[(std::size_t)ActiveState::Connected];
}

bool XboxController::IsDisconnected() const noexcept {
    return !_previousActiveState[(std::size_t)ActiveState::Connected] && !_currentActiveState[(std::size_t)ActiveState::Connected];
}

void XboxController::Update(int controller_number) noexcept {
    XINPUT_STATE state{};

    const auto error_status = ::XInputGetState(controller_number, &state);
    _previousPacketNumber = _currentPacketNumber;
    _currentPacketNumber = state.dwPacketNumber;

    if(error_status == ERROR_DEVICE_NOT_CONNECTED) {
        _previousActiveState = _currentActiveState;
        _currentActiveState[(std::size_t)ActiveState::Connected] = false;
        return;
    }

    if(error_status != ERROR_SUCCESS) {
        auto& logger = ServiceLocator::get<IFileLoggerService>();
        logger.LogErrorLine("XInputGetState returned: " + std::to_string(error_status));
        return;
    }

    _previousActiveState[(std::size_t)ActiveState::Connected] = _currentActiveState[(std::size_t)ActiveState::Connected];
    if(!_currentActiveState[(std::size_t)ActiveState::Connected]) {
        _currentActiveState[(std::size_t)ActiveState::Connected] = true;
    }

    if(_previousPacketNumber == _currentPacketNumber) {
        return;
    }

    _previousRawInput = _currentRawInput;
    _currentRawInput = state.Gamepad.wButtons;

    UpdateState();

    _leftThumbDistance = Vector2(state.Gamepad.sThumbLX, state.Gamepad.sThumbLY);
    _rightThumbDistance = Vector2(state.Gamepad.sThumbRX, state.Gamepad.sThumbRY);
    _triggerDistances = Vector2(state.Gamepad.bLeftTrigger, state.Gamepad.bRightTrigger);

    auto leftRadius = _leftThumbDistance.CalcLength();

    leftRadius = MathUtils::RangeMap<float>(leftRadius, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, 32000, 0.0f, 1.0f);
    leftRadius = std::clamp(leftRadius, 0.0f, 1.0f);

    _leftThumbDistance.SetLength(leftRadius);

    auto rightRadius = _rightThumbDistance.CalcLength();

    rightRadius = MathUtils::RangeMap<float>(rightRadius, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE, 32000, 0.0f, 1.0f);
    rightRadius = std::clamp(rightRadius, 0.0f, 1.0f);

    _rightThumbDistance.SetLength(rightRadius);

    _triggerDistances.x = MathUtils::RangeMap<float>(_triggerDistances.x, static_cast<float>(XINPUT_GAMEPAD_TRIGGER_THRESHOLD), 255.0f, 0.0f, 1.0f);
    _triggerDistances.x = std::clamp(_triggerDistances.x, 0.0f, 1.0f);
    _triggerDistances.y = MathUtils::RangeMap<float>(_triggerDistances.y, static_cast<float>(XINPUT_GAMEPAD_TRIGGER_THRESHOLD), 255.0f, 0.0f, 1.0f);
    _triggerDistances.y = std::clamp(_triggerDistances.y, 0.0f, 1.0f);

    if(DidMotorStateChange()) {
        SetMotorSpeed(controller_number, Motor::Left, _leftMotorState);
        SetMotorSpeed(controller_number, Motor::Right, _rightMotorState);
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
    _currentActiveState[(std::size_t)ActiveState::LMotor] = false;
}

void XboxController::StopRightMotor() noexcept {
    SetRightMotorSpeed(0);
    _currentActiveState[(std::size_t)ActiveState::RMotor] = false;
}

void XboxController::StopMotors() noexcept {
    StopLeftMotor();
    StopRightMotor();
}

void XboxController::SetLeftMotorSpeed(unsigned short speed) noexcept {
    if(speed == _leftMotorState) {
        return;
    }
    _leftMotorState = speed;
    _previousActiveState[(std::size_t)ActiveState::LMotor] = _currentActiveState[(std::size_t)ActiveState::LMotor];
    _currentActiveState[(std::size_t)ActiveState::LMotor] = true;
}

void XboxController::SetRightMotorSpeed(unsigned short speed) noexcept {
    if(speed == _rightMotorState) {
        return;
    }
    _rightMotorState = speed;
    _previousActiveState[(std::size_t)ActiveState::RMotor] = _currentActiveState[(std::size_t)ActiveState::RMotor];
    _currentActiveState[(std::size_t)ActiveState::RMotor] = true;
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
        _previousActiveState[(std::size_t)ActiveState::Connected] = _currentActiveState[(std::size_t)ActiveState::Connected];
        _currentActiveState[(std::size_t)ActiveState::Connected] = false;
    } else if(error_status == ERROR_SUCCESS) {
        _previousActiveState[(std::size_t)ActiveState::Connected] = _currentActiveState[(std::size_t)ActiveState::Connected];
        if(!_currentActiveState[(std::size_t)ActiveState::Connected]) {
            _currentActiveState[(std::size_t)ActiveState::Connected] = true;
        }
    }
}

void XboxController::UpdateState() noexcept {
    _previousButtonState = _currentButtonState;

    _currentButtonState[(std::size_t)Button::Up] = (_currentRawInput & XINPUT_GAMEPAD_DPAD_UP) != 0;
    _currentButtonState[(std::size_t)Button::Down] = (_currentRawInput & XINPUT_GAMEPAD_DPAD_DOWN) != 0;
    _currentButtonState[(std::size_t)Button::Left] = (_currentRawInput & XINPUT_GAMEPAD_DPAD_LEFT) != 0;
    _currentButtonState[(std::size_t)Button::Right] = (_currentRawInput & XINPUT_GAMEPAD_DPAD_RIGHT) != 0;

    _currentButtonState[(std::size_t)Button::Start] = (_currentRawInput & XINPUT_GAMEPAD_START) != 0;
    _currentButtonState[(std::size_t)Button::Back] = (_currentRawInput & XINPUT_GAMEPAD_BACK) != 0;
    _currentButtonState[(std::size_t)Button::LeftThumb] = (_currentRawInput & XINPUT_GAMEPAD_LEFT_THUMB) != 0;
    _currentButtonState[(std::size_t)Button::RightThumb] = (_currentRawInput & XINPUT_GAMEPAD_RIGHT_THUMB) != 0;

    _currentButtonState[(std::size_t)Button::LeftBumper] = (_currentRawInput & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0;
    _currentButtonState[(std::size_t)Button::RightBumper] = (_currentRawInput & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0;

    _currentButtonState[(std::size_t)Button::A] = (_currentRawInput & XINPUT_GAMEPAD_A) != 0;
    _currentButtonState[(std::size_t)Button::B] = (_currentRawInput & XINPUT_GAMEPAD_B) != 0;
    _currentButtonState[(std::size_t)Button::X] = (_currentRawInput & XINPUT_GAMEPAD_X) != 0;
    _currentButtonState[(std::size_t)Button::Y] = (_currentRawInput & XINPUT_GAMEPAD_Y) != 0;
}

void XboxController::SetMotorSpeed(int controller_number, const Motor& motor, unsigned short value) noexcept {
    XINPUT_VIBRATION vibration{};
    switch(motor) {
    case Motor::Left:
        vibration.wLeftMotorSpeed = value;
        vibration.wRightMotorSpeed = _rightMotorState;
        break;
    case Motor::Right:
        vibration.wRightMotorSpeed = value;
        vibration.wLeftMotorSpeed = _leftMotorState;
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
        _currentActiveState[(std::size_t)ActiveState::Connected] = false;
        return;
    } else {
        return;
    }
}

bool XboxController::DidMotorStateChange() const noexcept {
    const bool r = _previousActiveState[(std::size_t)ActiveState::RMotor] ^ _currentActiveState[(std::size_t)ActiveState::RMotor];
    const bool l = _previousActiveState[(std::size_t)ActiveState::LMotor] ^ _currentActiveState[(std::size_t)ActiveState::LMotor];
    return r || l;
}
