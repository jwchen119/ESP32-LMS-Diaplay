// This file was generated by SquareLine Studio
// SquareLine Studio version: SquareLine Studio 1.3.2
// LVGL version: 8.3.6
// Project name: SquareLine_Project

#include "../ui.h"

void ui_Screen2_screen_init(void)
{
ui_Screen2 = lv_obj_create(NULL);
lv_obj_clear_flag( ui_Screen2, LV_OBJ_FLAG_SCROLLABLE );    /// Flags
lv_obj_set_style_bg_color(ui_Screen2, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT );
lv_obj_set_style_bg_opa(ui_Screen2, 255, LV_PART_MAIN| LV_STATE_DEFAULT);

ui_Label6 = lv_label_create(ui_Screen2);
lv_obj_set_width( ui_Label6, LV_SIZE_CONTENT);  /// 1
lv_obj_set_height( ui_Label6, LV_SIZE_CONTENT);   /// 1
lv_obj_set_x( ui_Label6, 0 );
lv_obj_set_y( ui_Label6, -74 );
lv_obj_set_align( ui_Label6, LV_ALIGN_CENTER );
lv_label_set_text(ui_Label6,"Setting Wi-Fi");
lv_obj_set_style_text_font(ui_Label6, &ui_font_fontHEI18, LV_PART_MAIN| LV_STATE_DEFAULT);

ui_Label7 = lv_label_create(ui_Screen2);
lv_obj_set_width( ui_Label7, LV_SIZE_CONTENT);  /// 1
lv_obj_set_height( ui_Label7, LV_SIZE_CONTENT);   /// 1
lv_obj_set_x( ui_Label7, 31 );
lv_obj_set_y( ui_Label7, 76 );
lv_label_set_text(ui_Label7,"SSID :");
lv_obj_set_style_text_font(ui_Label7, &ui_font_fontHEI14, LV_PART_MAIN| LV_STATE_DEFAULT);

ui_Label8 = lv_label_create(ui_Screen2);
lv_obj_set_width( ui_Label8, LV_SIZE_CONTENT);  /// 1
lv_obj_set_height( ui_Label8, LV_SIZE_CONTENT);   /// 1
lv_obj_set_x( ui_Label8, 40 );
lv_obj_set_y( ui_Label8, 99 );
lv_label_set_text(ui_Label8,"PW :");
lv_obj_set_style_text_font(ui_Label8, &ui_font_fontHEI14, LV_PART_MAIN| LV_STATE_DEFAULT);

ui_Label9 = lv_label_create(ui_Screen2);
lv_obj_set_width( ui_Label9, 150);
lv_obj_set_height( ui_Label9, LV_SIZE_CONTENT);   /// 50
lv_obj_set_x( ui_Label9, 77 );
lv_obj_set_y( ui_Label9, 99 );
lv_label_set_text(ui_Label9,"PASSWORD");
lv_obj_set_style_text_align(ui_Label9, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN| LV_STATE_DEFAULT);
lv_obj_set_style_text_font(ui_Label9, &ui_font_fontHEI14, LV_PART_MAIN| LV_STATE_DEFAULT);

ui_Label10 = lv_label_create(ui_Screen2);
lv_obj_set_width( ui_Label10, 150);
lv_obj_set_height( ui_Label10, LV_SIZE_CONTENT);   /// 1
lv_obj_set_x( ui_Label10, 77 );
lv_obj_set_y( ui_Label10, 76 );
lv_label_set_text(ui_Label10,"SSID");
lv_obj_set_style_text_align(ui_Label10, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN| LV_STATE_DEFAULT);
lv_obj_set_style_text_font(ui_Label10, &ui_font_fontHEI14, LV_PART_MAIN| LV_STATE_DEFAULT);

ui_Label11 = lv_label_create(ui_Screen2);
lv_obj_set_width( ui_Label11, LV_SIZE_CONTENT);  /// 1
lv_obj_set_height( ui_Label11, LV_SIZE_CONTENT);   /// 1
lv_obj_set_x( ui_Label11, 50 );
lv_obj_set_y( ui_Label11, 122 );
lv_label_set_text(ui_Label11,"IP :");
lv_obj_set_style_text_font(ui_Label11, &ui_font_fontHEI14, LV_PART_MAIN| LV_STATE_DEFAULT);

ui_Label12 = lv_label_create(ui_Screen2);
lv_obj_set_width( ui_Label12, LV_SIZE_CONTENT);  /// 1
lv_obj_set_height( ui_Label12, LV_SIZE_CONTENT);   /// 1
lv_obj_set_x( ui_Label12, 77 );
lv_obj_set_y( ui_Label12, 122 );
lv_label_set_text(ui_Label12,"IP_ADDR");
lv_obj_set_style_text_font(ui_Label12, &ui_font_fontHEI14, LV_PART_MAIN| LV_STATE_DEFAULT);

}
