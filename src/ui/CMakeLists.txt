SET(SOURCES screens/ui_Screen2.c
    screens/ui_Screen1.c
    screens/ui_Screen3.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c
    images/ui_img_1968846774.c
    images/ui_img_1073469376.c
    fonts/ui_font_fontHEI14.c
    fonts/ui_font_fontHEI18.c)

add_library(ui ${SOURCES})
