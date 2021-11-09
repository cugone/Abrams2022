#include "Engine/Input/KeyCode.hpp"

#include "Engine/Platform/Win.hpp"

unsigned char ConvertKeyCodeToWinVK(const KeyCode& code) noexcept {
    switch(code) {
    case KeyCode::LButton: return VK_LBUTTON;
    case KeyCode::RButton: return VK_RBUTTON;
    case KeyCode::Cancel: return VK_CANCEL;
    case KeyCode::MButton: return VK_MBUTTON;
    case KeyCode::XButton1: return VK_XBUTTON1;
    case KeyCode::XButton2: return VK_XBUTTON2;
    case KeyCode::Back: return VK_BACK; /* Also BACKSPACE */
    case KeyCode::Tab: return VK_TAB;
    case KeyCode::Clear: return VK_CLEAR;
    case KeyCode::Return: return VK_RETURN; /* Also ENTER */
    case KeyCode::Shift: return VK_SHIFT;
    case KeyCode::Ctrl: return VK_CONTROL; /* Also Ctrl */
    case KeyCode::Menu: return VK_MENU;    /* Also Alt */
    case KeyCode::Pause: return VK_PAUSE;
    case KeyCode::Capital: return VK_CAPITAL; /* Also CAPSLOCK */
    case KeyCode::Kana: return VK_KANA;
    case KeyCode::Hangul: return VK_HANGUL; /* Also HANGEUL */
    case KeyCode::Junja: return VK_JUNJA;
    case KeyCode::Final: return VK_FINAL;
    case KeyCode::Hanja: return VK_HANJA;
    case KeyCode::Kanji: return VK_KANJI;
    case KeyCode::Escape: return VK_ESCAPE; /* Also ESC */
    case KeyCode::Convert: return VK_CONVERT;
    case KeyCode::NonConvert: return VK_NONCONVERT;
    case KeyCode::Accept: return VK_ACCEPT;
    case KeyCode::ModeChange: return VK_MODECHANGE;
    case KeyCode::Space: return VK_SPACE; /* Also SPACEBAR */
    case KeyCode::Prior: return VK_PRIOR; /* Also PAGEUP */
    case KeyCode::Next: return VK_NEXT;   /* Also PAGEDOWN, PAGEDN */
    case KeyCode::End: return VK_END;
    case KeyCode::Home: return VK_HOME;
    case KeyCode::Left: return VK_LEFT;
    case KeyCode::Up: return VK_UP;
    case KeyCode::Right: return VK_RIGHT;
    case KeyCode::Down: return VK_DOWN;
    case KeyCode::Select: return VK_SELECT;
    case KeyCode::Print: return VK_PRINT;
    case KeyCode::Execute: return VK_EXECUTE;
    case KeyCode::Snapshot: return VK_SNAPSHOT; /* Also PRINTSCREEN */
    case KeyCode::Insert: return VK_INSERT;
    case KeyCode::Delete: return VK_DELETE;
    case KeyCode::Help: return VK_HELP;
    case KeyCode::Numeric0: return '0';
    case KeyCode::Numeric1: return '1';
    case KeyCode::Numeric2: return '2';
    case KeyCode::Numeric3: return '3';
    case KeyCode::Numeric4: return '4';
    case KeyCode::Numeric5: return '5';
    case KeyCode::Numeric6: return '6';
    case KeyCode::Numeric7: return '7';
    case KeyCode::Numeric8: return '8';
    case KeyCode::Numeric9: return '9';
    case KeyCode::A: return 'A';
    case KeyCode::B: return 'B';
    case KeyCode::C: return 'C';
    case KeyCode::D: return 'D';
    case KeyCode::E: return 'E';
    case KeyCode::F: return 'F';
    case KeyCode::G: return 'G';
    case KeyCode::H: return 'H';
    case KeyCode::I: return 'I';
    case KeyCode::J: return 'J';
    case KeyCode::K: return 'K';
    case KeyCode::L: return 'L';
    case KeyCode::M: return 'M';
    case KeyCode::N: return 'N';
    case KeyCode::O: return 'O';
    case KeyCode::P: return 'P';
    case KeyCode::Q: return 'Q';
    case KeyCode::R: return 'R';
    case KeyCode::S: return 'S';
    case KeyCode::T: return 'T';
    case KeyCode::U: return 'U';
    case KeyCode::V: return 'V';
    case KeyCode::W: return 'W';
    case KeyCode::X: return 'X';
    case KeyCode::Y: return 'Y';
    case KeyCode::Z: return 'Z';
    case KeyCode::LWin: return VK_LWIN;
    case KeyCode::RWin: return VK_RWIN;
    case KeyCode::Apps: return VK_APPS;
    case KeyCode::Sleep: return VK_SLEEP;
    case KeyCode::NumPad0: return VK_NUMPAD0;
    case KeyCode::NumPad1: return VK_NUMPAD1;
    case KeyCode::NumPad2: return VK_NUMPAD2;
    case KeyCode::NumPad3: return VK_NUMPAD3;
    case KeyCode::NumPad4: return VK_NUMPAD4;
    case KeyCode::NumPad5: return VK_NUMPAD5;
    case KeyCode::NumPad6: return VK_NUMPAD6;
    case KeyCode::NumPad7: return VK_NUMPAD7;
    case KeyCode::NumPad8: return VK_NUMPAD8;
    case KeyCode::NumPad9: return VK_NUMPAD9;
    case KeyCode::Multiply: return VK_MULTIPLY;
    case KeyCode::Add: return VK_ADD;
    case KeyCode::Separator: return VK_SEPARATOR; /* Also KEYPADENTER */
    case KeyCode::Subtract: return VK_SUBTRACT;
    case KeyCode::Decimal: return VK_DECIMAL;
    case KeyCode::Divide: return VK_DIVIDE;
    case KeyCode::F1: return VK_F1;
    case KeyCode::F2: return VK_F2;
    case KeyCode::F3: return VK_F3;
    case KeyCode::F4: return VK_F4;
    case KeyCode::F5: return VK_F5;
    case KeyCode::F6: return VK_F6;
    case KeyCode::F7: return VK_F7;
    case KeyCode::F8: return VK_F8;
    case KeyCode::F9: return VK_F9;
    case KeyCode::F10: return VK_F10;
    case KeyCode::F11: return VK_F11;
    case KeyCode::F12: return VK_F12;
    case KeyCode::F13: return VK_F13;
    case KeyCode::F14: return VK_F14;
    case KeyCode::F15: return VK_F15;
    case KeyCode::F16: return VK_F16;
    case KeyCode::F17: return VK_F17;
    case KeyCode::F18: return VK_F18;
    case KeyCode::F19: return VK_F19;
    case KeyCode::F20: return VK_F20;
    case KeyCode::F21: return VK_F21;
    case KeyCode::F22: return VK_F22;
    case KeyCode::F23: return VK_F23;
    case KeyCode::F24: return VK_F24;
    case KeyCode::NumLock: return VK_NUMLOCK;
    case KeyCode::Scroll: return VK_SCROLL; /* Also SCROLLLOCK */
    case KeyCode::Oem_Nec_Equal: return VK_OEM_NEC_EQUAL;
    case KeyCode::Oem_Fj_Jisho: return VK_OEM_FJ_JISHO;
    case KeyCode::Oem_Fj_Masshou: return VK_OEM_FJ_MASSHOU;
    case KeyCode::Oem_Fj_Touroku: return VK_OEM_FJ_TOUROKU;
    case KeyCode::Oem_Fj_Loya: return VK_OEM_FJ_LOYA;
    case KeyCode::Oem_Fj_Roya: return VK_OEM_FJ_ROYA;
    case KeyCode::LShift: return VK_LSHIFT;
    case KeyCode::RShift: return VK_RSHIFT;
    case KeyCode::LControl: return VK_LCONTROL; /* Also LCTRL */
    case KeyCode::RControl: return VK_RCONTROL; /* Also RCtrl */
    case KeyCode::RMenu: return VK_RMENU;       /* Also RAlt */
    case KeyCode::Browser_Back: return VK_BROWSER_BACK;
    case KeyCode::Browser_Forward: return VK_BROWSER_FORWARD;
    case KeyCode::Browser_Refresh: return VK_BROWSER_REFRESH;
    case KeyCode::Browser_Stop: return VK_BROWSER_STOP;
    case KeyCode::Browser_Search: return VK_BROWSER_SEARCH;
    case KeyCode::Browser_Favorites: return VK_BROWSER_FAVORITES;
    case KeyCode::Browser_Home: return VK_BROWSER_HOME;
    case KeyCode::Volume_Mute: return VK_VOLUME_MUTE;
    case KeyCode::Volume_Down: return VK_VOLUME_DOWN;
    case KeyCode::Volume_Up: return VK_VOLUME_UP;
    case KeyCode::Media_Next_Track: return VK_MEDIA_NEXT_TRACK;
    case KeyCode::Media_Prev_Track: return VK_MEDIA_PREV_TRACK;
    case KeyCode::Media_Stop: return VK_MEDIA_STOP;
    case KeyCode::Media_Play_Pause: return VK_MEDIA_PLAY_PAUSE;
    case KeyCode::Launch_Mail: return VK_LAUNCH_MAIL;
    case KeyCode::Launch_Media_Select: return VK_LAUNCH_MEDIA_SELECT;
    case KeyCode::Launch_App1: return VK_LAUNCH_APP1;
    case KeyCode::Launch_App2: return VK_LAUNCH_APP2;
    case KeyCode::Oem_1: return VK_OEM_1;           /* ;: */
    case KeyCode::Oem_Plus: return VK_OEM_PLUS;     /* =+ */
    case KeyCode::Oem_Comma: return VK_OEM_COMMA;   /* ,< */
    case KeyCode::Oem_Minus: return VK_OEM_MINUS;   /* -_ */
    case KeyCode::Oem_Period: return VK_OEM_PERIOD; /* .> */
    case KeyCode::Oem_2: return VK_OEM_2;           /* /? */
    case KeyCode::Oem_3: return VK_OEM_3;           /* `~ */
    case KeyCode::Oem_4: return VK_OEM_4;           /* [{ */
    case KeyCode::Oem_5: return VK_OEM_5;           /* \| */
    case KeyCode::Oem_6: return VK_OEM_6;           /* ]} */
    case KeyCode::Oem_7: return VK_OEM_7;           /* '" */
    case KeyCode::Oem_8: return VK_OEM_8;           /* misc. unknown */
    case KeyCode::Oem_Ax: return VK_OEM_AX;
    case KeyCode::Oem_102: return VK_OEM_102;   /* RT 102's "<>" or "\|" */
    case KeyCode::Ico_Help: return VK_ICO_HELP; /* Help key on ICO keyboard */
    case KeyCode::Ico_00: return VK_ICO_00;     /* 00 key on ICO keyboard */
    case KeyCode::ProcessKey: return VK_PROCESSKEY;
    case KeyCode::Ico_Clear: return VK_ICO_CLEAR; /* Clear key on ICO keyboard */
    case KeyCode::Packet: return VK_PACKET;       /* Key is packet of data */
    case KeyCode::Oem_Reset: return VK_OEM_RESET;
    case KeyCode::Oem_Jump: return VK_OEM_JUMP;
    case KeyCode::Oem_Pa1: return VK_OEM_PA1;
    case KeyCode::Oem_Pa2: return VK_OEM_PA2;
    case KeyCode::Oem_Pa3: return VK_OEM_PA3;
    case KeyCode::Oem_WsCtrl: return VK_OEM_WSCTRL;
    case KeyCode::Oem_CuSel: return VK_OEM_CUSEL;
    case KeyCode::Oem_Attn: return VK_OEM_ATTN;
    case KeyCode::Oem_Finish: return VK_OEM_FINISH;
    case KeyCode::Oem_Copy: return VK_OEM_COPY;
    case KeyCode::Oem_Auto: return VK_OEM_AUTO;
    case KeyCode::Oem_EnlW: return VK_OEM_ENLW;
    case KeyCode::Oem_BackTab: return VK_OEM_BACKTAB;
    case KeyCode::Attn: return VK_ATTN;
    case KeyCode::CrSel: return VK_CRSEL;
    case KeyCode::ExSel: return VK_EXSEL;
    case KeyCode::ErEof: return VK_EREOF;
    case KeyCode::Play: return VK_PLAY;
    case KeyCode::Zoom: return VK_ZOOM;
    case KeyCode::NoName: return VK_NONAME;
    case KeyCode::Pa1: return VK_PA1;
    case KeyCode::Oem_Clear: return VK_OEM_CLEAR;
    case KeyCode::Unknown: return 0xFF;
    default: return 0xFF;
    }
}

KeyCode ConvertWinVKToKeyCode(unsigned char winVK) noexcept {
    switch(winVK) {
    case VK_LBUTTON: return KeyCode::LButton;
    case VK_RBUTTON: return KeyCode::RButton;
    case VK_CANCEL: return KeyCode::Cancel;
    case VK_MBUTTON: return KeyCode::MButton;
    case VK_XBUTTON1: return KeyCode::XButton1;
    case VK_XBUTTON2: return KeyCode::XButton2;
    case VK_BACK: return KeyCode::Back;
    case VK_TAB: return KeyCode::Tab;
    case VK_CLEAR: return KeyCode::Clear;
    case VK_RETURN: return KeyCode::Return;
    case VK_SHIFT: return KeyCode::Shift;
    case VK_CONTROL: return KeyCode::Ctrl;
    case VK_MENU: return KeyCode::Alt;
    case VK_PAUSE: return KeyCode::Pause;
    case VK_CAPITAL: return KeyCode::Capital;
    case VK_KANA:
        return KeyCode::Kana;
        //case VK_HANGUL: return KeyCode::HANGUL;
    case VK_JUNJA: return KeyCode::Junja;
    case VK_FINAL: return KeyCode::Final;
    case VK_HANJA:
        return KeyCode::Hanja;
        //case VK_KANJI:  return KeyCode::KANJI;
    case VK_ESCAPE: return KeyCode::Escape;
    case VK_CONVERT: return KeyCode::Convert;
    case VK_NONCONVERT: return KeyCode::NonConvert;
    case VK_ACCEPT: return KeyCode::Accept;
    case VK_MODECHANGE: return KeyCode::ModeChange;
    case VK_SPACE: return KeyCode::Space;
    case VK_PRIOR: return KeyCode::Prior;
    case VK_NEXT: return KeyCode::Next;
    case VK_END: return KeyCode::End;
    case VK_HOME: return KeyCode::Home;
    case VK_LEFT: return KeyCode::Left;
    case VK_UP: return KeyCode::Up;
    case VK_RIGHT: return KeyCode::Right;
    case VK_DOWN: return KeyCode::Down;
    case VK_SELECT: return KeyCode::Select;
    case VK_PRINT: return KeyCode::Print;
    case VK_EXECUTE: return KeyCode::Execute;
    case VK_SNAPSHOT: return KeyCode::Snapshot;
    case VK_INSERT: return KeyCode::Insert;
    case VK_DELETE: return KeyCode::Delete;
    case VK_HELP: return KeyCode::Help;
    case '0': return KeyCode::Numeric0;
    case '1': return KeyCode::Numeric1;
    case '2': return KeyCode::Numeric2;
    case '3': return KeyCode::Numeric3;
    case '4': return KeyCode::Numeric4;
    case '5': return KeyCode::Numeric5;
    case '6': return KeyCode::Numeric6;
    case '7': return KeyCode::Numeric7;
    case '8': return KeyCode::Numeric8;
    case '9': return KeyCode::Numeric9;
    case 'A': return KeyCode::A;
    case 'B': return KeyCode::B;
    case 'C': return KeyCode::C;
    case 'D': return KeyCode::D;
    case 'E': return KeyCode::E;
    case 'F': return KeyCode::F;
    case 'G': return KeyCode::G;
    case 'H': return KeyCode::H;
    case 'I': return KeyCode::I;
    case 'J': return KeyCode::J;
    case 'K': return KeyCode::K;
    case 'L': return KeyCode::L;
    case 'M': return KeyCode::M;
    case 'N': return KeyCode::N;
    case 'O': return KeyCode::O;
    case 'P': return KeyCode::P;
    case 'Q': return KeyCode::Q;
    case 'R': return KeyCode::R;
    case 'S': return KeyCode::S;
    case 'T': return KeyCode::T;
    case 'U': return KeyCode::U;
    case 'V': return KeyCode::V;
    case 'W': return KeyCode::W;
    case 'X': return KeyCode::X;
    case 'Y': return KeyCode::Y;
    case 'Z': return KeyCode::Z;
    case VK_LWIN: return KeyCode::LWin;
    case VK_RWIN: return KeyCode::RWin;
    case VK_APPS: return KeyCode::Apps;
    case VK_SLEEP: return KeyCode::Sleep;
    case VK_NUMPAD0: return KeyCode::NumPad0;
    case VK_NUMPAD1: return KeyCode::NumPad1;
    case VK_NUMPAD2: return KeyCode::NumPad2;
    case VK_NUMPAD3: return KeyCode::NumPad3;
    case VK_NUMPAD4: return KeyCode::NumPad4;
    case VK_NUMPAD5: return KeyCode::NumPad5;
    case VK_NUMPAD6: return KeyCode::NumPad6;
    case VK_NUMPAD7: return KeyCode::NumPad7;
    case VK_NUMPAD8: return KeyCode::NumPad8;
    case VK_NUMPAD9: return KeyCode::NumPad9;
    case VK_MULTIPLY: return KeyCode::Multiply;
    case VK_ADD: return KeyCode::Add;
    case VK_SEPARATOR: return KeyCode::Separator;
    case VK_SUBTRACT: return KeyCode::Subtract;
    case VK_DECIMAL: return KeyCode::Decimal;
    case VK_DIVIDE: return KeyCode::Divide;
    case VK_F1: return KeyCode::F1;
    case VK_F2: return KeyCode::F2;
    case VK_F3: return KeyCode::F3;
    case VK_F4: return KeyCode::F4;
    case VK_F5: return KeyCode::F5;
    case VK_F6: return KeyCode::F6;
    case VK_F7: return KeyCode::F7;
    case VK_F8: return KeyCode::F8;
    case VK_F9: return KeyCode::F9;
    case VK_F10: return KeyCode::F10;
    case VK_F11: return KeyCode::F11;
    case VK_F12: return KeyCode::F12;
    case VK_F13: return KeyCode::F13;
    case VK_F14: return KeyCode::F14;
    case VK_F15: return KeyCode::F15;
    case VK_F16: return KeyCode::F16;
    case VK_F17: return KeyCode::F17;
    case VK_F18: return KeyCode::F18;
    case VK_F19: return KeyCode::F19;
    case VK_F20: return KeyCode::F20;
    case VK_F21: return KeyCode::F21;
    case VK_F22: return KeyCode::F22;
    case VK_F23: return KeyCode::F23;
    case VK_F24: return KeyCode::F24;
    case VK_NUMLOCK: return KeyCode::NumLock;
    case VK_SCROLL: return KeyCode::Scroll;
    case VK_OEM_NEC_EQUAL:
        return KeyCode::Oem_Nec_Equal;
        //case VK_OEM_FJ_JISHO:  return KeyCode::OEM_FJ_JISHO;
    case VK_OEM_FJ_MASSHOU: return KeyCode::Oem_Fj_Masshou;
    case VK_OEM_FJ_TOUROKU: return KeyCode::Oem_Fj_Touroku;
    case VK_OEM_FJ_LOYA: return KeyCode::Oem_Fj_Loya;
    case VK_OEM_FJ_ROYA: return KeyCode::Oem_Fj_Roya;
    case VK_LSHIFT: return KeyCode::LShift;
    case VK_RSHIFT: return KeyCode::RShift;
    case VK_LCONTROL: return KeyCode::LControl;
    case VK_RCONTROL: return KeyCode::RControl;
    case VK_LMENU: return KeyCode::LAlt;
    case VK_RMENU: return KeyCode::RAlt;
    case VK_BROWSER_BACK: return KeyCode::Browser_Back;
    case VK_BROWSER_FORWARD: return KeyCode::Browser_Forward;
    case VK_BROWSER_REFRESH: return KeyCode::Browser_Refresh;
    case VK_BROWSER_STOP: return KeyCode::Browser_Stop;
    case VK_BROWSER_SEARCH: return KeyCode::Browser_Search;
    case VK_BROWSER_FAVORITES: return KeyCode::Browser_Favorites;
    case VK_BROWSER_HOME: return KeyCode::Browser_Home;
    case VK_VOLUME_MUTE: return KeyCode::Volume_Mute;
    case VK_VOLUME_DOWN: return KeyCode::Volume_Down;
    case VK_VOLUME_UP: return KeyCode::Volume_Up;
    case VK_MEDIA_NEXT_TRACK: return KeyCode::Media_Next_Track;
    case VK_MEDIA_PREV_TRACK: return KeyCode::Media_Prev_Track;
    case VK_MEDIA_STOP: return KeyCode::Media_Stop;
    case VK_MEDIA_PLAY_PAUSE: return KeyCode::Media_Play_Pause;
    case VK_LAUNCH_MAIL: return KeyCode::Launch_Mail;
    case VK_LAUNCH_MEDIA_SELECT: return KeyCode::Launch_Media_Select;
    case VK_LAUNCH_APP1: return KeyCode::Launch_App1;
    case VK_LAUNCH_APP2: return KeyCode::Launch_App2;
    case VK_OEM_1: return KeyCode::Semicolon;
    case VK_OEM_PLUS: return KeyCode::Equals;
    case VK_OEM_COMMA: return KeyCode::Comma;
    case VK_OEM_MINUS: return KeyCode::Minus;
    case VK_OEM_PERIOD: return KeyCode::Period;
    case VK_OEM_2: return KeyCode::ForwardSlash;
    case VK_OEM_3: return KeyCode::Tilde;
    case VK_GAMEPAD_A: return KeyCode::Gamepad_A;
    case VK_GAMEPAD_B: return KeyCode::Gamepad_B;
    case VK_GAMEPAD_X: return KeyCode::Gamepad_X;
    case VK_GAMEPAD_Y: return KeyCode::Gamepad_Y;
    case VK_GAMEPAD_RIGHT_SHOULDER: return KeyCode::Gamepad_Right_Shoulder;
    case VK_GAMEPAD_LEFT_SHOULDER: return KeyCode::Gamepad_Left_Shoulder;
    case VK_GAMEPAD_LEFT_TRIGGER: return KeyCode::Gamepad_Left_Trigger;
    case VK_GAMEPAD_RIGHT_TRIGGER: return KeyCode::Gamepad_Right_Trigger;
    case VK_GAMEPAD_DPAD_UP: return KeyCode::Gamepad_DPad_Up;
    case VK_GAMEPAD_DPAD_DOWN: return KeyCode::Gamepad_DPad_Down;
    case VK_GAMEPAD_DPAD_LEFT: return KeyCode::Gamepad_DPad_Left;
    case VK_GAMEPAD_DPAD_RIGHT: return KeyCode::Gamepad_DPad_Right;
    case VK_GAMEPAD_MENU: return KeyCode::Gamepad_Menu;
    case VK_GAMEPAD_VIEW: return KeyCode::Gamepad_View;
    case VK_GAMEPAD_LEFT_THUMBSTICK_BUTTON: return KeyCode::Gamepad_Left_Thumbstick_Button;
    case VK_GAMEPAD_RIGHT_THUMBSTICK_BUTTON: return KeyCode::Gamepad_Right_Thumbstick_Button;
    case VK_GAMEPAD_LEFT_THUMBSTICK_UP: return KeyCode::Gamepad_Left_Thumbstick_Up;
    case VK_GAMEPAD_LEFT_THUMBSTICK_DOWN: return KeyCode::Gamepad_Left_Thumbstick_Down;
    case VK_GAMEPAD_LEFT_THUMBSTICK_RIGHT: return KeyCode::Gamepad_Left_Thumbstick_Right;
    case VK_GAMEPAD_LEFT_THUMBSTICK_LEFT: return KeyCode::Gamepad_Left_Thumbstick_Left;
    case VK_GAMEPAD_RIGHT_THUMBSTICK_UP: return KeyCode::Gamepad_Right_Thumbstick_Up;
    case VK_GAMEPAD_RIGHT_THUMBSTICK_DOWN: return KeyCode::Gamepad_Right_Thumbstick_Down;
    case VK_GAMEPAD_RIGHT_THUMBSTICK_RIGHT: return KeyCode::Gamepad_Right_Thumbstick_Right;
    case VK_GAMEPAD_RIGHT_THUMBSTICK_LEFT: return KeyCode::Gamepad_Right_Thumbstick_Left;
    case VK_OEM_4: return KeyCode::LBracket;
    case VK_OEM_5: return KeyCode::Backslash;
    case VK_OEM_6: return KeyCode::RBracket;
    case VK_OEM_7: return KeyCode::SingleQuote;
    case VK_OEM_8: return KeyCode::Oem_8;
    case VK_OEM_AX: return KeyCode::Oem_Ax;
    case VK_OEM_102: return KeyCode::Oem_102;
    case VK_ICO_HELP: return KeyCode::Ico_Help;
    case VK_ICO_00: return KeyCode::Ico_00;
    case VK_PROCESSKEY: return KeyCode::ProcessKey;
    case VK_ICO_CLEAR: return KeyCode::Ico_Clear;
    case VK_PACKET: return KeyCode::Packet;
    case VK_OEM_RESET: return KeyCode::Oem_Reset;
    case VK_OEM_JUMP: return KeyCode::Oem_Jump;
    case VK_OEM_PA1: return KeyCode::Oem_Pa1;
    case VK_OEM_PA2: return KeyCode::Oem_Pa2;
    case VK_OEM_PA3: return KeyCode::Oem_Pa3;
    case VK_OEM_WSCTRL: return KeyCode::Oem_WsCtrl;
    case VK_OEM_CUSEL: return KeyCode::Oem_CuSel;
    case VK_OEM_ATTN: return KeyCode::Oem_Attn;
    case VK_OEM_FINISH: return KeyCode::Oem_Finish;
    case VK_OEM_COPY: return KeyCode::Oem_Copy;
    case VK_OEM_AUTO: return KeyCode::Oem_Auto;
    case VK_OEM_ENLW: return KeyCode::Oem_EnlW;
    case VK_OEM_BACKTAB: return KeyCode::Oem_BackTab;
    case VK_ATTN: return KeyCode::Attn;
    case VK_CRSEL: return KeyCode::CrSel;
    case VK_EXSEL: return KeyCode::ExSel;
    case VK_EREOF: return KeyCode::ErEof;
    case VK_PLAY: return KeyCode::Play;
    case VK_ZOOM: return KeyCode::Zoom;
    case VK_NONAME: return KeyCode::NoName;
    case VK_PA1: return KeyCode::Pa1;
    case VK_OEM_CLEAR: return KeyCode::Oem_Clear;
    default: return KeyCode::Unknown;
    }
}
