#pragma once

#include "Engine/Platform/Win.hpp"

#include <XInput.h>
#pragma comment(lib, "Xinput.lib")

#include "Engine/Math/Vector2.hpp"

#include <bitset>

class XboxController {
public:
    // clang-format off
    enum class Button : unsigned char {
        First_ /* Internal use only. */
        , A = First_
        , B
        , X
        , Y
        , Start
        , Back
        , Up
        , Down
        , Left
        , Right
        , LeftThumb
        , RightThumb
        , RightBumper
        , LeftBumper
        , Last_ /* Internal use only. */
        , Max = Last_ /* Internal use only. */
    };

    enum class Motor {
        Left
        , Right
        , Both
    };
    // clang-format on
    [[nodiscard]] const Vector2& GetLeftThumbPosition() const noexcept;
    [[nodiscard]] const Vector2& GetRightThumbPosition() const noexcept;

    [[nodiscard]] float GetLeftTriggerPosition() const noexcept;
    [[nodiscard]] float GetRightTriggerPosition() const noexcept;

    [[nodiscard]] bool IsButtonUp(const Button& button) const noexcept;
    [[nodiscard]] bool WasButtonJustPressed(const Button& button) const noexcept;
    [[nodiscard]] bool IsButtonDown(const Button& button) const noexcept;
    [[nodiscard]] bool WasButtonJustReleased(const Button& button) const noexcept;

    [[nodiscard]] bool WasJustConnected() const noexcept;
    [[nodiscard]] bool IsConnected() const noexcept;
    [[nodiscard]] bool WasJustDisconnected() const noexcept;
    [[nodiscard]] bool IsDisconnected() const noexcept;

    [[nodiscard]] bool WasAnyButtonJustPressed() const noexcept;
    [[nodiscard]] bool WasAnyButtonJustReleased() const noexcept;
    [[nodiscard]] bool IsAnyButtonDown() const noexcept;

    void Update(int controller_number) noexcept;

    void StartLeftMotor(unsigned short speed) noexcept;
    void StartRightMotor(unsigned short speed) noexcept;
    void StartMotors(unsigned short speed) noexcept;
    void StartLeftMotor(float percent) noexcept;
    void StartRightMotor(float percent) noexcept;
    void StartMotors(float percent) noexcept;

    void StopLeftMotor() noexcept;
    void StopRightMotor() noexcept;
    void StopMotors() noexcept;

    void SetLeftMotorSpeed(unsigned short speed) noexcept;
    void SetRightMotorSpeed(unsigned short speed) noexcept;
    void SetBothMotorSpeed(unsigned short speed) noexcept;

    void SetLeftMotorSpeedToMax() noexcept;
    void SetRightMotorSpeedToMax() noexcept;
    void SetBothMotorSpeedToMax() noexcept;

    void SetLeftMotorSpeedAsPercent(float speed) noexcept;
    void SetRightMotorSpeedAsPercent(float speed) noexcept;
    void SetBothMotorSpeedAsPercent(float speed) noexcept;

    void UpdateConnectedState(int controller_number) noexcept;

protected:
private:
    void UpdateState() noexcept;
    void SetMotorSpeed(int controller_number, const Motor& motor, unsigned short value) noexcept;

    [[nodiscard]] bool DidMotorStateChange() const noexcept;
    // clang-format off
    enum class ActiveState {
        Connected
        , LMotor
        , RMotor
        , Max
    };
    // clang-format on
    Vector2 _leftThumbDistance = Vector2::Zero;
    Vector2 _rightThumbDistance = Vector2::Zero;
    Vector2 _triggerDistances = Vector2::Zero;
    unsigned short _leftMotorState = 0;
    unsigned short _rightMotorState = 0;
    unsigned short _previousRawInput = 0;
    unsigned short _currentRawInput = 0;
    unsigned long _previousPacketNumber = 0;
    unsigned long _currentPacketNumber = 0;
    std::bitset<(std::size_t)ActiveState::Max> _previousActiveState{};
    std::bitset<(std::size_t)ActiveState::Max> _currentActiveState{};
    std::bitset<(std::size_t)Button::Max> _previousButtonState{};
    std::bitset<(std::size_t)Button::Max> _currentButtonState{};
};