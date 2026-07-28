#include "ui.hpp"

static LGFX _tft;
static LGFX_Sprite _canvas(&_tft);

// 色定数
const uint32_t _black_color = 0x000000;
const uint32_t _white_color = 0xFFFFFF;
const uint32_t _gray_color = 0x555555;
const uint32_t _red_color = 0xFF0000;
const uint32_t _pink_color = 0xE74D4D;
const uint32_t _blue_color = 0x4472C4;
const uint32_t _light_blue_color = 0xADC1FF;
const uint32_t _yellow_color = 0xEBEB40;
const uint32_t _green_color = 0x70AD47;

// 画面状態
enum class ui_screen_t
{
    HOMESCREEN,
    MONITORSCREEN,
    TESTSCREEN
};
static ui_screen_t _current_screen = ui_screen_t::HOMESCREEN;

// マウス状態
static mouse_t _current_mouse;

// ホームスクリーン
namespace _homescreen
{
    static uiToggle runToggle(5, 5, 60, 95, _gray_color, _black_color, _red_color, "ON", _white_color, _blue_color, "OFF", _white_color, false, 5);

    const uint32_t goalBtnBgColors[] = {_yellow_color, _blue_color, _gray_color};
    const char *goalBtnTexts[] = {"YELLOW", "BLUE", "GYRO"};
    const uint32_t goalBtnTextColors[] = {_black_color, _black_color, _white_color};
    static uiCycleButton goalCycleBtn(5, 105, 60, 80, 3, goalBtnBgColors, goalBtnTexts, goalBtnTextColors);

    static uiToggle showToggle(255, 5, 60, 95, _gray_color, _black_color, _red_color, "SHOW", _white_color, _blue_color, "HIDE", _white_color, true, 5);

    const uint32_t posBtnBgColors[] = {_green_color, _gray_color};
    const char *posBtnTexts[] = {"POS", "ODA"};
    const uint32_t posBtnTextColors[] = {_white_color, _white_color};
    static uiCycleButton posCycleBtn(255, 105, 60, 80, 2, posBtnBgColors, posBtnTexts, posBtnTextColors);

    const uint32_t actionmodeBtnBgColors[] = {_pink_color, _pink_color, _pink_color};
    const char *actionmodeBtnTexts[] = {"ATK", "DEF", "RADICON"};
    const uint32_t actionmodeBtnTextColors[] = {_white_color, _white_color, _white_color};
    static uiCycleButton actionmodeBtn(70, 5, 180, 180, 3, actionmodeBtnBgColors, actionmodeBtnTexts, actionmodeBtnTextColors);

    static uiFlexibleImageButton monitorBtn(5, 195, 5, 195, 120, 40, _gray_color, 20, 0, "MONITOR", _white_color, 40, 40, "/ui_monitor_icon.png");
    static uiFlexibleImageButton testBtn(130, 195, 130, 195, 120, 40, _gray_color, 20, 0, "TEST", _white_color, 40, 40, "/ui_test_icon.png");
    static uiButton nextBtn(255, 195, 60, 40, _gray_color, "NEXT", _white_color);

    void init()
    {
        monitorBtn.init();
        testBtn.init();
    }

    void main()
    {
        // 背景画像描画
        _canvas.fillScreen(TFT_WHITE);                   // 背景
        _canvas.fillRect(0, 190, 320, 50, _black_color); // サイドバー

        // コントロールオブジェクト更新
        runToggle.update(_current_mouse);
        goalCycleBtn.update(_current_mouse);
        showToggle.update(_current_mouse);
        posCycleBtn.update(_current_mouse);
        actionmodeBtn.update(_current_mouse);
        monitorBtn.update(_current_mouse);
        testBtn.update(_current_mouse);
        nextBtn.update(_current_mouse);

        // コントロールオブジェクト描画
        runToggle.draw(_canvas);
        goalCycleBtn.draw(_canvas);
        showToggle.draw(_canvas);
        posCycleBtn.draw(_canvas);
        actionmodeBtn.draw(_canvas);
        monitorBtn.draw(_canvas);
        testBtn.draw(_canvas);
        nextBtn.draw(_canvas);

        // ボタンが押されたときの処理
        if (monitorBtn.isReleasedMoment())
        {
            _current_screen = ui_screen_t::MONITORSCREEN;
        }
        else if (testBtn.isReleasedMoment())
        {
            _current_screen = ui_screen_t::TESTSCREEN;
        }
        else if (nextBtn.isReleasedMoment())
        {
        }
    }
}

// モニタースクリーン
namespace _monitorscreen
{
    static uiImageButton iconBtn(5, 5, 40, 40, 40, 40, "/ui_monitor_icon.png");
    const uint32_t sensorBtnBgColors[] = {_gray_color, _yellow_color, _blue_color, _gray_color};
    const char *sensorBtnTexts[] = {"GYRO", "YELLOW_CAM", "BLUE_CAM", "LIDAR"};
    const uint32_t sensorBtnTextColors[] = {_white_color, _black_color, _black_color, _white_color};
    static uiCycleButton sensorBtn(50, 5, 220, 40, 4, sensorBtnBgColors, sensorBtnTexts, sensorBtnTextColors, uiCycleSplitMode::Horizontal);
    static uiButton closeBtn(275, 5, 40, 40, _red_color, "CLOSE", _white_color);

    void init()
    {
        iconBtn.init();
    }

    void main()
    {
        _canvas.fillScreen(TFT_WHITE);                 // 背景
        _canvas.fillRect(0, 0, 320, 50, _black_color); // サイドバー

        // コントロールオブジェクト更新
        iconBtn.update(_current_mouse);
        sensorBtn.update(_current_mouse);
        closeBtn.update(_current_mouse);

        // コントロールオブジェクト描画
        iconBtn.draw(_canvas);
        sensorBtn.draw(_canvas);
        closeBtn.draw(_canvas);

        if (iconBtn.isReleasedMoment())
        {
            _current_screen = ui_screen_t::TESTSCREEN;
        }

        if (closeBtn.isReleasedMoment())
        {
            _current_screen = ui_screen_t::HOMESCREEN;
        }
    }
}

// テストスクリーン
namespace _testscreen
{
    static uiImageButton iconBtn(5, 5, 40, 40, 40, 40, "/ui_test_icon.png");
    const uint32_t testBtnBgColors[] = {_gray_color, _gray_color, _gray_color, _gray_color};
    const char *testBtnTexts[] = {"KICKER", "MOTOR", "DRIBBLER", "GYRO AND CAM"};
    const uint32_t testBtnTextColors[] = {_white_color, _white_color, _white_color, _white_color};
    static uiCycleButton testBtn(50, 5, 220, 40, 4, testBtnBgColors, testBtnTexts, testBtnTextColors, uiCycleSplitMode::Horizontal);
    static uiButton closeBtn(275, 5, 40, 40, _red_color, "CLOSE", _white_color);

    void init()
    {
        iconBtn.init();
    }

    void main()
    {
        _canvas.fillScreen(TFT_WHITE);                 // 背景
        _canvas.fillRect(0, 0, 320, 50, _black_color); // サイドバー

        // コントロールオブジェクト更新
        iconBtn.update(_current_mouse);
        testBtn.update(_current_mouse);
        closeBtn.update(_current_mouse);

        // コントロールオブジェクト描画
        iconBtn.draw(_canvas);
        testBtn.draw(_canvas);
        closeBtn.draw(_canvas);

        if (iconBtn.isReleasedMoment())
        {
            _current_screen = ui_screen_t::MONITORSCREEN;
        }

        if (closeBtn.isReleasedMoment())
        {
            _current_screen = ui_screen_t::HOMESCREEN;
        }
    }
}

// 初期化
bool uiInit()
{
    _tft.init();
    _tft.setRotation(1);
    _tft.setBrightness(128);

    if (!SPIFFS.begin(true))
    {
        _tft.println("SPIFFS Mount Failed!");
        return false;
    }

    _tft.fillScreen(TFT_BLACK);

    _canvas.setColorDepth(8);
    _canvas.createSprite(_tft.width(), _tft.height());

    _homescreen::init();
    _monitorscreen::init();
    _testscreen::init();

    return true;
}

// 更新
void uiUpdate(mouse_t mouse)
{
    _current_mouse = mouse;

    // スクリーン描画
    switch (_current_screen)
    {
    case ui_screen_t::HOMESCREEN:
        _homescreen::main();
        break;
    case ui_screen_t::MONITORSCREEN:
        _monitorscreen::main();
        break;
    case ui_screen_t::TESTSCREEN:
        _testscreen::main();
        break;
    }

    // マウス描画
    _canvas.fillCircle(_current_mouse.posi.x, _current_mouse.posi.y, 3, TFT_BLACK);

    // 描画更新
    _canvas.pushSprite(0, 0);
}

// ゲッター
int uiWidth() { return _tft.width(); }
int uiHeight() { return _tft.height(); }