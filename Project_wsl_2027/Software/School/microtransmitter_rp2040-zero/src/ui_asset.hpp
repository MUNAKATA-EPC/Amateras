#pragma once

#include "ui.hpp"

ui::button small_alphabet_btns[3][10] = {
    {ui::button(0, 0, 12, 12, "a"), ui::button(12, 0, 12, 12, "b"), ui::button(24, 0, 12, 12, "c"), ui::button(36, 0, 12, 12, "d"), ui::button(48, 0, 12, 12, "e"),
     ui::button(60, 0, 12, 12, "f"), ui::button(72, 0, 12, 12, "g"), ui::button(84, 0, 12, 12, "h"), ui::button(96, 0, 12, 12, "i"), ui::button(108, 0, 12, 12, "j")},
    {ui::button(0, 12, 12, 12, "k"), ui::button(12, 12, 12, 12, "l"), ui::button(24, 12, 12, 12, "m"), ui::button(36, 12, 12, 12, "n"), ui::button(48, 12, 12, 12, "o"),
     ui::button(60, 12, 12, 12, "p"), ui::button(72, 12, 12, 12, "q"), ui::button(84, 12, 12, 12, "r"), ui::button(96, 12, 12, 12, "s"), ui::button(108, 12, 12, 12, "t")},
    {ui::button(0, 24, 12, 12, "u"), ui::button(12, 24, 12, 12, "v"), ui::button(24, 24, 12, 12, "w"), ui::button(36, 24, 12, 12, "h"), ui::button(48, 24, 12, 12, "y"),
     ui::button(60, 24, 12, 12, "z"), ui::button(), ui::button(), ui::button(), ui::button()},
};

ui::button_container small_alphabet_board(small_alphabet_btns);