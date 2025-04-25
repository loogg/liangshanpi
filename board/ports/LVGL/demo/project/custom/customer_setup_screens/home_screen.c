#include "home_screen.h"

static lv_obj_t *_music_btn = NULL;

static void _screen_btn_event_cb(lv_event_t *e) {
    lv_ui *ui = (lv_ui *)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);

    if (code != LV_EVENT_CLICKED) return;

    if (btn == _music_btn) {
        ui_load_scr_animation(ui, &ui->AudioList, ui->AudioList_del, &ui->Home_del, setup_scr_AudioList, LV_SCR_LOAD_ANIM_NONE, 0, 0, false, true);
    }
}

void home_screen_custom_setup(lv_ui *ui) {
    _music_btn = lv_btn_create(ui->Home);
    lv_obj_align(_music_btn, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(lv_label_create(_music_btn), LV_SYMBOL_AUDIO);



    lv_obj_t * img;
    img = lv_gif_create(ui->Home);
    /* Assuming a File system is attached to letter 'A'
     * E.g. set LV_USE_FS_STDIO 'A' in lv_conf.h */
    lv_gif_set_src(img, "S:/sdio/bulb.gif");
    lv_obj_align(img, LV_ALIGN_RIGHT_MID, -20, 0);

    lv_obj_add_event_cb(_music_btn, _screen_btn_event_cb, LV_EVENT_ALL, ui);
}
