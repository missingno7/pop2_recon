#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <dos.h>
#include <io.h>
#include "types.h"
#include "system.h"
#include "resc.h"
#include "throne.h"
#include "snd.h"
#include "graf.h"
#include "shape.h"
#include "level.h"
#include "heap.h"
#include "char.h"
#include "game.h"
#include "draw.h"
#include "custom.h"
#include "add.h"
#include "checknis.h"
#include "ctrl.h"
#include "dissolve.h"
#include "dlghelp.h"
#include "dialog.h"
#include "english.h"
#include "gfxerr.h"
#include "fade.h"
#include "anim.h"
#include "hnewshpi.h"
#include "init.h"
#include "intrface.h"
#include "lgraf.h"
#include "load.h"
#include "move.h"
#include "nis.h"
#include "xlatgraf.h"
#include "prince.h"
#include "rooftops.h"
#include "sound.h"
#include "top.h"
#include "util.h"


static Rect save_heading = {14, 16, 37, 304};
static Rect save_description = {167, 16, 183, 304};

static const POINT symbol_heading = {28, 47};
static const POINT symbol_caption = {44, 72};
static Rect notice_port[1] = {{32, 73, 50, 266}};
int notice_state = 0;
static const POINT hall_name_caption = {53, 25};
static const POINT hall_score_caption = {53, 260};
static Rect hall_box = {38, 20, 57, 300};
static const char save_path[12] = "PRINCE.SAV";
static const char hall_read_path[12] = "PRINCE.HOF";
static const char options_path[12] = "PRINCE.OPT";
static const int credits_first[] = {1000, 1007, 1009, 1011, 1012};
static const int credits_last[] = {1006, 1008, 1010, 1011, 1012};
static const Rect credits_blink_box = {70, 17, 85, 307};
struct notice_and_menu_text {
    char notice_failure[25];
    char hall_write_path[11];
    char save_option[10];
    char resume_option[12];
    char continue_option[10];
};
static const struct notice_and_menu_text dialog_texts = {
    "Copy protection failure.", "PRINCE.HOF", "Save Game", "Resume Game", "Quit Game"
};
#define notice_failure dialog_texts.notice_failure
#define hall_write_path dialog_texts.hall_write_path
#define save_option dialog_texts.save_option
#define resume_option dialog_texts.resume_option
#define continue_option dialog_texts.continue_option


void draw_dialog_text_block(EDITFIELD far *block)
{
    POINT range;
    int style;

    style = Graf->background == 12 ? 0 : 1;
    range.x = -1;
    range.y = 0;
    DialogDrawStr(block->text, 10, style, 0, &block->clip, &range);
    hFree(hGetFont(10));
}

void DialogDrawNumber(int number, const POINT *point,
                                 const Rect far *rect,
                                 const POINT *range)
{
    char text[10];

    itoa(number, text, 10);
    DialogDrawStr(text, 10, 0, point, rect, range);
    hFree(hGetFont(10));
}

void DialogDrawStr(const char far *text, int font, int style,
                              const POINT *point,
                              const Rect far *rect,
                              const POINT *bounds)
{
    Rect shadow_rect;
    POINT range;
    int ink, shadow, first_ink;

    if (style == 0) {
        first_ink = 0;
        ink = 15;
        shadow = 14;
    } else {
        first_ink = 2;
        ink = 4;
        shadow = 13;
    }
    if (bounds != 0)
        range = *bounds;
    else {
        range.x = -1;
        range.y = 0;
    }

    SetFont(font);
    Graf->foreground = first_ink;
    if (rect) {
        OffsetRect(&shadow_rect, rect, 1, 0);
        StringRect(&shadow_rect, range.x, range.y, text);
    } else {
        MoveTo(point->x + 1, point->y);
        DrawString(text);
    }

    Graf->foreground = shadow;
    if (rect)
        StringRect(rect, range.x, range.y, text);
    else {
        MoveTo(point->x, point->y);
        DrawString(text);
    }

    SetFont(font + 1);
    Graf->foreground = ink;
    if (rect)
        StringRect(rect, range.x, range.y, text);
    else {
        MoveTo(point->x, point->y);
        DrawString(text);
    }
}

void DialogDrawStrId(int string_id, int font, int style,
                                const POINT *point,
                                const Rect far *rect,
                                const POINT *range)
{
    Handle handle;

    handle = hGetTxt4(string_id);
    if (handle) {
        DialogDrawStr(hLock(handle), font, style, point, rect, range);
        hFree(handle);
    }
}


int DialogFadeIn(void)
{
    Handle handle;
    int result;

    ReadDACBlock(0, 32, h2p(handle = hNew(96UL)));
    SetDACBlock(0, 32, 0, 1);
    CopyBits(&ScreenPort, BPort, &ScreenPortRect, &ScreenPortRect, 0);
    result = FadeTo(handle, 3, 0);
    if (result)
        SetDACBlock(0, 32, h2p(handle), 1);
    hFree(handle);
    return result;
}

int DialogFadeOut(void)
{
    int result;

    result = FadeTo(0, 3, 0);
    EraseRect(&ScreenPortRect);
    return result;
}


void DialogResetToGamePlay(int restart_input)
{
    if (BPort)
        DeallocBackPort(BPort);
    BPort = AllocBackPort(&GamePlayRect);
    Graf = &ScreenPort;
    SetFont(0);
    Graf->background = 0;
    Graf->foreground = 15;
    EraseRect(&ScreenPortRect);
    ResetStdPalette();
    hFree(hGetFont(11));
    hFree(hGetFont(12));
    hFree(hGetFont(13));
    if (restart_input && !nis_skip_requested) {
        ForceOppRedraw();
        PrepCut(0);
        RedrawFlag = 1;
    }
    RestoreGamePlayPalette();
    SetKidPalette(Level.environment_kind);
    WaitFunc = Input;
}

int DialogWaitFunc(void)
{
    int key;

    key = read_queued_key();
    if (key) {
        if (key == 0x1000 || key == 17)
            abort_with_message(0, 0);
        else if (key != 27 && key != 32)
            key = 0;
    } else if (GameState.control_mode == 2) {
        poll_game_port();
        key = Joy.button;
    }
    return key;
}


struct frame_bitmap {
    int width, stride, height;
    char pixels[1];
};

void DrawDialogFrame(Handle asset)
{
    Rect border;
    int temporary;
    Handle handle;
    struct frame_bitmap far *bitmap;

    temporary = asset == 0;
    if (temporary)
        asset = load_frame_asset();
    PaintRLEShapeItem(asset, 1, 0, 0, 0);
    PaintRLEShapeItem(asset, 2, 0, 192, 0);
    bitmap = (struct frame_bitmap far *)h2p(handle = hNewShpItem1(asset, 3, 1));
    PaintRLEShape_asm(bitmap, 0, 8, 0);
    MirrorRLEBlock(bitmap->pixels, bitmap->width, bitmap->height);
    PaintRLEShape_asm(bitmap, 312, 8, 0);
    hFree(handle);
    InsetRect(&border, &ScreenPortRect, 9, 9);
    FillRect(&border, 12);
    if (temporary)
        hFreeShpList(asset);
}

void InitDialog(void)
{
    ResetSnd(0);
    SaveGamePlayPalette();
    WaitFunc = DialogWaitFunc;
    Graf = &ScreenPort;
    ClrLifeBar();
    SetFont(11);
    Graf->background = 14;
    DeallocBackPort(BPort);
    BPort = AllocBackPort(&ScreenPortRect);
    Graf = (PORT *)BPort;
    SetFont(11);
    EraseRect(&ScreenPortRect);
}

int IsValidTimeForDialog(void)
{
    int result = 1;

    if ((!Minutes && !SixtiethsCtr) || MobCt)
        result = 0;
    return result;
}

Handle load_frame_asset(void)
{
    return hGetNMapShpList(8000, 1, 1);
}


int ResumeGameDialog(void)
{
    struct save_slot_table far *slots;
    int selection = 0;
    int loaded;

    InitDialog();
    slots = InitSavedGameStruct();
    if (slots) {
        DrawDialogFrame(0);
        draw_save_slots(slots, 0);
        Graf = &ScreenPort;
        DialogFadeIn();
        DeallocBackPort(BPort);
        BPort = 0;
        selection = GetSaveGameLocation(slots, 0);
        if (selection != -1) {
            loaded = ResumeGame(selection);
            if (loaded) {
                StartLevel = LevelIdx;
                NextLevel = LevelIdx;
                RestartLevel = 1;
            }
            selection = loaded;
        } else {
            selection = -1;
        }
        clear_payload_header(slots);
        DialogFadeOut();
    }
    DialogResetToGamePlay(selection == -1 ? 1 : 0);
    if (selection > 0)
        CopyProtectionDialog();
    if (selection != -1 && BPort) {
        DeallocBackPort(BPort);
        BPort = 0;
    }
    return selection;
}

int SaveGameDialog(void)
{
    struct save_slot_table far *slots;
    int selection = 0;
    int chosen;

    InitDialog();
    DrawDialogFrame(0);
    slots = InitSavedGameStruct();
    if (slots) {
        draw_save_slots(slots, 1);
        Graf = &ScreenPort;
        DialogFadeIn();
        DeallocBackPort(BPort);
        BPort = 0;
        chosen = GetSaveGameLocation(slots, 1);
        if (chosen != -1)
            selection = SaveGame(slots, chosen);
        else
            selection = -1;
        clear_payload_header(slots);
        DialogFadeOut();
    }
    DialogResetToGamePlay(1);
    DrawSaveGameMsg(selection);
    return selection;
}

struct resource_shape {
    int width, height, attributes;
};

void main_menu_dialog(void)
{
    int selection;

    EraseRect(&ScreenPortRect);
    UnloadCustomRoomData();
    InitDialog();
    DrawDialogFrame(0);
    ((struct resource_shape far *)h2p(hGetShpList(1000)))->attributes = 0;
    hGetNMapShpList(1000, 2, 1);
    draw_main_menu();
    Graf = &ScreenPort;
    DialogFadeIn();
    DeallocBackPort(BPort);
    BPort = 0;
    selection = main_menu_choice();
    DialogFadeOut();
    hFree(hGetShpList(1000));
    DialogResetToGamePlay(0);
    if (selection == 3)
        SaveGameDialog();
    else if (selection == 4)
        ResumeGameDialog();
    SaveOptionsToDisk();
    SetKidPalette(Level.environment_kind);
    RedrawFlag = 1;
}

Rect far * compute_save_slot_rect(Rect far *rect, int index)
{
    if (index >= 5) {
        rect->left = 163;
        index -= 5;
    } else {
        rect->left = 19;
    }
    rect->right = rect->left + 137;
    rect->top = index * 22 + 37;
    rect->bottom = rect->top + 20;
    return rect;
}


void draw_save_slot(struct save_slot_table far *slots, int index, int selected)
{
    Rect rect;
    int style;

    compute_save_slot_rect(&rect, index);
    draw_save_slot_frame(&rect, selected);
    style = selected != 0;
    if (_fstrlen(slots->initials[index])) {
        InsetRect(&rect, &rect, 2, 2);
        DialogDrawStr(slots->initials[index], 10, style, 0, &rect, 0);
    }
}

void draw_save_slot_frame(Rect *rect, int selected)
{
    int fill, edge;

    if (selected) {
        fill = 3;
        edge = 14;
    } else {
        fill = 6;
        edge = 11;
    }
    FillRect(rect, edge);
    Graf->foreground = fill;
    FrameRect(rect);
}

void draw_save_slots(struct save_slot_table far *slots, int saving)
{
    int title, action, i;
    Handle light_font, dark_font;
    POINT range;

    light_font = hGetFont(12);
    dark_font = hGetFont(13);
    range.y = range.x = 0;
    if (!saving) {
        title = 8004;
        action = 8013;
    } else {
        title = 8000;
        action = 8003;
    }
    DialogDrawStrId(title, 12, 0, 0, &save_heading, &range);
    hFree(light_font);
    hFree(dark_font);
    light_font = hGetFont(10);
    hGetFont(11);
    OffsetRect(&save_description, &save_description, 0, -18);
    DialogDrawStrId(8001, 10, 0, 0, &save_description, &range);
    OffsetRect(&save_description, &save_description, 0, 18); range.x = -1; DialogDrawStrId(action, 10, 0, 0, &save_description, &range);
    range.x = 1; DialogDrawStrId(8002, 10, 0, 0, &save_description, &range);
    for (i = 0; i < 10; ++i) {
        draw_save_slot(slots, i, 0);
    }
    hFree(light_font);
}

void DrawSaveGameMsg(int selection)
{
    if (selection != -1) {
        if (selection)
            DrawMessage(saved_message);
        else {
            DrawMessage(nosave_message);
            play_song(-1);
        }
        MsgTimer = MsgTimerStart = 24;
    }
}


int GetSaveGameLocation(struct save_slot_table far *slots, int saving)
{
    int result, done, slot, previous, key;
    EDITFIELD far *editor;
    Rect rect;
    char initials[26];

    result = previous = -1;
    done = slot = 0;
    editor = 0;

    do {
        if (previous != slot) {
            if (previous != -1) {
                draw_save_slot(slots, previous, 0);
                if (editor)
                    clear_flags_of_far_block(editor);
            }
            previous = slot;
            draw_save_slot(slots, slot, 1);
            if (saving == 1) {
                compute_save_slot_rect(&rect, slot);
                InsetRect(&rect, &rect, 2, 2);
                _fstrcpy(initials, slots->initials[slot]);
                editor = create_edit_field(&rect, &rect, initials, 24);
                editor->paint = draw_dialog_text_block;
                show_edit_caret(editor);
            }
        }
        if (editor)
            blink_edit_caret(editor);
        key = read_queued_key();
        switch (key) {
        case 9:
            previous = slot;
            if (slot != 9)
                ++slot;
            else
                slot = 0;
            break;
        case 13:
            if ((saving == 0 && !_fstrlen(slots->initials[slot])) ||
                (saving == 1 && !_fstrlen(initials))) {
                SetSnd(-2);
                break;
            }
            if (editor)
                _fstrcpy(slots->initials[slot], initials);
            result = slot;
            /* fall through: Enter and Escape both leave the loop */
        case 27:
            done = 1;
            break;
        case 17:
        case 0x1000:
            abort_with_message(0, 0);
            break;
        default:
            if (editor) {
                if (key != '>' && key != '<')
                    handle_edit_key(editor, key);
            } else {
                switch (key) {
                case 0x4700:
                    slot = 0;
                    break;
                case 0x4800:
                    if (slot == 0)
                        slot = 9;
                    else
                        --slot;
                    break;
                case 0x4b00:
                case 0x4d00:
                    if (slot >= 5)
                        slot -= 5;
                    else
                        slot += 5;
                    break;
                case 0x4f00:
                    slot = 9;
                    break;
                case 0x5000:
                    if (slot != 9)
                        ++slot;
                    else
                        slot = 0;
                    break;
                }
            }
            break;

        }
    } while (!done);
    if (editor)
        clear_flags_of_far_block(editor);
    return result;
}


struct save_slot_table far * InitSavedGameStruct(void)
{
    int file;
    word read_count;
    struct save_slot_table far *slots;
    int count;

    if (!_dos_open(save_path, 2, &file)) {
        count = filelength(file) >= sizeof(*slots) ?
                sizeof(*slots) : filelength(file);
        slots = allocate_pinned_payload(count);
        if (_dos_read(file, slots, count, &read_count) || read_count != count) {
            clear_payload_header(slots);
            slots = 0;
        } else if (slots->count > 10) {
            clear_payload_header(slots);
            slots = 0;
        }
        _dos_close(file);
    } else {
        _fmemset(slots = allocate_pinned_payload(sizeof(*slots)), 0, sizeof(*slots));
    }
    return slots;
}

int IsValidTimeForSaveGame(void)
{
    int result = 1;

    if (LevelIdx > 14 || Kid.alive >= 0 || Kid.life_points <= 0)
        result = 0;
    return result;
}
#pragma pack(1)


int ResumeGame(int slot)
{
    long offset;
    int result = 0;
    REBIRTHREC far *saved;
    int file;
    word bytes_read;

    if (slot < 10 && !_dos_open(save_path, 2, &file)) {
        saved = allocate_pinned_payload(sizeof(*saved));
        offset = sizeof(struct save_slot_table) + slot * (long)sizeof(*saved);
        lseek(file, offset, 0);
        if (!_dos_read(file, saved, sizeof(*saved), &bytes_read) &&
            bytes_read == sizeof(*saved)) {
            if (saved->point_id) {
                if (!hRebirthSaved)
                    hRebirthSaved = allocate_pinned_payload(sizeof(*hRebirthSaved));
                _fmemcpy(hRebirthSaved, saved, sizeof(*saved));
            } else {
                RemoveRebirthPt();
            }
            Minutes = saved->menu;
            SixtiethsCtr = saved->disabled;
            LevelIdx = saved->level;
            Kid.life_initial = Kid.life_points = OrigLifePts = saved->life;
            KidInfo.entrance_mode = saved->flags;
            KidInfo.sword_kind = saved->sword_kind;
            Invert = saved->invert;
            quote_index = saved->room_state;
            Resume = 1;
            lseek(file, offset, 0);
            _dos_write(file, saved, sizeof(*saved), &bytes_read);
            result = 1;
        }
        clear_payload_header(saved);
        _dos_close(file);
    }
    return result;
}

int SaveGame(struct save_slot_table far *slots, int slot)
{
    int result = 0;
    int size;
    int file;
    word transferred;
    REBIRTHREC far *saved;

    if (slot < 10 &&
        (!_dos_open(save_path, 1, &file) || !_dos_creat(save_path, 0, &file))) {
        saved = allocate_pinned_payload(sizeof(*saved));
        if (hRebirthSaved)
            _fmemcpy(saved, hRebirthSaved, sizeof(*saved));
        else {
            _fmemset(saved, 0, sizeof(*saved));
            saved->level = LevelIdx;
            saved->life = OrigLifePts;
            saved->direction = Level.entry_facing;
        }
        saved->menu = Minutes;
        saved->disabled = SixtiethsCtr;
        saved->flags = KidInfo.entrance_mode;
        saved->sword_kind = KidInfo.sword_kind;
        saved->invert = Invert;
        saved->room_state = quote_index;
        size = sizeof(*slots);
        if (!_dos_write(file, slots, size, &transferred) && transferred == size) {
            size = sizeof(*saved);
            lseek(file, slot * (long)size, 1);
            if (!_dos_write(file, saved, size, &transferred) && transferred == size)
                result = 1;
        }
        clear_payload_header(saved);
        _dos_close(file);
    }
    return result;
}


void SaveRebirthPtStatus(int point_id)
{
    if (!hRebirthSaved)
        hRebirthSaved = allocate_pinned_payload(sizeof(*hRebirthSaved));
    hRebirthSaved->point_id = point_id;
    _fmemcpy(hRebirthSaved->cell_kinds, Level.cell_kinds, sizeof(Level.cell_kinds));
    _fmemcpy(hRebirthSaved->cell_info, Level.cell_info, sizeof(Level.cell_info));
    _fmemcpy(hRebirthSaved->room_objects, Level.room_objects, sizeof(Level.room_objects));
    _fmemcpy(hRebirthSaved->generators, Level.generators, sizeof(Level.generators));
    hRebirthSaved->level = Level.level_number;
    hRebirthSaved->life = Kid.life_initial;
    hRebirthSaved->direction = Kid.direction;
    hRebirthSaved->invert = Invert;
    hRebirthSaved->sword_kind = KidInfo.sword_kind;
    SetRebirthPtInitials();
}

void RemoveRebirthPt(void)
{
    if (hRebirthSaved) {
        clear_payload_header(hRebirthSaved);
        hRebirthSaved = 0;
    }
}


void SetRebirthPtInitials(void)
{
    byte room = 0;
    sbyte tile;
    word state, value;
    TROBREC *trob;

    if (Level.room_count != 0) {
        do {
            for (tile = 0; tile < 30; ++tile) {
                switch (hRebirthSaved->cell_kinds[room][tile]) {
                case 4:
                    state = hRebirthSaved->cell_info[room][tile].modifier;
                    value = state & 0xff;
                    trob = GetTrobRoomCell(room + 1, tile);
                    if (trob && trob->state == 3)
                        value = 200;
                    else if (trob)
                        value = 0;
                    state &= 0xff00;
                    state |= value;
                    hRebirthSaved->cell_info[room][tile].modifier = state;
                    break;
                case 17:
                    state = hRebirthSaved->cell_info[room][tile].modifier;
                    if (state & 0xff) {
                        state &= 0xff00;
                        state |= 42;
                        hRebirthSaved->cell_info[room][tile].modifier = state;
                    }
                    break;
                }
            }
        } while (++room < Level.room_count);
    }
}

void SetRebirthPtLevelStatus(void)
{
    if (hRebirthSaved) {
        _fmemcpy(Level.cell_kinds, hRebirthSaved->cell_kinds, sizeof(Level.cell_kinds));
        _fmemcpy(Level.cell_info, hRebirthSaved->cell_info, sizeof(Level.cell_info));
        _fmemcpy(Level.room_objects, hRebirthSaved->room_objects, sizeof(Level.room_objects));
        _fmemcpy(Level.generators, hRebirthSaved->generators, sizeof(Level.generators));
        Invert = hRebirthSaved->invert;
    }
}

int SetRebirthPtKidStatus(void)
{
    const REBIRTHPOINT *point;
    int result;

    if (hRebirthSaved) {
        /* Saved point IDs are one-based. */
        point = &Level.rebirth_points[hRebirthSaved->point_id - 1];
        CutRoom = Char.room = point->room;
        Char.column = point->cell % 10;
        Char.row = point->cell / 10;
        OrigLifePts = hRebirthSaved->life;
        Char.direction = hRebirthSaved->direction;
        KidInfo.sword_kind = hRebirthSaved->sword_kind;
        result = 2;
    } else {
        result = -1;
    }
    return result;
}
#pragma pack()

/* The sibling DOS image preserves the guard before the challenge setup. */
struct notice_record {
    word symbol, detail;
};


void CopyProtectionDialog(void)
{
    int passed = 1;
    word attempt;
    word previous;
    word index;
    word count;
    Handle table_handle;
    Handle asset;
    word far *table;
    struct notice_record prompt;

    if (!notice_state && !GameState.recording_mode) {
        notice_state = passed;
        InitDialog();
        asset = load_frame_asset();
        DrawDialogFrame(asset);
        attempt = 0;
        previous = -1;
        table_handle = hGetRsrc(0UL, 8000);
        table = (word far *)hLock(table_handle);
        count = table[0];
        do {
            index = Rnd(count - 1);
            if (index == previous)
                index = (Rnd(count - 2) + index + 1) % count;
            previous = index;
            index = index * 2 + 1;
            _fmemcpy(&prompt, &table[index], sizeof(prompt));
            Graf = (PORT *)BPort;
            DrawCopyProtectionDialog(prompt.symbol, asset, attempt);
            Graf = &ScreenPort;
            if (!attempt)
                DialogFadeIn();
            else
                CopyBits(&ScreenPort, BPort, notice_port, notice_port, 0);
            index = GetCopyProtectionSymbol();
            passed = index == prompt.detail || index == (word)-2;
        } while (!passed && ++attempt != 3);
        hFree(table_handle);
        hFreeShpList(asset);
        if (BPort) {
            DeallocBackPort(BPort);
            BPort = 0;
        }
        DialogFadeOut();
        DialogResetToGamePlay(0);
    }
    if (!passed)
        abort_with_message(1, notice_failure);
}

int * ComputeSymbolPt(int *p, int index)
{
    p[1] = (index % 5) * 42 + 62;
    p[0] = (index / 5) * 42 + 69;
    return p;
}


void DrawCopyProtectionDialog(word symbol, Handle asset, int attempt)
{
    char text[100];
    Handle text_handle;
    const char far *format;
    char *near_format;
    int point[2];
    POINT range;
    int index;

    if (attempt)
        FillRect((Rect far *)notice_port, 12);
    text_handle = hGetTxt4(8006);
    format = hLock(text_handle);
    near_format = _nmalloc(_fstrlen(format) + 1);
    _fstrcpy(near_format, format);
    sprintf(text, near_format, symbol);
    hFree(text_handle);
    _nfree(near_format);
    DialogDrawStr(text, 10, 0, &symbol_caption, 0, 0);

    if (!attempt) {
        DialogDrawStrId(8005, 10, 0, &symbol_heading, 0, 0);
        /* The original writes only the last bound for these two lines. */
        range.x = -1;
        DialogDrawStrId(8007, 10, 0, 0, &save_description, &range);
        range.x = 1;
        DialogDrawStrId(8008, 10, 0, 0, &save_description, &range);
        for (index = 0; index < 10; ++index) {
            ComputeSymbolPt(point, index);
            PaintRLEShapeItem(asset, index + 4, point[1], point[0], 10);
        }
    }
}

void DrawSymbolBox(int index, int selected)
{
    struct { int * ink; Rect rect; } state;
    int previous_ink;

    state.ink = &Graf->foreground;
    previous_ink = *state.ink;

    if (selected)
        *state.ink = 14;
    else
        *(word *)state.ink = 12;
    ComputeSymbolPt((int *)&state.rect, index);
    state.rect.top -= 3;
    state.rect.bottom = state.rect.top + 35;
    state.rect.left -= 3;
    state.rect.right = state.rect.left + 35;
    FrameRect(&state.rect);
    Graf->foreground = previous_ink;
}

int GetCopyProtectionSymbol(void)
{
    int index = 0;
    int previous = -1;
    int done = 0;
    word key;

    do {
        if (previous != index) {
            if (previous != -1)
                DrawSymbolBox(previous, 0);
            previous = index;
            DrawSymbolBox(index, 1);
        }
        key = read_queued_key();
        switch (key) {
        case 9:
            previous = index;
            if (index != 9)
                ++index;
            else
                index = 0;
            continue;
        case 13:
            done = 1;
            continue;
        case 17:
        case 0x1000:
            abort_with_message(0, 0);
            break;
        case 0x4700:
            index = 0;
            continue;
        case 0x4800:
        case 0x5000:
            if (index >= 5)
                index -= 5;
            else
                index += 5;
            continue;
        case 0x4b00:
            if (index % 5 == 0)
                index += 4;
            else
                --index;
            continue;
        case 0x4d00:
            if (index % 5 == 4)
                index -= 4;
            else
                ++index;
            continue;
        case 0x4f00:
            index = 9;
            continue;
        }
    } while (!done);
    DrawSymbolBox(index, 0);
    return index;
}


void HallOfFameDialog(int score)
{
    int index;
    struct hall_table far *entries;
    Rect rect;
    struct hall_table far *table;

    entries = InitHallOfFame(score, &index);
    if (entries) {
        InitDialog();
        DrawHallOfFameDialog(entries);
        Graf = &ScreenPort;
        DialogFadeIn();
        GetHallOfFameName(entries, index);
        Graf = (PORT *)BPort;
        ComputeHallPlayerRect((int far *)&rect, index);
        InsetRect(&rect, &rect, -2, 0);
        FillRect(&rect, 12);
        InsetRect(&rect, &rect, 2, 0);
        table = entries;
        DialogDrawStr(table->records[index].name, 10, 0, 0, &rect, 0);
        InsetRect(&rect, &rect, -2, 0);
        CopyBits(Graf = &ScreenPort, BPort, &rect, &rect, 0);
        WriteHallOfFameFile(table);
        Wait(0, 900);
        DialogFadeOut();
        DialogResetToGamePlay(0);
        if (BPort) {
            DeallocBackPort(BPort);
            BPort = 0;
        }
        if (entries)
            clear_payload_header(entries);
        ResetSnd(10253);
    }
}

int far * ComputeHallPlayerRect(int far *p, int index)
{
    p[1] = 25;
    p[3] = 155;
    p[0] = index * 24 + 59;
    p[2] = p[0] + 20;
    return p;
}


void DrawHallOfFameDialog(struct hall_table far *table)
{
    Rect rect;
    POINT range;
    int row;

    DrawDialogFrame(0);
    range.y = range.x = 0;
    DialogDrawStrId(8009, 12, 0, 0, &save_heading, &range);
    draw_save_slot_frame(&hall_box, 0);
    range.x = -1;
    DialogDrawStrId(8010, 10, 0, &hall_name_caption, 0, &range);
    DialogDrawStrId(8012, 10, 0, &hall_score_caption, 0, &range);

    for (row = 0; row < table->count; ++row) {
        ComputeHallPlayerRect((int far *)&rect, row);
        range.x = -1; DialogDrawStr(table->records[row].name, 10, 0, 0, &rect, &range);
        rect.left = 185; rect.right = 220;
        range.x = 0;
        OffsetRect(&rect, &rect, 75, 0);
        DialogDrawNumber(table->records[row].score, 0, &rect, &range);
    }
}

void GetHallOfFameName(struct hall_table far *table, int row)
{
    Rect rect;
    EDITFIELD far *editor;
    int key, done = 0;

    ComputeHallPlayerRect((int far *)&rect, row);
    InsetRect(&rect, &rect, -2, 0);
    draw_save_slot_frame(&rect, 1);
    InsetRect(&rect, &rect, 2, 2);
    editor = create_edit_field(
        &rect, &rect, table->records[row].name, 24);
    editor->paint = draw_dialog_text_block;
    show_edit_caret(editor);
    do {
        blink_edit_caret(editor);
        key = read_queued_key();
        if (key == 13) {
            if (!_fstrlen(editor->text))
                SetSnd(-2);
            else
                done = 1;
        } else if (key && key != '>' && key != '<') {
            handle_edit_key(editor, key);
        }
    } while (!done);
    clear_flags_of_far_block(editor);
}

struct hall_table far * HOFAddPlayer(
    struct hall_table far *table, int score, int *index)
{
    struct hall_table far *old;
    int size, row;

    if (table) {
        old = table;
        size = pGetSize(old) + sizeof(struct hall_record);
        table = allocate_pinned_payload(size);
        _fmemcpy(table, old, size - sizeof(struct hall_record));
        clear_payload_header(old);
    } else {
        table = allocate_pinned_payload(sizeof(table->count) + sizeof(struct hall_record));
        table->count = 0;
    }

    for (row = 0; row < table->count; ++row)
        if (table->records[row].score <= score)
            break;
    if (row < table->count && row != 4) {
        _fmemmove(&table->records[row + 1], &table->records[row],
                  (table->count - row) * sizeof(struct hall_record));
    }
    if (table->count < 5)
        ++table->count;
    else if (row == 5)
        row = 4;
    table->records[row].name[0] = 0;
    table->records[row].score = score;
    *index = row;
    return table;
}

struct hall_table far * InitHallOfFame(int score, int *index)
{
    struct hall_table far *table = 0;
    int file, size;
    word transferred;

    if (!_dos_open(hall_read_path, 2, &file)) {
        size = filelength(file);
        table = allocate_pinned_payload(size);
        if (_dos_read(file, table, size, &transferred) ||
            transferred != size || table->count > 5) {
            clear_payload_header(table);
            table = 0;
        }
        _dos_close(file);
    }
    if (score > 0)
        table = HOFAddPlayer(table, score, index);
    else {
        if (table)
            return table;
        table = allocate_pinned_payload(sizeof(table->count) + sizeof(struct hall_record));
        table->count = 0;
    }
    return table;
}

int ShowHallOfFame(int hold)
{
    struct hall_table far *table;
    int index;
    word key = 0;

    table = InitHallOfFame(0, &index);
    if (table && (table->count || hold)) {
        InitDialog();
        play_song(253);
        DrawHallOfFameDialog(table);
        Graf = &ScreenPort;
        key = DialogFadeIn();
        if (!key)
            key = hold ? WaitKey() : Wait(0, 900);
        if (key && !hold)
            FillRect(&ScreenPortRect, 0);
        else
            key = DialogFadeOut();
        DialogResetToGamePlay(hold);
        if (BPort && !hold) {
            DeallocBackPort(BPort);
            BPort = 0;
        }
        ResetSnd(10253);
    }
    if (table)
        clear_payload_header(table);
    return key;
}

int WriteHallOfFameFile(struct hall_table far *table)
{
    int written = 0;
    int file;
    word transferred;
    int size;

    if (!_dos_creat(hall_write_path, 0, &file)) {
        size = table->count * sizeof(struct hall_record) + sizeof(table->count);
        if (!_dos_write(file, table, size, &transferred) && size == transferred)
            written = 1;
        _dos_close(file);
    }
    return written;
}

int * make_index_rect(int *p, int index)
{
    if (index >= 3) {
        p[1] = 198;
        index -= 3;
    } else {
        p[1] = 38;
    }
    p[3] = (p[1] + 140 > 310) ? 310 : p[1] + 140;
    p[0] = index * 22 + 40;
    p[2] = p[0] + 20;
    return p;
}


int main_menu_choice(void)
{
    int previous = -1;
    int option = 0;
    int done = 0;

    do {
        if (previous != option) {
            if (previous != -1 && previous != option)
                draw_menu_icon(previous, 0);
            previous = option;
            draw_menu_icon(option, 1);
        }
        switch (read_queued_key()) {
        case 9:
            previous = option;
            if (option == 5)
                option = 0;
            else
                ++option;
            break;
        case 0x4700:
            option = 0;
            break;
        case 13:
            if (option != 3 && option != 4) {
                if (change_menu_option(option))
                    draw_menu_label(option, 1);
                else
                    SetSnd(-2);
            } else if (option == 3 && !IsValidTimeForSaveGame())
                SetSnd(-2);
            else
                done = 1;
            break;
        case 17:
        case 0x1000:
            abort_with_message(0, 0);
            break;
        case 27:
            option = 0;
            done = 1;
            break;
        case 0x4800:
            if (option == 0)
                option = 5;
            else
                --option;
            break;
        case 0x4f00:
            option = 5;
            break;
        case 0x4b00:
        case 0x4d00:
            if (option >= 3)
                option -= 3;
            else
                option += 3;
            break;
        case 0x5000:
            if (option == 5)
                option = 0;
            else
                ++option;
            break;
        }
    } while (!done);
    return option;
}

void draw_main_menu(void)
{
    int i;
    POINT range;

    Graf->background = 12;
    range.y = range.x = 0;
    DialogDrawStrId(8016, 12, 0, 0, &save_heading, &range);
    OffsetRect(&save_description, &save_description, 0, -18);
    DialogDrawStrId(8001, 10, 0, 0, &save_description, &range);
    range.x = -1;
    OffsetRect(&save_description, &save_description, 0, 18);
    DialogDrawStrId(8014, 10, 0, 0, &save_description, &range);
    range.x = 1;
    DialogDrawStrId(8015, 10, 0, 0, &save_description, &range);
    for (i = 0; i < 6; ++i)
        draw_menu_label(i, 0);
    draw_menu_icon(0, 1);
}


struct menu_text_10 { char value[10]; };
struct menu_text_12 { char value[12]; };

void draw_menu_label(int index, int selected)
{
    char label_buffer[40];
    Rect rect;
    POINT range;
    const char *label;

    if (make_index_rect((int *)&rect, index)) {
        range.x = -1;
        range.y = 0;
        switch (index) {
        case 0:
            label = Midi.volume ? sound_on_message
                                 : sound_off_message;
            break;
        case 1:
            label = ambient_state.enabled ? ambient_on_message
                               : ambient_off_message;
            break;
        case 2:
            /* The keyboard label immediately follows the joystick strings in DGROUP. */
            label = GameState.control_mode == 0 ? keyboard_mode_message
                                      : joystick_mode_message;
            break;
        case 3:
            *(struct menu_text_10 *)label_buffer = *(const struct menu_text_10 *)save_option;
            label = label_buffer;
            break;
        case 4:
            *(struct menu_text_12 *)label_buffer = *(const struct menu_text_12 *)resume_option;
            label = label_buffer;
            break;
        case 5:
            *(struct menu_text_10 *)label_buffer = *(const struct menu_text_10 *)continue_option;
            label = label_buffer;
            break;
        }
        if (selected)
            FillRect(&rect, 12);
        DialogDrawStr(label, 10, 0, 0, &rect, &range);
    }
}


void draw_menu_icon(int index, int selected)
{
    Rect rect;
    POINT origin;
    struct frame_bitmap far *bitmap;
    Handle handle;

    if (make_index_rect((int *)&rect, index)) {
        origin.y = rect.top + 6;
        origin.x = rect.left - 22;
        handle = hNewShpItem(hGetShpList(1000), 5);
        if (handle) {
            bitmap = (struct frame_bitmap far *)h2p(handle);
            if (selected) {
                MirrorMCGABlock(bitmap->pixels, bitmap->width, bitmap->height);
                PaintShape(bitmap, origin.x, origin.y, 10);
            } else {
                *(POINT *)&rect = origin;
                rect.bottom = bitmap->width + rect.top;
                rect.right = bitmap->stride + rect.left;
                FillRect(&rect, 12);
            }
        }
    }
}


struct menu_options {
    int sound, music;
};

void SaveOptionsToDisk(void)
{
    struct menu_options options;
    word written;
    int file;

    options.sound = Midi.volume == 15 ? 1 : 0;
    options.music = ambient_state.enabled;
    if (!_dos_open(options_path, 1, &file) || !_dos_creat(options_path, 0, &file)) {
        _dos_write(file, &options, sizeof(options), &written);
        _dos_close(file);
    }
}

void SetOptionsFromDisk(void)
{
    struct menu_options options;
    word transferred;
    int file;
    int size;

    if (!_dos_open(options_path, 0, &file)) {
        size = sizeof(options);
        if (!_dos_read(file, &options, size, &transferred) && transferred == size) {
            if (!options.sound)
                set_sound_level(0);
            if (!options.music)
                ambient_state.enabled = 0;
        }
        _dos_close(file);
    }
}


int change_menu_option(int option)
{
    int changed = 1;

    switch (option) {
    case 0:
        set_sound_level((byte)(!Midi.volume * 15));
        break;
    case 1:
        changed = ambient_state.enabled;
        toggle_ambient();
        changed = ambient_state.enabled != changed;
        break;
    case 2:
        if (GameState.control_mode == 2)
            GameState.control_mode = 0;
        else {
            joystick_status();
            if (GameState.control_mode != 2)
                changed = 0;
        }
        break;
    case 5:
        abort_with_message(0, 0);
        break;
    }
    return changed;
}


static const char credits_blink_key[] = "BLINK";

int DoCreditsDialog(void)
{
    Rect clip;
    int key, page;

    InitDialog();
    Graf = (PORT *)BPort;
    DrawDialogFrame(0);
    play_song(268);
    InsetRect(&clip, &ScreenPortRect, 9, 9);
    for (page = key = 0; page < sizeof(credits_first) / sizeof(*credits_first); ++page) {
        if (key)
            break;
        Graf = (PORT *)BPort;
        FillRect(&clip, 12);
        DrawCreditsPage(credits_first[page], credits_last[page]);
        Graf = &ScreenPort;
        if (page)
            key = DissolveDoAll(1, BPort, &ScreenPortRect, 90);
        else
            key = DialogFadeIn();
        if (!key) {
            if (!page && find_option(credits_blink_key)) {
                SAVEDRECT *port;
                Timer[0] = 480;
                port = SaveRect(&credits_blink_box);
                while (!key && Timer[0]) {
                    RestoreRectKeep(port);
                    key = Wait(1, 15);
                    if (!key) {
                        FillRect(&credits_blink_box, 12);
                        key = Wait(1, 15);
                    }
                }
                DeallocMapSave(port);
            } else {
                key = Wait(0, 480);
            }
        }
    }
    Graf = &ScreenPort;
    DialogFadeOut();
    hFree(hGetFont(10));
    DialogResetToGamePlay(0);
    if (BPort) {
        DeallocBackPort(BPort);
        BPort = 0;
    }
    ResetSnd(10268);
    return key;
}

void DrawCreditsPage(int first, int last)
{
    int id;

    MoveTo(save_heading.left, save_heading.bottom);
    for (id = first; id <= last; ++id) {
        DrawCreditCategory(id);
        Move(0, 4);
    }
}

void DrawCreditCategory(int id)
{
    Rect rect;
    POINT range;
    char far *strings, far *payload;
    char far *title;
    byte line;
    Handle handle = hGetTxt4(id);

    if (!handle)
        return;
    payload = hLock(handle);
    rect.bottom = Graf->pen_y;
    rect.left = save_description.left;
    rect.top = rect.bottom - 19;
    rect.right = save_description.right;
    strings = payload;
    title = lookup_prefixed_string(strings, 1);
    if (strings[0] == 1) {
        range.x = 0;
        if (id == credits_first[4] || id == credits_first[3] || (id >= 1020 && id <= 1026))
            InsetRect(&rect, &ScreenPortRect, 9, 9);
    } else {
        range.x = -1;
    }
    range.y = 0;
    DialogDrawStr(title, 10, 0, 0, &rect, &range);
    if ((byte)strings[0] > 1) {
        range.x = 1;
        line = 2;
        if ((byte)strings[0] >= 2) {
            do {
                DialogDrawStr(lookup_prefixed_string(strings, line), 10, 0, 0, &rect, &range);
                OffsetRect(&rect, &rect, 0, 21);
                Move(0, 21);
            } while (++line <= (byte)strings[0]);
        }
    } else {
        Move(0, 21);
    }
    hFree(handle);
}
