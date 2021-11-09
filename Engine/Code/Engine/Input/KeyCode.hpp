#pragma once

#include "Engine/Core/TypeUtils.hpp"

// clang-format off
enum class KeyCode : int {
    FirstMouseButton_ /* Internal use only. */
    , LButton = FirstMouseButton_ /* Left Mouse Button */
    , RButton /* Right Mouse Button */
    , Cancel /* Control-break processing */
    , MButton /* Middle Mouse Button */
    , XButton1 /* Xtra Mouse Button 1 */
    , XButton2 /* Xtra Mouse Button 2 */
    , LastMouseButton_ /* Internal use only. */
    , First_ /* Internal use only. */
    , Back = First_ /* Also Backspace */
    , Backspace = Back /* Also Back */
    , Tab
    , Clear
    , Return /* Also Enter */
    , Enter = Return
    , Shift /* either RShift or LShift */
    , Ctrl /* either RCtrl or LCtrl */
    , Menu /* either RMenu or LMenu, Also Alt */
    , Alt = Menu /* either RAlt or LAlt, Also Menu */
    , Pause
    , Capital /* Also CapsLock */
    , CapsLock = Capital
    , Kana
    , Hangul /* Also Hangeul */
    , Hangeul = Hangul /* Also Hangul */
    , Junja
    , Final
    , Hanja
    , Kanji
    , Escape /* Also Esc */
    , Esc = Escape
    , Convert
    , NonConvert
    , Accept
    , ModeChange
    , Space /* Also Spacebar */
    , Spacebar = Space
    , Prior /* Also PageUp */
    , PageUp = Prior
    , Next /* Also PageDown and PageDn */
    , PageDown = Next /* Also PageDn or Next */
    , PageDn = Next /* Also PageDown or Next */
    , End
    , Home
    , Left
    , Up
    , Right
    , Down
    , Select
    , Print
    , Execute
    , Snapshot /* Also PrintScreen */
    , PrintScreen = Snapshot /* Also Snapshot */
    , Insert
    , Delete /* Also Del */
    , Del = Delete /* Also Delete */
    , Help
    , Numeric0 /* Number key above keyboard */
    , Numeric1 /* Number key above keyboard */
    , Numeric2 /* Number key above keyboard */
    , Numeric3 /* Number key above keyboard */
    , Numeric4 /* Number key above keyboard */
    , Numeric5 /* Number key above keyboard */
    , Numeric6 /* Number key above keyboard */
    , Numeric7 /* Number key above keyboard */
    , Numeric8 /* Number key above keyboard */
    , Numeric9 /* Number key above keyboard */
    , A
    , B
    , C
    , D
    , E
    , F
    , G
    , H
    , I
    , J
    , K
    , L
    , M
    , N
    , O
    , P
    , Q
    , R
    , S
    , T
    , U
    , V
    , W
    , X
    , Y
    , Z
    , LWin
    , RWin
    , Apps
    , Sleep
    , NumPad0
    , NumPad1
    , NumPad2
    , NumPad3
    , NumPad4
    , NumPad5
    , NumPad6
    , NumPad7
    , NumPad8
    , NumPad9
    , Multiply /* NumPad * */
    , Add /* NumPad + */
    , Separator /* Also NumPadEnter */
    , NumPadEnter = Separator /* Also Separator */
    , Subtract /* NumPad - */
    , Decimal /* NumPad . */
    , Divide /* NumPad / */
    , F1
    , F2
    , F3
    , F4
    , F5
    , F6
    , F7
    , F8
    , F9
    , F10
    , F11
    , F12
    , F13
    , F14
    , F15
    , F16
    , F17
    , F18
    , F19
    , F20
    , F21
    , F22
    , F23
    , F24
    , NumLock
    , Scroll /* Also ScrollLock */
    , ScrollLock = Scroll /* Also Scroll */
    , Oem_Nec_Equal
    , Oem_Fj_Jisho
    , Oem_Fj_Masshou
    , Oem_Fj_Touroku
    , Oem_Fj_Loya
    , Oem_Fj_Roya
    , LShift
    , RShift
    , LControl /* Also LCtrl */
    , LCtrl = LControl /* Also LControl */
    , RControl /* Also RCtrl */
    , RCtrl = RControl /* Also RControl */
    , RMenu /* Also RAlt */
    , RAlt = RMenu /* Also RMenu */
    , LMenu /* Also LAlt */
    , LAlt = LMenu /* Also LMenu */
    , Browser_Back
    , Browser_Forward
    , Browser_Refresh
    , Browser_Stop
    , Browser_Search
    , Browser_Favorites
    , Browser_Home
    , Volume_Mute
    , Volume_Down
    , Volume_Up
    , Media_Next_Track
    , Media_Prev_Track
    , Media_Stop
    , Media_Play_Pause
    , Launch_Mail
    , Launch_Media_Select
    , Launch_App1
    , Launch_App2
    , Oem_1 /* Key ;: */
    , Semicolon = Oem_1 /* Key ;: */
    , Oem_Plus /* Key =+ */
    , Equals = Oem_Plus /* Key =+ */
    , Oem_Comma /* Key ,< */
    , Comma = Oem_Comma /* Key ,< */
    , Oem_Minus /* Key -_ */
    , Minus = Oem_Minus /* Key -_ */
    , Oem_Period /* Key .> */
    , Period = Oem_Period /* Key .> */
    , Oem_2 /* Key /? */
    , ForwardSlash = Oem_2 /* Key /?, Also FSlash */
    , FSlash = Oem_2 /* Key /?, Also ForwardSlash */
    , Oem_3 /* Key `~ */
    , Backquote = Oem_3 /* Key `~, Also Tilde */
    , Tilde = Oem_3 /* Key `~, Also Backquote */
    , Gamepad_First_ /* Internal use only. */
    , Gamepad_A = Gamepad_First_
    , Gamepad_FaceButton_Bottom = Gamepad_A /* Also Gamepad_A */
    , Gamepad_B
    , Gamepad_FaceButton_Right = Gamepad_B /* Also Gamepad_B */
    , Gamepad_X
    , Gamepad_FaceButton_Left = Gamepad_X /* Also Gamepad_X */
    , Gamepad_Y
    , Gamepad_FaceButton_Top = Gamepad_Y /* Also Gamepad_Y */
    , Gamepad_Right_Shoulder
    , Gamepad_Left_Shoulder
    , Gamepad_Left_Trigger
    , Gamepad_Right_Trigger
    , Gamepad_DPad_Up
    , Gamepad_DPad_Down
    , Gamepad_DPad_Left
    , Gamepad_DPad_Right
    , Gamepad_Menu
    , Gamepad_View
    , Gamepad_Left_Thumbstick_Button
    , Gamepad_Right_Thumbstick_Button
    , Gamepad_Left_Thumbstick_Up
    , Gamepad_Left_Thumbstick_Down
    , Gamepad_Left_Thumbstick_Right
    , Gamepad_Left_Thumbstick_Left
    , Gamepad_Right_Thumbstick_Up
    , Gamepad_Right_Thumbstick_Down
    , Gamepad_Right_Thumbstick_Right
    , Gamepad_Right_Thumbstick_Left
    , Gamepad_Last_ /* Internal use only. */
    , Oem_4 /* Key [{ */
    , LeftBracket = Oem_4 /* Key [{, Also LBracket */
    , LBracket = Oem_4 /* Key [{, Also LeftBracket */
    , Oem_5 /* Key \|, Also Backslash */
    , Backslash = Oem_5 /* Key \| */
    , Oem_6 /* Key ]} */
    , RightBracket = Oem_6 /* Key ]}, Also RBracket */
    , RBracket = Oem_6 /* Key ]}, Also RightBracket */
    , Oem_7 /* Key '" */
    , Apostrophe = Oem_7 /* Key '", Also Apostrophe */
    , SingleQuote = Oem_7 /* Key '", Also SingleQuote */
    , Oem_8 /* misc. unknown */
    , Oem_Ax
    , Oem_102 /* RT 102's "<>" or "\|" */
    , Ico_Help /* Help key on ICO keyboard */
    , Ico_00 /* 00 key on ICO keyboard */
    , ProcessKey
    , Ico_Clear /* Clear key on ICO keyboard */
    , Packet /* Key is packet of data */
    , Oem_Reset
    , Oem_Jump
    , Oem_Pa1
    , Oem_Pa2
    , Oem_Pa3
    , Oem_WsCtrl
    , Oem_CuSel
    , Oem_Attn
    , Oem_Finish
    , Oem_Copy
    , Oem_Auto
    , Oem_EnlW
    , Oem_BackTab
    , Attn
    , CrSel
    , ExSel
    , ErEof
    , Play
    , Zoom
    , NoName
    , Pa1
    , Oem_Clear
    , Last_ /* Internal use only */
    , Unknown = 0xFF /* A manufacturer-specific key was pressed. */
    , Max /* Internal use only */
};
// clang-format on

template<>
struct TypeUtils::is_incrementable_enum_type<KeyCode> : std::true_type {};


[[nodiscard]] unsigned char ConvertKeyCodeToWinVK(const KeyCode& code) noexcept;
[[nodiscard]] KeyCode ConvertWinVKToKeyCode(unsigned char winVK) noexcept;
