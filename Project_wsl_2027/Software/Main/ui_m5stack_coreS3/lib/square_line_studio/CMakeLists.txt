SET(SOURCES ui_HomeScreen.c
    ui_Action.c
    ui_Test.c
    ui_SensorMonitor.c
    ui_Communication.c
    ui.c
    ui_comp_hook.c
    ui_helpers.c
    ui_events.c)

add_library(ui ${SOURCES})
