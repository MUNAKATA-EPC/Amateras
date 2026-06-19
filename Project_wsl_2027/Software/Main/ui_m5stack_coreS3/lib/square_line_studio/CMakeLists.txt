SET(SOURCES ui_HomeScreen.c
    ui_ActionSelectAppScreen.c
    ui_TestAppScreen.c
    ui_SensorCheckAppScreen.c
    ui_MessageAppScreen.c
    ui.c
    ui_comp_hook.c
    ui_helpers.c
    ui_events.cpp)

add_library(ui ${SOURCES})
