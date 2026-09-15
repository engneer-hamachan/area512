/*----------------------------------------------------------------------------/
  Lovyan GFX - Graphics library for embedded devices.

Original Source:
 https://github.com/lovyan03/LovyanGFX/

Licence:
 [FreeBSD](https://github.com/lovyan03/LovyanGFX/blob/master/license.txt)

Author:
 [lovyan03](https://twitter.com/lovyan03)

Contributors:
 [ciniml](https://github.com/ciniml)
 [mongonta0716](https://github.com/mongonta0716)
 [tobozo](https://github.com/tobozo)
/----------------------------------------------------------------------------*/
#pragma once

#include <lgfx/v1/panel/Panel_LCD.hpp>

namespace lgfx {
inline namespace v1 {

struct Panel_ILI9341 : public Panel_LCD {
protected:
  static constexpr uint8_t CMD_FRMCTR1 = 0xB1;
  static constexpr uint8_t CMD_DFUNCTR = 0xB6;
  static constexpr uint8_t CMD_PWCTR1 = 0xC0;
  static constexpr uint8_t CMD_PWCTR2 = 0xC1;
  static constexpr uint8_t CMD_VMCTR1 = 0xC5;
  static constexpr uint8_t CMD_VMCTR2 = 0xC7;
  static constexpr uint8_t CMD_GMCTRP1 = 0xE0;
  static constexpr uint8_t CMD_GMCTRN1 = 0xE1;

  const uint8_t *getInitCommands(uint8_t listno) const override {
    static constexpr uint8_t list0[] = {
      0xEF, 3, 0x03, 0x80, 0x02,
      0xCF, 3, 0x00, 0xC1, 0x30,
      0xED, 4, 0x64, 0x03, 0x12, 0x81,
      0xE8, 3, 0x85, 0x00, 0x78,
      0xCB, 5, 0x39, 0x2C, 0x00, 0x34, 0x02,
      0xF7, 1, 0x20,
      0xEA, 2, 0x00, 0x00,
      CMD_PWCTR1, 1, 0x23,
      CMD_PWCTR2, 1, 0x10,
      CMD_VMCTR1, 2, 0x3e, 0x28,
      CMD_VMCTR2, 1, 0x86,
      CMD_FRMCTR1, 2, 0x00, 0x13,
      0xF2, 1, 0x00,
      CMD_GAMMASET, 1, 0x01,
      CMD_GMCTRP1, 15, 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00,
      CMD_GMCTRN1, 15, 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F,
      CMD_DFUNCTR, 3, 0x08, 0xC2, 0x27,
      CMD_SLPOUT, 0 + CMD_INIT_DELAY, 120,
      CMD_IDMOFF, 0,
      CMD_DISPON, 0 + CMD_INIT_DELAY, 100,
      0xFF, 0xFF,
    };
    switch (listno) {
    case 0:
      return list0;
    default:
      return nullptr;
    }
  }
};

} // namespace v1
} // namespace lgfx
