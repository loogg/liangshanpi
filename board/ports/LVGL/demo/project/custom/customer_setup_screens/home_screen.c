#include "home_screen.h"
#include <dfs_file.h>
#include <poll.h>
#include <unistd.h>

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

#define TTF_PATH "sdio/Cubic_11.ttf"

void _lv_example_tiny_ttf_2(lv_ui *ui)
{
    struct stat s;

    stat(TTF_PATH, &s); /* Get file size */
    uint8_t *buf = rt_malloc(s.st_size);
    int fd = open(TTF_PATH, O_RDONLY); /* Open file */
    read(fd, buf, s.st_size); /* Read file content */
    close(fd); /* Close file */


    /*Create style with the new font*/
    static lv_style_t style;
    lv_style_init(&style);
    lv_font_t * font = lv_tiny_ttf_create_data(buf, s.st_size, 30);
    lv_style_set_text_font(&style, font);
    lv_style_set_text_align(&style, LV_TEXT_ALIGN_CENTER);

    /*Create a label with the new style*/
    lv_obj_t * label = lv_label_create(ui->Home);
    lv_obj_set_width(label, LV_PCT(100));
    lv_obj_add_style(label, &style, 0);
    lv_label_set_text(label, "我是一段测试文字，测试字体是否正常显示。");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
}

void home_screen_custom_setup(lv_ui *ui) {
    // _music_btn = lv_btn_create(ui->Home);
    // lv_obj_align(_music_btn, LV_ALIGN_CENTER, 0, 0);
    // lv_label_set_text(lv_label_create(_music_btn), LV_SYMBOL_AUDIO);

    _lv_example_tiny_ttf_2(ui);

    // lv_obj_t * img;
    // img = lv_gif_create(ui->Home);
    // /* Assuming a File system is attached to letter 'A'
    //  * E.g. set LV_USE_FS_STDIO 'A' in lv_conf.h */
    // lv_gif_set_src(img, "S:/sdio/bulb.gif");
    // lv_obj_align(img, LV_ALIGN_RIGHT_MID, -20, 0);

    // lv_obj_add_event_cb(_music_btn, _screen_btn_event_cb, LV_EVENT_ALL, ui);
}
