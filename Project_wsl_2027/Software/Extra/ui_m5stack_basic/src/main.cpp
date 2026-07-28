#include <Arduino.h>

#include "ui.hpp"
#include "button.hpp"
Button btnA;
Button btnB;
Button btnC;
// AnalogController(12bit ADC)
const uint8_t analog_controller_x_pin = 36;
const uint8_t analog_controller_y_pin = 35;

Timer analog_controller_update_timer;
mouse_t mouse;

void setup()
{
    Serial.begin(9600);

    pinMode(analog_controller_x_pin, INPUT);
    pinMode(analog_controller_y_pin, INPUT);

    btnA.init(39, INPUT_PULLUP); // ボタンA
    btnB.init(38, INPUT_PULLUP); // ボタンB
    btnC.init(37, INPUT_PULLUP); // ボタンC

    uiInit();

    mouse.posi.x = uiWidth() / 2;
    mouse.posi.y = uiHeight() / 2;
    mouse.pushing = false;
}

void loop()
{
    btnA.update();
    btnB.update();
    btnC.update();

    if (analog_controller_update_timer.everReset() || analog_controller_update_timer.msTime() >= 10)
    {
        int xx = - analogRead(analog_controller_x_pin) + 0xFFF / 2;
        if (abs(xx) > 100)
        {
            mouse.posi.x += (xx > 0) ? 2 : -2;
            mouse.posi.x = constrain(mouse.posi.x, 0, uiWidth() - 1);
        }

        int yy = analogRead(analog_controller_y_pin) - 0xFFF / 2;
        if (abs(yy) > 100)
        {
            mouse.posi.y += (yy > 0) ? 2 : -2;
            mouse.posi.y = constrain(mouse.posi.y, 0, uiHeight() - 1);
        }

        analog_controller_update_timer.reset();
    }

    mouse.pushing = btnA.isPushing();

    uiUpdate(mouse);

    delay(1);
}