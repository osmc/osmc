/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "AndroidKey.h"
#include "AndroidExtra.h"
#include "XBMCApp.h"
#include "input/Key.h"
#include "windowing/WinEvents.h"
#include "android/jni/KeyCharacterMap.h"


typedef struct {
  int32_t nativeKey;
  int16_t xbmcKey;
} KeyMap;

static KeyMap keyMap[] = {
  { AKEYCODE_UNKNOWN         , XBMCK_LAST },
  { AKEYCODE_SOFT_LEFT       , XBMCK_LEFT },
  { AKEYCODE_SOFT_RIGHT      , XBMCK_RIGHT },
  { AKEYCODE_HOME            , XBMCK_HOME },
  { AKEYCODE_BACK            , XBMCK_BACKSPACE },
  { AKEYCODE_CALL            , XBMCK_LAST },
  { AKEYCODE_ENDCALL         , XBMCK_LAST },
  { AKEYCODE_0               , XBMCK_0 },
  { AKEYCODE_1               , XBMCK_1 },
  { AKEYCODE_2               , XBMCK_2 },
  { AKEYCODE_3               , XBMCK_3 },
  { AKEYCODE_4               , XBMCK_4 },
  { AKEYCODE_5               , XBMCK_5 },
  { AKEYCODE_6               , XBMCK_6 },
  { AKEYCODE_7               , XBMCK_7 },
  { AKEYCODE_8               , XBMCK_8 },
  { AKEYCODE_9               , XBMCK_9 },
  { AKEYCODE_STAR            , XBMCK_ASTERISK },
  { AKEYCODE_POUND           , XBMCK_HASH },
  { AKEYCODE_DPAD_UP         , XBMCK_UP },
  { AKEYCODE_DPAD_DOWN       , XBMCK_DOWN },
  { AKEYCODE_DPAD_LEFT       , XBMCK_LEFT },
  { AKEYCODE_DPAD_RIGHT      , XBMCK_RIGHT },
  { AKEYCODE_DPAD_CENTER     , XBMCK_RETURN },
  { AKEYCODE_VOLUME_UP       , XBMCK_LAST },
  { AKEYCODE_VOLUME_DOWN     , XBMCK_LAST },
  { AKEYCODE_POWER           , XBMCK_POWER },
  { AKEYCODE_CAMERA          , XBMCK_LAST },
  { AKEYCODE_CLEAR           , XBMCK_LAST },
  { AKEYCODE_A               , XBMCK_a },
  { AKEYCODE_B               , XBMCK_b },
  { AKEYCODE_C               , XBMCK_c },
  { AKEYCODE_D               , XBMCK_d },
  { AKEYCODE_E               , XBMCK_e },
  { AKEYCODE_F               , XBMCK_f },
  { AKEYCODE_G               , XBMCK_g },
  { AKEYCODE_H               , XBMCK_h },
  { AKEYCODE_I               , XBMCK_i },
  { AKEYCODE_J               , XBMCK_j },
  { AKEYCODE_K               , XBMCK_k },
  { AKEYCODE_L               , XBMCK_l },
  { AKEYCODE_M               , XBMCK_m },
  { AKEYCODE_N               , XBMCK_n },
  { AKEYCODE_O               , XBMCK_o },
  { AKEYCODE_P               , XBMCK_p },
  { AKEYCODE_Q               , XBMCK_q },
  { AKEYCODE_R               , XBMCK_r },
  { AKEYCODE_S               , XBMCK_s },
  { AKEYCODE_T               , XBMCK_t },
  { AKEYCODE_U               , XBMCK_u },
  { AKEYCODE_V               , XBMCK_v },
  { AKEYCODE_W               , XBMCK_w },
  { AKEYCODE_X               , XBMCK_x },
  { AKEYCODE_Y               , XBMCK_y },
  { AKEYCODE_Z               , XBMCK_z },
  { AKEYCODE_COMMA           , XBMCK_COMMA },
  { AKEYCODE_PERIOD          , XBMCK_PERIOD },
  { AKEYCODE_ALT_LEFT        , XBMCK_LALT },
  { AKEYCODE_ALT_RIGHT       , XBMCK_RALT },
  { AKEYCODE_SHIFT_LEFT      , XBMCK_LSHIFT },
  { AKEYCODE_SHIFT_RIGHT     , XBMCK_RSHIFT },
  { AKEYCODE_TAB             , XBMCK_TAB },
  { AKEYCODE_SPACE           , XBMCK_SPACE },
  { AKEYCODE_SYM             , XBMCK_LAST },
  { AKEYCODE_EXPLORER        , XBMCK_LAST },
  { AKEYCODE_ENVELOPE        , XBMCK_LAST },
  { AKEYCODE_ENTER           , XBMCK_RETURN },
  { AKEYCODE_DEL             , XBMCK_BACKSPACE },
  { AKEYCODE_GRAVE           , XBMCK_BACKQUOTE },
  { AKEYCODE_MINUS           , XBMCK_MINUS },
  { AKEYCODE_EQUALS          , XBMCK_EQUALS },
  { AKEYCODE_LEFT_BRACKET    , XBMCK_LEFTBRACKET },
  { AKEYCODE_RIGHT_BRACKET   , XBMCK_RIGHTBRACKET },
  { AKEYCODE_BACKSLASH       , XBMCK_BACKSLASH },
  { AKEYCODE_SEMICOLON       , XBMCK_SEMICOLON },
  { AKEYCODE_APOSTROPHE      , XBMCK_QUOTE },
  { AKEYCODE_SLASH           , XBMCK_SLASH },
  { AKEYCODE_AT              , XBMCK_AT },
  { AKEYCODE_NUM             , XBMCK_NUMLOCK },
  { AKEYCODE_HEADSETHOOK     , XBMCK_LAST },
  { AKEYCODE_FOCUS           , XBMCK_LAST },   // *Camera* focus
  { AKEYCODE_PLUS            , XBMCK_PLUS },
  { AKEYCODE_MENU            , XBMCK_MENU },
  { AKEYCODE_NOTIFICATION    , XBMCK_LAST },
  { AKEYCODE_SEARCH          , XBMCK_LAST },
  { AKEYCODE_MEDIA_PLAY_PAUSE, XBMCK_MEDIA_PLAY_PAUSE },
  { AKEYCODE_MEDIA_STOP      , XBMCK_MEDIA_STOP },
  { AKEYCODE_MEDIA_NEXT      , XBMCK_MEDIA_NEXT_TRACK },
  { AKEYCODE_MEDIA_PREVIOUS  , XBMCK_MEDIA_PREV_TRACK },
  { AKEYCODE_MEDIA_REWIND    , XBMCK_MEDIA_REWIND },
  { AKEYCODE_MEDIA_FAST_FORWARD , XBMCK_MEDIA_FASTFORWARD },
  { AKEYCODE_MUTE            , XBMCK_LAST },
  { AKEYCODE_PAGE_UP         , XBMCK_PAGEUP },
  { AKEYCODE_PAGE_DOWN       , XBMCK_PAGEDOWN },
  { AKEYCODE_PICTSYMBOLS     , XBMCK_LAST },
  { AKEYCODE_SWITCH_CHARSET  , XBMCK_LAST },
  { AKEYCODE_BUTTON_A        , XBMCK_LAST },
  { AKEYCODE_BUTTON_B        , XBMCK_LAST },
  { AKEYCODE_BUTTON_C        , XBMCK_LAST },
  { AKEYCODE_BUTTON_X        , XBMCK_LAST },
  { AKEYCODE_BUTTON_Y        , XBMCK_LAST },
  { AKEYCODE_BUTTON_Z        , XBMCK_LAST },
  { AKEYCODE_BUTTON_L1       , XBMCK_LAST },
  { AKEYCODE_BUTTON_R1       , XBMCK_LAST },
  { AKEYCODE_BUTTON_L2       , XBMCK_LAST },
  { AKEYCODE_BUTTON_R2       , XBMCK_LAST },
  { AKEYCODE_BUTTON_THUMBL   , XBMCK_LAST },
  { AKEYCODE_BUTTON_THUMBR   , XBMCK_LAST },
  { AKEYCODE_BUTTON_START    , XBMCK_LAST },
  { AKEYCODE_BUTTON_SELECT   , XBMCK_LAST },
  { AKEYCODE_BUTTON_MODE     , XBMCK_LAST },
  { AKEYCODE_ESCAPE          , XBMCK_ESCAPE },
  { AKEYCODE_FORWARD_DEL     , XBMCK_DELETE },
  { AKEYCODE_CTRL_LEFT       , XBMCK_LCTRL },
  { AKEYCODE_CTRL_RIGHT      , XBMCK_RCTRL },
  { AKEYCODE_CAPS_LOCK       , XBMCK_CAPSLOCK },
  { AKEYCODE_SCROLL_LOCK     , XBMCK_SCROLLOCK },
  { AKEYCODE_INSERT          , XBMCK_INSERT },
  { AKEYCODE_FORWARD         , XBMCK_MEDIA_FASTFORWARD },
  { AKEYCODE_MEDIA_PLAY      , XBMCK_MEDIA_PLAY_PAUSE },
  { AKEYCODE_MEDIA_PAUSE     , XBMCK_MEDIA_PLAY_PAUSE },
  { AKEYCODE_MEDIA_RECORD    , XBMCK_RECORD },
  { AKEYCODE_MEDIA_EJECT     , XBMCK_EJECT },
  { AKEYCODE_GUIDE           , XBMCK_GUIDE },
  { AKEYCODE_SETTINGS        , XBMCK_SETTINGS },
  { AKEYCODE_INFO            , XBMCK_INFO },
  { AKEYCODE_PROG_RED        , XBMCK_RED },
  { AKEYCODE_PROG_GREEN      , XBMCK_GREEN },
  { AKEYCODE_PROG_YELLOW     , XBMCK_YELLOW },
  { AKEYCODE_PROG_BLUE       , XBMCK_BLUE },
};

bool CAndroidKey::onKeyboardEvent(AInputEvent *event)
{
  if (event == NULL)
    return false;

  int32_t flags   = AKeyEvent_getFlags(event);
  int32_t state   = AKeyEvent_getMetaState(event);
  int32_t action  = AKeyEvent_getAction(event);
  int32_t repeat  = AKeyEvent_getRepeatCount(event);
  int32_t keycode = AKeyEvent_getKeyCode(event);

  int32_t deviceId = AInputEvent_getDeviceId(event);
  CJNIKeyCharacterMap map = CJNIKeyCharacterMap::load(deviceId);
  uint16_t unicode = map.get(keycode, state);

  // Check if we got some special key
  uint16_t sym = XBMCK_UNKNOWN;
  for (unsigned int index = 0; index < sizeof(keyMap) / sizeof(KeyMap); index++)
  {
    if (keycode == keyMap[index].nativeKey)
    {
      sym = keyMap[index].xbmcKey;
      break;
    }
  }

  // check if this is a key we don't want to handle
  if (sym == XBMCK_LAST || sym == XBMCK_UNKNOWN)
  {
    CXBMCApp::android_printf("CAndroidKey: key ignored (code: %d)", keycode);
    return false;
  }

  uint16_t modifiers = 0;
  if (state & AMETA_ALT_LEFT_ON)
    modifiers |= XBMCKMOD_LALT;
  if (state & AMETA_ALT_RIGHT_ON)
    modifiers |= XBMCKMOD_RALT;
  if (state & AMETA_SHIFT_LEFT_ON)
    modifiers |= XBMCKMOD_LSHIFT;
  if (state & AMETA_SHIFT_RIGHT_ON)
    modifiers |= XBMCKMOD_RSHIFT;
  if (state & AMETA_CTRL_LEFT_ON)
    modifiers |= XBMCKMOD_LCTRL;
  if (state & AMETA_CTRL_RIGHT_ON)
    modifiers |= XBMCKMOD_RCTRL;
  /* TODO:
  if (state & AMETA_SYM_ON)
    modifiers |= 0x000?;*/

  switch (action)
  {
    case AKEY_EVENT_ACTION_DOWN:
#if 1
      CXBMCApp::android_printf("CAndroidKey: key down (code: %d; repeat: %d; flags: 0x%0X; alt: %s; shift: %s; sym: %s)",
        keycode, repeat, flags,
        (state & AMETA_ALT_ON) ? "yes" : "no",
        (state & AMETA_SHIFT_ON) ? "yes" : "no",
        (state & AMETA_SYM_ON) ? "yes" : "no");
#endif
      XBMC_Key((uint8_t)keycode, sym, modifiers, unicode, false);
      return true;

    case AKEY_EVENT_ACTION_UP:
#if 1
      CXBMCApp::android_printf("CAndroidKey: key up (code: %d; repeat: %d; flags: 0x%0X; alt: %s; shift: %s; sym: %s)",
        keycode, repeat, flags,
        (state & AMETA_ALT_ON) ? "yes" : "no",
        (state & AMETA_SHIFT_ON) ? "yes" : "no",
        (state & AMETA_SYM_ON) ? "yes" : "no");
#endif
      XBMC_Key((uint8_t)keycode, sym, modifiers, unicode, true);
      return true;

    case AKEY_EVENT_ACTION_MULTIPLE:
#if 1
      CXBMCApp::android_printf("CAndroidKey: key multiple (code: %d; repeat: %d; flags: 0x%0X; alt: %s; shift: %s; sym: %s)",
        keycode, repeat, flags,
        (state & AMETA_ALT_ON) ? "yes" : "no",
        (state & AMETA_SHIFT_ON) ? "yes" : "no",
        (state & AMETA_SYM_ON) ? "yes" : "no");
#endif
      break;

    default:
#if 1
      CXBMCApp::android_printf("CAndroidKey: unknown key (code: %d; repeat: %d; flags: 0x%0X; alt: %s; shift: %s; sym: %s)",
        keycode, repeat, flags,
        (state & AMETA_ALT_ON) ? "yes" : "no",
        (state & AMETA_SHIFT_ON) ? "yes" : "no",
        (state & AMETA_SYM_ON) ? "yes" : "no");
#endif
      break;
  }

  return false;
}

void CAndroidKey::XBMC_Key(uint8_t code, uint16_t key, uint16_t modifiers, uint16_t unicode, bool up)
{
  XBMC_Event newEvent;
  memset(&newEvent, 0, sizeof(newEvent));

  unsigned char type = up ? XBMC_KEYUP : XBMC_KEYDOWN;
  newEvent.type = type;
  newEvent.key.type = type;
  newEvent.key.keysym.scancode = code;
  newEvent.key.keysym.sym = (XBMCKey)key;
  newEvent.key.keysym.unicode = unicode;
  newEvent.key.keysym.mod = (XBMCMod)modifiers;

  //CXBMCApp::android_printf("XBMC_Key(%u, %u, 0x%04X, %d)", code, key, modifiers, up);
  CWinEvents::MessagePush(&newEvent);
}
