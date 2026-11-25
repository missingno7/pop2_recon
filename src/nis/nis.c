/* NIS.C. Function and field declarations reflect the reviewed DOS ABI. */
typedef struct {
    int top, left, bottom, right;
} Rect;

typedef struct {
    int y, x;
} Point;

char nis_file[] = "NIS.DAT";
char transition_file[] = "TRANS.DAT";
char caverns_file[] = "CAVERNS.DAT";
char ruins_file[] = "RUINS.DAT";
int nis_skip_requested = 0;
char *nis_data_files[6] = { "NISIBM.DAT", "NISDIGI.DAT", "NISMIDI.DAT", 0, 0, "NIS3VC.DAT" };
Rect dialog_rect = { 157, 23, 197, 296 }, extended_dialog_rect = { 10, 23, 50, 296 };
Rect nis_clip = { 32, 40, 152, 280 };
Rect small_throne_bounds = { 126, 140, 175, 180 };
Point princess_shadow = { 137, 90 };
Rect ending_fill_bounds = { 0, 0, 8, 320 };
unsigned int alternate_dialog = 0;
unsigned int extended_dialog = 0;

typedef unsigned int Handle;
typedef struct {
    unsigned long animation_key;
    int shape_mode;
} ShapeList;

extern Handle HGETRSRC(unsigned long, int);
extern Handle hNoPurge(Handle);
extern void far *h2p(Handle);
extern int cdecl sprintf(char *, const char *, ...);
extern void abort_with_message(int status, const char far *message);

Handle hGetSL(int resource_id, int mode)
{
    Handle handle = hNoPurge(HGETRSRC(0x5348504cUL, resource_id));
    {
        char message[40];

        if (handle) {
            ShapeList far *list = h2p(handle);

            list->shape_mode = mode;
            return handle;
        } else {
            sprintf(message, "hGetSL(%d)", resource_id);
            abort_with_message(1, message);
        }
    }
}

unsigned int GetStrLCt(int resource_id)
{
    return *(unsigned char far *)h2p(HGETRSRC(0x5354524cUL, resource_id));
}

extern struct { unsigned char channels, volume; } Midi;
extern union {
    unsigned long count;
    struct {
        unsigned int low, high;
    } words;
} TimeVal;
extern unsigned char midi_cue;
static unsigned long music_deadline;
extern void SetSnd(int sound);
extern int WaitSnd(int sound);
extern int (far *WaitFunc)(void);
extern int CheckWaitFunc(void);

void SetWaitSnd(int sound, unsigned int duration)
{
    if (Midi.channels & 1) SetSnd(sound);
    else
        music_deadline = TimeVal.count + duration;
}

int WaitCue(int sound);

int WaitSnd1(int sound, unsigned int duration)
{
    SetWaitSnd(sound, duration);
    return WaitCue(sound);
}

int WaitCue(int sound)
{
    int interrupted = 0;

    if (Midi.channels & 1) {
        interrupted = WaitSnd(sound);
    } else {
        do {
            if (CheckWaitFunc()) interrupted = 1;
        } while (!interrupted && TimeVal.count < music_deadline);
    }
    return interrupted;
}

extern int query_sound_payload(const void far *sound);
int WaitMidiPlayTime(unsigned char time)
{
    int interrupted;

    do {
        if (!query_sound_payload(0L))
            return 0;
        if ((interrupted = CheckWaitFunc()) != 0)
            return 1;
    } while (time > midi_cue);
    return 0;
}

typedef struct {
    char private_fields[16];
    Rect clip;
} ImagePort;

/* NIS.C's own variables, in RTLink's order from DGROUP 68fa. */
Handle hTranSL, hRuinsSL, hTitleShp1, hTitleShp2;
ImagePort *SkyMS;
Handle hDiscSL, hNISPal;
int (far *SaveWaitFunc)(void);
Handle hFont, hWitchShp, hFontHilite, hHappySL;
void far *pFont;
Handle hFlyingPal, hBorderSL, hRosyPal, hIrisPal, hWitchPal, hTreePal;
int WhichTree, NISButtonDown;
Handle hTransPalP, hTransPalS;
int pending_decoration, decoration_shape, decoration_x, decoration_y;

extern Handle hGetFont(int resource_id);
extern void hFree(Handle handle);

void GetDialog(void)
{
    hBorderSL = hGetSL(25000, 1);
    hFont = hGetFont(10);
    hFontHilite = hGetFont(11);
}

void FreeDialog(void)
{
    hFree(hFontHilite);
    hFree(hFont);
    hFree(hBorderSL);
}

typedef struct {
    char private_fields[40];
    int color;
    char private_tail[2];
} DrawPort;

extern DrawPort *Graf;
static ImagePort *alternate_port, *extended_port;

extern const char far *lookup_prefixed_string(const char far *list, unsigned char line);
extern char far *cdecl _fstrcpy(char far *to, const char far *from);
extern DrawPort *BackPort(void *saved_state, const Rect far *bounds);
extern void CopyBits(DrawPort *port, const void *source, const Rect far *src_rect,
                      const Rect far *dst_rect, int mode);
extern void PaintRLEShapeItem(Handle shapes, int shape, int x, int y, int mode);
extern void hSetFont(Handle font);
extern Rect far *AlterRect(Rect far *dst, const Rect far *src,
                                  int left, int top, int right, int bottom);
extern Rect far *OFFSETRECT(Rect far *dst, const Rect far *src, int lines, int mode);
extern void StringRect(const Rect far *rect, int x, int y, const char far *text);
extern void FreePort(void *saved_state);

void NISTextDraw(int resource_id, int line)
{
    int body_color;
    char text_line[256];
    Rect title_bounds;
    char backing_store[44];
    Rect lines;
    Handle strings;
    Rect box;
    DrawPort *old_port;
    int title_color;

    if (line == 0) text_line[0] = 0;
    else {
        strings = HGETRSRC(0x5354524cUL, resource_id);
        _fstrcpy(text_line, lookup_prefixed_string(h2p(strings), (unsigned char)line));
    }

    if (extended_dialog)
        box = extended_dialog_rect;
    else
        box = dialog_rect;

    old_port = Graf;
    Graf = BackPort(backing_store, &box);
    if (alternate_dialog) {
        body_color = 49;
        title_color = 34;
        CopyBits(Graf, alternate_port, &alternate_port->clip, &alternate_port->clip, 0);
    } else {
        body_color = 14;
        title_color = 15;
        if (extended_dialog)
            CopyBits(Graf, extended_port, &extended_port->clip, &extended_port->clip, 0);
        else
            PaintRLEShapeItem(hBorderSL, 2, 0, 157, 0);
    }

    if (pending_decoration) {
        PaintRLEShapeItem(hBorderSL, decoration_shape, decoration_x, decoration_y, 0);
        pending_decoration = 0;
    }

    hSetFont(hFont);
    Graf->color = 0;
    AlterRect(&lines, &box, 0, 3, 0, 0);
    StringRect(OFFSETRECT(&title_bounds, &lines, 1, 0), 0, 0, text_line);
    Graf->color = body_color;
    StringRect(&lines, 0, 0, text_line);
    hSetFont(hFontHilite);
    Graf->color = title_color;
    StringRect(&lines, 0, 0, text_line);

    CopyBits(old_port, backing_store, &box, &box, 0);
    Graf = old_port;
    FreePort(backing_store);
}

void RuinsSky(Handle shapes)
{
    int i = 0;

    do {
        PaintRLEShapeItem(shapes, 7, i << 5, 0, 0);
    } while (++i < 10);
}

typedef struct {
    int width, height, pitch;
    unsigned char pixels[1];
} ShapeImage;

extern void MirrorRLEBlock(unsigned char far *pixels, int width, int height);
extern void MirrorMCGABlock(unsigned char far *pixels, int width, int height);

typedef struct {
    char private_fields[0x4c0];
    Handle shapes[436];
} AnimationShapes;

void FlipHorses(const AnimationShapes *animation)
{
    int i = 400;

    do {
        if (animation->shapes[i]) {
            ShapeImage far *image = h2p(animation->shapes[i]);

            MirrorRLEBlock(image->pixels, image->width, image->height);
        }
    } while (++i <= 435);
}

void FlipKids(const AnimationShapes *animation)
{
    int i = 249;

    do {
        if (animation->shapes[i]) {
            ShapeImage far *image = h2p(animation->shapes[i]);

            MirrorMCGABlock(image->pixels, image->width, image->height);
        }
    } while (++i <= 262);
}

void FlipKid(const Handle *shapes)
{
    ShapeImage far *image;

    image = h2p(shapes[623]);
    MirrorMCGABlock(image->pixels, image->width, image->height);
}

extern DrawPort ScreenPort;
enum {
    TREE_RESOURCE = 26000,
    TREE_PALETTE_BASE = 26001,
    TREE_MUSIC = 26002,
    LAST_TREE_PALETTE = 26004,
    TREE_VOICE_BASE = 26010
};

extern int FadeTo(void *palette, unsigned int blocks, unsigned int delay);
extern int NISDialog(int resource, int line, int voice, unsigned int duration);

int TreeFunc(void *playback, int frame)
{
    int interrupted;

    midi_cue = 0;
    SetSnd(TREE_VOICE_BASE + WhichTree);
    interrupted = FadeTo(hTreePal, -1, 0);
    if (!interrupted) {
        if (WhichTree == 1) {
            DrawPort *old_port = Graf;

            Graf = &ScreenPort;
            interrupted = NISDialog(TREE_RESOURCE, 1, 1, 69);
            if (!interrupted) SetWaitSnd(TREE_MUSIC, 120);
            Graf = old_port;
        } else if (WhichTree == 9) {
            interrupted = WaitMidiPlayTime(97);
        }
    }
    return interrupted;
}

typedef int (far *AnimationCallback)(void *playback, int frame);
typedef struct TitleAnimationState TitleAnimationState;

extern void open_level_resources(char **files);
extern void InitNISInput(void);
extern void OpenRsrcVerify(const char *name);
extern void CloseRsrcFile(const char *name);
extern void close_level_resources(char **files);
extern void SetDACBlock(int first, int count, const void far *colors, int sync);
extern int PlayAnim(Handle shapes, int resource, int scene, int mode,
                              AnimationCallback callback, int x, int y, int options);
extern int Wait(int mode, unsigned int duration);
extern void ForceFadeOut(void);
extern void ResetSound(const void far *sound);

void TreeCheck(int scene)
{
    Handle sheet;
    int interrupted;

    open_level_resources(nis_data_files);
    InitNISInput();
    WhichTree = scene;
    OpenRsrcVerify(nis_file);
    sheet = hGetSL(TREE_RESOURCE, -1); GetDialog();
    SetDACBlock(0, 256, 0L, 1); PaintRLEShapeItem(hBorderSL, 1, 0, 0, 0);
    if (scene == 1) NISTextDraw(0, 0);
    PaintRLEShapeItem(sheet, 2, 40, 32, 0);

    hTreePal = HGETRSRC(0x50414c54UL, TREE_PALETTE_BASE +
                           ((WhichTree - 1) / 3 > 2 ? 2 : (WhichTree - 1) / 3));
    interrupted = PlayAnim(sheet, TREE_RESOURCE, WhichTree, 1, TreeFunc, 0, 0, 1);
    hFree(hTreePal);
    if (!interrupted) {
        if (WhichTree == 1) {
            interrupted = WaitCue(TREE_MUSIC);
            if (!interrupted) interrupted = NISDialog(TREE_RESOURCE, 2, 4, 110);
            if (!interrupted) interrupted = Wait(0, 60);
            if (!interrupted) {
                SetWaitSnd(10500, 121);
                NISTextDraw(TREE_RESOURCE, 3);
                interrupted = WaitCue(10500);
            }
        } else if (WhichTree == 9) {
            interrupted = WaitMidiPlayTime(98);
            if (!interrupted) {
                PaintRLEShapeItem(sheet, 15, 69, 56, 10);
                interrupted = WaitMidiPlayTime(99);
                if (!interrupted) {
                    hTreePal = HGETRSRC(0x50414c54UL, LAST_TREE_PALETTE);
                    interrupted = FadeTo(hTreePal, -1, 2); hFree(hTreePal);
                }
            }
        }
    }
    if (!interrupted)
        interrupted = WaitSnd(0);
    ForceFadeOut();
    if (interrupted) ResetSound(0L);

    FreeDialog(); hFree(sheet); hFree(hBorderSL);
    CloseRsrcFile(nis_file);
    WaitFunc = SaveWaitFunc;
    close_level_resources(nis_data_files);
}

extern Rect ScreenPortRect, GamePlayRect;
extern int RLEShapeCutOff;
extern int ForceFuncParam;
enum { TRANSITION_RESOURCE = 25000, PRINCESS_MUSIC = 31030, PRINCESS_CUE = 31031 };
extern void FillRect(const Rect far *bounds, int color);
extern int SndOn(int sound);
extern Handle hNewShpItem1(Handle shapes, int item, int flags);
extern void DeallocMapSave(ImagePort *port);
extern void DrawRoomFromNIS(signed char room, signed char level);
extern void PaintShapeItem(Handle shapes, int item, int x, int y, int mode, int plane);
extern void EraseRect(const Rect far *bounds);
extern int TitleFunc(TitleAnimationState *playback, int frame);
extern int TransFunc1(void *playback, int frame);
extern int TransFunc4(void *playback, int frame);
extern int TransFunc5(void *playback, int frame);
extern int TransFunc6(void *playback, int frame);
extern int TransFunc7(void *playback, int frame);
extern int TransFunc8(void *playback, int frame);
extern int TransFunc9(void *playback, int frame);
extern int TransFunc10(void *playback, int frame);
extern int TransFunc11(void *playback, int frame);
extern int TransFuncJaffarMagic(void *playback, int frame);

int TransPrincessRoom(Handle shapes);
int TransThroneRoom(Handle shapes);
void DrawPrincessRoom(Handle shapes);

#define TRANSITION_COLORS(i) HGETRSRC(0x50414c54UL, TRANSITION_RESOURCE+(i))
#define PLAY_TRANSITION(script, mode, callback) \
    PlayAnim(hTranSL, TRANSITION_RESOURCE, script, mode, callback, 0, 0, 1)

#define NIS_ITEM(item, x, y, mode) \
    PaintRLEShapeItem(hTranSL, item, x, y, mode)
#define BACKDROP_ITEM(item, x, y, mode) \
    PaintRLEShapeItem(hRuinsSL, item, x, y, mode)

int Transition(int scene)
{
    int interrupted;

    open_level_resources(nis_data_files);
    RLEShapeCutOff = 265;
    if (scene != 5)
        OpenRsrcVerify(transition_file);
    if (scene != 6) {
        SetDACBlock(0, 256, 0L, 1);
        FillRect(&ScreenPortRect, 0);
    }
    InitNISInput();
    hTranSL = hGetSL(TRANSITION_RESOURCE, -1);

    if (scene == 4) {
        if (!SndOn(TRANSITION_RESOURCE + 11))
            SetSnd(TRANSITION_RESOURCE + 16);
        FillRect(&ScreenPortRect, 16);
        NIS_ITEM(320, 118, 0, 10);
        NIS_ITEM(321, 73, 60, 10);
        NIS_ITEM(322, -41, 32, 10);
        hTitleShp1 = hNewShpItem1(hTranSL, 312, 1);
        hTitleShp2 = hNewShpItem1(hTranSL, 313, 1);
        hTransPalP = TRANSITION_COLORS(2);
        SkyMS = 0;
        ForceFuncParam = 2;
        interrupted = PLAY_TRANSITION(12, 1, TitleFunc);
        ForceFuncParam = 0;
        if (SkyMS)
            DeallocMapSave(SkyMS);
        hFree(hTransPalP);
        hFree(hTitleShp2);
        hFree(hTitleShp1);
    } else if (scene == 1) {
        OpenRsrcVerify(caverns_file);
        OpenRsrcVerify(ruins_file);
        FillRect(&GamePlayRect, 48);
        NIS_ITEM(358, -127, -34, 0);
        NIS_ITEM(354, 0, 105, 0);
        NIS_ITEM(369, 185, 46, 10);
        hTransPalP = TRANSITION_COLORS(1);
        interrupted = PLAY_TRANSITION(1, 1, TransFunc1);
        hFree(hTransPalP);
        if (!interrupted) {
            SetDACBlock(0, 256, 0L, 1);
            FillRect(&ScreenPortRect, 0);
            FillRect(&GamePlayRect, 16);
            NIS_ITEM(320, 117, 0, 10);
            NIS_ITEM(321, 73, 60, 10);
            NIS_ITEM(322, -41, 34, 10);
            hTransPalP = TRANSITION_COLORS(2);
            interrupted = PLAY_TRANSITION(2, 1, TransFunc1);
            hFree(hTransPalP);
        }
        if (!interrupted) {
            SetDACBlock(0, 256, 0L, 1);
            FillRect(&ScreenPortRect, 0);
            hFree(hTranSL);
            hTranSL = hGetSL(TRANSITION_RESOURCE, -2);
            hRuinsSL = hGetSL(3500, -16);
            BACKDROP_ITEM(1100, 0, 0, 0);
            BACKDROP_ITEM(1101, 10, 100, 0);
            BACKDROP_ITEM(1102, 0, 100, 0);
            hFree(hRuinsSL);
            hTransPalP = TRANSITION_COLORS(11);
            interrupted = FadeTo(hTransPalP, -1, 0);
            hFree(hTransPalP);
        }
        if (!interrupted)
            interrupted = PLAY_TRANSITION(3, 1, 0);
        CloseRsrcFile(ruins_file);
        CloseRsrcFile(caverns_file);
    } else if (scene == 2) {
        OpenRsrcVerify(ruins_file);
        hRuinsSL = hGetSL(3500, -8);
        RuinsSky(hRuinsSL);
        BACKDROP_ITEM(763, 0, 39, 10);
        BACKDROP_ITEM(764, 0, 99, 0);
        NIS_ITEM(317, 75, 166, 10);
        hTransPalP = TRANSITION_COLORS(3);
        hTransPalS = TRANSITION_COLORS(4);
        interrupted = PLAY_TRANSITION(4, 1, TransFunc4);
        hFree(hTransPalS);
        hFree(hTransPalP);
        if (!interrupted) {
            hTransPalP = TRANSITION_COLORS(4);
            interrupted = PLAY_TRANSITION(5, 1, TransFunc5);
            hFree(hTransPalP);
        }
        if (!interrupted) {
            SetDACBlock(0, 256, 0L, 1);
            hTransPalP = TRANSITION_COLORS(4);
            RuinsSky(hRuinsSL);
            BACKDROP_ITEM(766, 0, 11, 10);
            BACKDROP_ITEM(767, 0, 101, 0);
            SetDACBlock(0, 256, h2p(hTransPalP), 1);
            hFree(hTransPalP);
            interrupted = PLAY_TRANSITION(6, 1, TransFunc6);
        }
        if (!interrupted) {
            interrupted = FadeTo(0, -1, 0);
            FillRect(&ScreenPortRect, 0);
        }
        if (!interrupted) {
            NIS_ITEM(347, 0, 0, 10);
            hTransPalP = TRANSITION_COLORS(5);
            interrupted = FadeTo(hTransPalP, -1, 0);
            hFree(hTransPalP);
        }
        if (!interrupted)
            interrupted = PLAY_TRANSITION(7, 1, TransFunc7);
        if (!interrupted) {
            interrupted = FadeTo(0, -1, 0);
            FillRect(&ScreenPortRect, 0);
        }
        if (!interrupted) {
            FillRect(&GamePlayRect, 0);
            hFree(hTranSL);
            hTranSL = hGetSL(TRANSITION_RESOURCE, -2);
            hTransPalP = TRANSITION_COLORS(6);
            hTransPalS = TRANSITION_COLORS(7);
            DrawRoomFromNIS(10, 22);
            interrupted = PLAY_TRANSITION(8, 1, TransFunc8);
            hFree(hTransPalS);
            hFree(hTransPalP);
        }
        hFree(hRuinsSL);
    } else if (scene == 3) {
        FillRect(&GamePlayRect, 48);
        NIS_ITEM(362, 0, 126, 0);
        NIS_ITEM(363, 0, 160, 10);
        NIS_ITEM(364, 0, 168, 10);
        NIS_ITEM(365, 183, 97, 10);
        NIS_ITEM(368, 11, 77, 10);
        NIS_ITEM(369, -12, 40, 10);
        NIS_ITEM(373, 28, 13, 10);
        NIS_ITEM(373, 154, 31, 10);
        NIS_ITEM(374, 126, 50, 10);
        NIS_ITEM(352, 0, 0, 10);
        interrupted = PLAY_TRANSITION(9, 1, TransFunc9);
        if (!interrupted) {
            FadeTo(0, -1, 0);
            FillRect(&ScreenPortRect, 0);
            FillRect(&GamePlayRect, 48);
            NIS_ITEM(373, 153, 33, 0); NIS_ITEM(374, 129, 51, 0); NIS_ITEM(375, -8, 39, 0); NIS_ITEM(373, 2, 10, 0);
            NIS_ITEM(362, 0, 135, 0);
            NIS_ITEM(363, 0, 168, 0);
            NIS_ITEM(364, 0, 176, 10);
            hTransPalP = TRANSITION_COLORS(8);
            interrupted = PLAY_TRANSITION(10, 1, TransFunc10);
            hFree(hTransPalP);
        }
        if (!interrupted) {
            SetDACBlock(0, 256, 0L, 1);
            FillRect(&ScreenPortRect, 0);
            hFree(hTranSL);
            hTranSL = hGetSL(TRANSITION_RESOURCE, -2);
            DrawRoomFromNIS(14, 1);
            hTransPalP = TRANSITION_COLORS(9);
            interrupted = FadeTo(hTransPalP, -1, 0);
            hFree(hTransPalP);
        }
        if (!interrupted) {
            hTransPalS = TRANSITION_COLORS(10);
            interrupted = PLAY_TRANSITION(11, 1, TransFunc11);
            hFree(hTransPalS);
        }
    } else if (scene == 5) {
        interrupted = TransPrincessRoom(hTranSL);
    } else if (scene == 6) {
        interrupted = TransThroneRoom(hTranSL);
    }

    if (interrupted) {
        ForceFadeOut();
        ResetSound(0L);
    }
    hFree(hTranSL);
    WaitFunc = SaveWaitFunc;
    if (scene != 5)
        CloseRsrcFile(transition_file);
    if (scene != 6)
        CloseRsrcFile(ruins_file);
    RLEShapeCutOff = 0; close_level_resources(nis_data_files);
    return interrupted;
}

#undef BACKDROP_ITEM
#undef NIS_ITEM

int TransPrincessRoom(Handle shapes)
{
    int interrupted;
    Handle shadow;

    RLEShapeCutOff = 265;
    DrawPrincessRoom(shapes);
    shadow = hGetSL(TRANSITION_RESOURCE + 1, 2);
    PaintShapeItem(shadow, 249, princess_shadow.x, princess_shadow.y, 10, 1);
    hFree(shadow);
    SetSnd(PRINCESS_MUSIC);
    hTransPalP = TRANSITION_COLORS(0);
    interrupted = FadeTo(hTransPalP, -1, 0);
    hFree(hTransPalP);
    if (!interrupted)
        interrupted = PlayAnim(shapes, TRANSITION_RESOURCE, 0, 1, 0, 0, 0, 1);
    if (!interrupted)
        interrupted = Wait(0, 90);
    if (!interrupted) {
        ((ShapeList far *)h2p(shapes))->shape_mode = -2;
        interrupted = PlayAnim(shapes, TRANSITION_RESOURCE, 1, 1, TransFuncJaffarMagic, 0, 0, 1);
        EraseRect(&ScreenPortRect);
    }
    RLEShapeCutOff = 0;
    return interrupted;
}

void DrawPrincessRoom(Handle shapes)
{
    PaintRLEShapeItem(hTranSL, 344, 0, -7, 0);
    PaintRLEShapeItem(hTranSL, 345, 0, 93, 0);
    PaintRLEShapeItem(hTranSL, 348, 34, 79, 10);
    PaintRLEShapeItem(hTranSL, 348, 223, 79, 10);
}

#undef PLAY_TRANSITION
#undef TRANSITION_COLORS

typedef struct {
    char header[0x2e];
    ImagePort port;
    char image_table[0x4bc - 0x46];
    void far *fade;
} AnimationState;

extern void DrawDeadKid(int ruined);
extern ImagePort *SaveRect(const Rect far *bounds);
extern DrawPort *AllocBackPort(const Rect far *bounds);
extern void DeallocBackPort(DrawPort *port);
extern void DeallocMapSave(ImagePort *port);
extern int DissolveDoAll(int mode, DrawPort *port, const Rect far *bounds, long duration);
extern void ThroneMoveQueenInFrontOfKid(Rect *bounds, int resource);
extern int TransThroneFunc(void *playback, int frame);
enum { THRONE_DIALOG_RESOURCE = 31000, THRONE_MUSIC = 31020, THRONE_NEXT_MUSIC = 31021 };
extern void far *InitFadeTo(Handle palette, int timing, int options);
extern void SetDACBlocks(int timing, const void far *colors);

void DrawRuinedThroneRoom(Handle shapes);

int TransThroneRoom(Handle shapes)
{
    DrawPort *port;
    Handle tile_sheet, colors, animation_shapes;
    int skipped;

    extended_dialog = 1;
    extended_port = 0;
    hFont = hGetFont(10);
    hFontHilite = hGetFont(11);
    port = AllocBackPort(&GamePlayRect);
    Graf = port;
    tile_sheet = hGetSL(3500, -16);
    PaintRLEShapeItem(tile_sheet, 708, 0, 0, 0);
    PaintRLEShapeItem(tile_sheet, 709, 0, 100, 0);
    DrawDeadKid(0);
    colors = HGETRSRC(0x50414c54UL, 4208);
    SetDACBlock(0, 224, h2p(colors), 1);
    hFree(colors);
    skipped = WaitSnd(254);
    if (!skipped) {
        SetSnd(THRONE_MUSIC);
        Graf = &ScreenPort;
        skipped = DissolveDoAll(1, port, &GamePlayRect, 240L);
    }
    if (!skipped) {
        colors = HGETRSRC(0x50414c54UL, 4210);
        SetDACBlock(32, 16, h2p(colors), 1);
        hFree(colors);
        animation_shapes = hGetSL(3500, 4);
        ((ShapeList far *)h2p(animation_shapes))->animation_key = (4UL << 16) | 4210;
        ThroneMoveQueenInFrontOfKid(&small_throne_bounds, 4210);
        skipped = WaitMidiPlayTime(97);
    }
    if (!skipped) {
        skipped = PlayAnim(animation_shapes, 4210, 0, 1, TransThroneFunc, 0, 0, 1);
        hFree(animation_shapes);
        DeallocBackPort(port);
        port = 0;
    }
    if (!skipped) {
        Graf = &ScreenPort;
        extended_port = SaveRect(&extended_dialog_rect);
    }
    if (!skipped) skipped = NISDialog(THRONE_DIALOG_RESOURCE, 1, 1, 311);
    if (!skipped) skipped = NISDialog(THRONE_DIALOG_RESOURCE, 2, 2, 278);
    if (!skipped) skipped = NISDialog(THRONE_DIALOG_RESOURCE, 3, 3, 121);
    if (!skipped)
        skipped = NISDialog(THRONE_DIALOG_RESOURCE, 4, 4, 326);
    if (!skipped) {
        NISTextDraw(0, 0); skipped = Wait(0, 60);
    }
    if (!skipped)
        skipped = NISDialog(THRONE_DIALOG_RESOURCE, 5, 5, 222);
    if (!skipped)
        skipped = Wait(0, 60);
    if (!skipped)
        skipped = NISDialog(THRONE_DIALOG_RESOURCE, 6, 6, 224);
    if (!skipped) {
        skipped = Wait(0, 60); NISTextDraw(0, 0);
    }
    if (!skipped)
        skipped = WaitSnd(THRONE_MUSIC);
    SetSnd(10255);
    if (extended_port) {
        DeallocMapSave(extended_port);
        extended_port = 0;
    }
    if (!skipped) {
        port = AllocBackPort(&small_throne_bounds);
        Graf = port;
        tile_sheet = hGetSL(3500, -16);
        PaintRLEShapeItem(tile_sheet, 709, 0, 100, 0);
        Graf = &ScreenPort;
        skipped = DissolveDoAll(1, port, &small_throne_bounds, 180L);
        DeallocBackPort(port);
        port = 0;
    }
    if (!skipped) {
        port = AllocBackPort(&GamePlayRect);
        Graf = port;
        tile_sheet = hGetSL(3500, -16);
        DrawRuinedThroneRoom(tile_sheet);
    }
    if (!skipped) {
        colors = HGETRSRC(0x50414c54UL, 4209);
        skipped = FadeTo(colors, -1, 1);
        hFree(colors);
    }
    if (!skipped) {
        CopyBits(&ScreenPort, port, &GamePlayRect, &GamePlayRect, 0);
        colors = HGETRSRC(0x50414c54UL, 4211);
        skipped = FadeTo(colors, -1, 1);
        hFree(colors);
    }
    if (!skipped)
        skipped = Wait(0, 60);
    if (port)
        DeallocBackPort(port);
    Graf = &ScreenPort;
    hFree(hGetSL(3500, -16));
    hFree(HGETRSRC(0x5354524cUL, 4210));
    hFree(hFontHilite);
    hFree(hFont);
    if (!SndOn(10255))
        SetSnd(10255);
    extended_dialog = 0;
    return skipped;
}

void DrawRuinedThroneRoom(Handle shapes)
{
    PaintRLEShapeItem(shapes, 700, 27, 0, 0);
    PaintRLEShapeItem(shapes, 701, 28, 100, 0);
    PaintRLEShapeItem(shapes, 702, 0, 0, 10);
    PaintRLEShapeItem(shapes, 703, 289, 90, 10);
    PaintRLEShapeItem(shapes, 704, 56, 181, 10);
    PaintRLEShapeItem(shapes, 705, 56, 185, 10);
    PaintRLEShapeItem(shapes, 707, 274, 90, 10);
    DrawDeadKid(1);
}

int TransThroneFunc(void *source, int frame)
{
    int interrupted;

    {
        DrawPort *port = AllocBackPort(&small_throne_bounds);

        CopyBits(port, source, &small_throne_bounds, &((ImagePort *)source)->clip, 0);
        Graf = &ScreenPort;
        interrupted = DissolveDoAll(1, port, &small_throne_bounds, 180L);
        DeallocBackPort(port);
    }
    if (!interrupted)
        interrupted = WaitMidiPlayTime(98);
    return interrupted;
}

void InitFadeToPrimary(AnimationState *playback, int options)
{
    playback->fade = InitFadeTo(hTransPalP, -1, options);
}

void CutToPrimary(void)
{
    SetDACBlocks(-1, h2p(hTransPalP));
}

void InitFadeToBlack(AnimationState *playback, int options)
{
    playback->fade = InitFadeTo(0, -1, options);
}

int TransFunc1(void *playback, int frame)
{
    int interrupted = 0;

    if (frame == 0) {
        InitFadeToPrimary(playback, 1);
        interrupted = WaitMidiPlayTime(97);
    } else {
        InitFadeToBlack(playback, 1);
    }
    return interrupted;
}

int TransFunc4(void *playback, int frame)
{
    if (frame == 0) {
        FlipHorses(playback);
        SetSnd(TRANSITION_RESOURCE + 4);
    } else if (frame == 1) {
        CutToPrimary();
    } else {
        ((AnimationState *)playback)->fade = InitFadeTo(hTransPalS, 7, 1);
    }
    return 0;
}

int TransFunc5(void *playback, int frame)
{
    DrawPort *saved;
    Rect lowered_bounds;

    if (frame == 0) {
        FlipHorses(playback);
        SetDACBlock(0, 256, 0L, 1);
        saved = Graf; Graf = &ScreenPort;
        FillRect(&GamePlayRect, 0);
        RuinsSky(hRuinsSL);
        lowered_bounds = GamePlayRect;
        lowered_bounds.top = 80;
        FillRect(&lowered_bounds, 162);
        PaintRLEShapeItem(hTranSL, 332, 0, 35, 10);
        Graf = saved;
        CopyBits((DrawPort *)&((AnimationState *)playback)->port, &ScreenPort,
                  OFFSETRECT(&lowered_bounds, &ScreenPortRect,
                                  -((AnimationState *)playback)->port.clip.left,
                                  -((AnimationState *)playback)->port.clip.top),
                  &ScreenPortRect, 0);
    } else if (frame == 1) {
        CutToPrimary();
    } else {
        SetDACBlock(0, 256, 0L, 1);
    }
    return 0;
}

int TransFunc6(void *playback, int frame)
{
    if (frame == 0)
        FlipHorses(playback);
    else
        SetSnd(TRANSITION_RESOURCE + 6);
    return 0;
}

int TransFunc7(void *playback, int frame)
{
    FlipHorses(playback);
    return 0;
}

int TransFunc8(void *playback, int frame)
{
    if (frame == 0) {
        FlipKid(playback);
        CutToPrimary();
    } else if (frame == 1) {
        ((AnimationState *)playback)->fade = InitFadeTo(hTransPalS, 7, 1);
    }
    return 0;
}

int TransFunc9(void *playback, int frame)
{
    int interrupted;

    SetSnd(TRANSITION_RESOURCE + 9);
    hTransPalP = HGETRSRC(0x50414c54UL, TRANSITION_RESOURCE + 8);
    interrupted = FadeTo(hTransPalP, -1, 0);
    hFree(hTransPalP);
    if (!interrupted)
        interrupted = WaitMidiPlayTime(97);
    return interrupted;
}

int TransFunc10(void *playback, int frame)
{
    if (frame == 0)
        InitFadeToPrimary(playback, 1);
    else
        InitFadeToBlack(playback, 1);
    return 0;
}

int TransFunc11(void *playback, int frame)
{
    if (frame == 0)
        FlipKid(playback);
    else
        ((AnimationState *)playback)->fade = InitFadeTo(hTransPalS, 7, 1);
    return 0;
}

void EraseStaticJaffar(DrawPort *port);

int TransFuncJaffarMagic(void *playback, int frame)
{
    Handle palette;

    if (frame != 0) {
        if (frame == 1)
            SetSnd(PRINCESS_CUE);
        palette = HGETRSRC(0x50414c54UL, TRANSITION_RESOURCE + frame);
        if (palette) {
            const void far *colors = h2p(palette);

            SetDACBlock(0, 256, colors, 1);
            hFree(palette);
        }
    } else {
        FlipKids(playback);
        EraseStaticJaffar(playback);
    }
    return 0;
}

void EraseStaticJaffar(DrawPort *port)
{
    Rect *clip;
    DrawPort *saved = Graf;

    Graf = port;
    PaintRLEShapeItem(hGetSL(TRANSITION_RESOURCE, -1), 363, -6, -3, 0);
    clip = &((ImagePort *)port)->clip;
    CopyBits((DrawPort *)((char *)port + 0x2e), port,
                 clip, clip, 0);
    Graf = saved;
}

extern void *cdecl memset(void *destination, int value, unsigned int length);

extern volatile int Timer[4];
extern int WaitTimer(int timer);
void ClearNIS(DrawPort *port, int color);
void AddNIS(DrawPort *port, Handle shapes, int item, int x, int y, int mode);
void CopyNIS(const void *source);
void CutNIS(const void *source);
int FadeNIS(const void *source, int duration);

enum { SHIP_SCENE = 28000, SHIP_MUSIC = 28100 };
#define SHIP_PALETTE(index) HGETRSRC(0x50414c54UL, SHIP_SCENE + (index))

int AboardShip(void)
{
    Handle sheet;
    int aborted;
    DrawPort port;
    unsigned char white[240][3];

    SetSnd(SHIP_MUSIC); memset(white, 63, sizeof white);
    BackPort(&port, &nis_clip);
    hBorderSL = hGetSL(25000, 1);
    sheet = hGetSL(SHIP_SCENE, -1);
    GetDialog();
    SetDACBlock(0, 256, 0L, 1);
    PaintRLEShapeItem(hBorderSL, 1, 0, 0, 0);
    PaintRLEShapeItem(sheet, 2, 40, 32, 0);
    NISTextDraw(0, 0);

    hNISPal = SHIP_PALETTE(1);
    aborted = WaitMidiPlayTime(97);
    if (!aborted) aborted = FadeTo(hNISPal, -1, 2);
    if (!aborted) aborted = WaitMidiPlayTime(98);
    if (!aborted) {
        decoration_shape = 5;
        decoration_x = 88;
        decoration_y = 159;
        pending_decoration = 1;
        aborted = NISDialog(SHIP_SCENE, 1, 1, 186);
        if (!aborted)
            aborted = Wait(0, 20);
    }
    if (!aborted) {
        hFree(hNISPal);
        hNISPal = SHIP_PALETTE(2);
        ClearNIS(&port, 0);
        AddNIS(&port, sheet, 3, 38, 31, 0);
        AddNIS(&port, sheet, 4, 37, 102, 10);
        aborted = WaitMidiPlayTime(99);
        NISTextDraw(0, 0);
        if (!aborted)
            aborted = FadeNIS(&port, 0);
        if (!aborted)
            aborted = Wait(0, 18);
    }
    if (!aborted) aborted = WaitMidiPlayTime(100);
    if (!aborted) {
        aborted = NISDialog(SHIP_SCENE, 2, 2, 131);
        if (!aborted) aborted = Wait(0, 20);
    }
    if (!aborted) aborted = WaitMidiPlayTime(101);
    if (!aborted) {
        aborted = NISDialog(SHIP_SCENE, 3, 3, 200);
        if (!aborted) aborted = Wait(0, 20);
    }
    if (!aborted) {
        NISTextDraw(0, 0); aborted = Wait(0, 72);
    }
    if (!aborted) {
        aborted = WaitMidiPlayTime(102);
        if (!aborted) SetWaitSnd(SHIP_SCENE + 5, 238);
        if (!aborted) aborted = FadeTo(0, -2, 5);
    }
    if (!aborted) aborted = WaitCue(SHIP_SCENE + 5);
    if (!aborted) {
        hFree(hNISPal); hNISPal = SHIP_PALETTE(2); ClearNIS(&port, 0); AddNIS(&port, sheet, 5, 40, 32, 0);
        aborted = WaitMidiPlayTime(103);
        if (!aborted) aborted = FadeNIS(&port, 3);
    }
    if (!aborted)
        aborted = WaitMidiPlayTime(104);
    if (!aborted) {
        SetWaitSnd(10500, 121);
        NISTextDraw(SHIP_SCENE, 4);
        aborted = WaitCue(10500);
    }
    if (!aborted)
        aborted = WaitMidiPlayTime(105);
    if (!aborted)
        SetSnd(30500);
    if (!aborted) {
        NISTextDraw(0, 0);
        AddNIS(&port, sheet, 6, 41, 32, 10);
        CutNIS(&port);
        aborted = Wait(0, 6);
    }
    if (!aborted) {
        hFree(hNISPal);
        hNISPal = SHIP_PALETTE(4);
        SetDACBlocks(-1, h2p(hNISPal));
        aborted = Wait(0, 12);
    }
    if (!aborted) {
        SetDACBlock(16, 240, white, 1);
        aborted = Wait(0, 12);
    }
    if (!aborted) {
        hFree(hNISPal);
        hNISPal = SHIP_PALETTE(5);
        ClearNIS(&port, 0);
        AddNIS(&port, sheet, 7, 40, 32, 0);
        SetSnd(SHIP_MUSIC + 1);
        CopyNIS(&port);
        if (!aborted)
            aborted = Wait(0, 18);
    }
    if (!aborted) {
        hFree(hNISPal);
        hNISPal = SHIP_PALETTE(2);
        SetDACBlocks(-1, h2p(hNISPal));
        aborted = Wait(0, 6);
    }
    if (!aborted) {
        ClearNIS(&port, 0); AddNIS(&port, sheet, 3, 39, 32, 0); AddNIS(&port, sheet, 8, 38, 86, 10); CutNIS(&port);
        if (!aborted) aborted = Wait(0, 114);
    }
    if (!aborted) {
        hFree(hNISPal); hNISPal = SHIP_PALETTE(6); ClearNIS(&port, 0); AddNIS(&port, sheet, 9, 38, 27, 0);
        aborted = WaitMidiPlayTime(98);
        if (!aborted) CopyNIS(&port);
    }
    if (!aborted) {
        AddNIS(&port, sheet, 10, 40, 33, 10); CutNIS(&port);
        if (!aborted)
            aborted = Wait(0, 6);
    }
    if (!aborted) {
        if (Midi.channels & 3)
            SetSnd(30500);
        ClearNIS(&port, 0);
        AddNIS(&port, sheet, 9, 38, 27, 0);
        AddNIS(&port, sheet, 11, 40, 32, 10);
        CutNIS(&port);
        if (!aborted)
            aborted = Wait(0, 6);
    }
    if (!aborted) {
        ClearNIS(&port, 0);
        AddNIS(&port, sheet, 9, 38, 27, 0);
        AddNIS(&port, sheet, 12, 39, 32, 10);
        CutNIS(&port);
        if (!aborted)
            aborted = Wait(0, 6);
    }
    if (!aborted) {
        ClearNIS(&port, 0);
        AddNIS(&port, sheet, 13, 39, 31, 0);
        CutNIS(&port);
        if (!aborted)
            aborted = Wait(0, 6);
    }
    if (!aborted) {
        SetDACBlock(16, 240, white, 1);
        Timer[0] = 12;
        aborted = Wait(0, 12);
    }
    if (!aborted) {
        ClearNIS(&port, 0);
        AddNIS(&port, sheet, 17, 40, 31, 0);
        aborted = WaitTimer(0);
        if (!aborted) {
            SetDACBlocks(-2, 0L);
            aborted = FadeNIS(&port, 0);
            if (!aborted)
                aborted = Wait(0, 18);
        }
    }
    if (!aborted)
        aborted = WaitMidiPlayTime(99);
    if (!aborted)
        SetSnd(10262);
    if (!aborted) {
        AddNIS(&port, sheet, 14, 67, 45, 10);
        CutNIS(&port);
        if (!aborted)
            aborted = Wait(0, 6);
    }
    if (!aborted) {
        ClearNIS(&port, 0);
        AddNIS(&port, sheet, 17, 40, 31, 0);
        AddNIS(&port, sheet, 15, 78, 44, 10);
        CutNIS(&port);
        if (!aborted)
            aborted = Wait(0, 6);
    }
    if (!aborted) {
        ClearNIS(&port, 0);
        AddNIS(&port, sheet, 17, 40, 31, 0);
        AddNIS(&port, sheet, 16, 77, 46, 10);
        CutNIS(&port);
        if (!aborted)
            aborted = Wait(0, 12);
    }
    if (!aborted) {
        aborted = WaitMidiPlayTime(100);
        if (!aborted)
            aborted = FadeTo(0, -1, 4);
    }

    ForceFadeOut();
    if (aborted)
        ResetSound(0L);
    hFree(hNISPal);
    FreeDialog();
    hFree(sheet);
    FreePort(&port);
    return aborted;
}

#undef SHIP_PALETTE

extern Handle hGetSnd(int cue);
extern void ResetSnd(int sound);
extern Rect *GetSRect(Rect far *bounds, Handle shapes, int item, int x, int y);
extern void AddShpNIS(DrawPort *port, Handle shape, int x, int y, int mode);
int DissolveNIS(DrawPort *port, unsigned int duration, const Rect far *bounds);
extern Rect far *UNIONRECT(Rect far *dst, const Rect far *a, const Rect far *b);
extern void PaintRLEShape_asm(const void far *shape, int x, int y, int mode);

typedef DrawPort NisTemporaryPort;

enum { OPENING_SCENE = 27000 };

int Opening1(void)
{
    int interrupted;
    Handle shapes_handle;
    Rect bounds;
    NisTemporaryPort game_port;
    midi_cue = 0;
    SetSnd(25010);
    BackPort(&game_port, &nis_clip);
    hBorderSL = hGetSL(25000, 1);
    shapes_handle = hGetSL(OPENING_SCENE, -1);
    GetDialog();
    SetDACBlock(0, 256, 0L, 1);
    PaintRLEShapeItem(hBorderSL, 1, 0, 0, 0);
    FillRect(&nis_clip, 212);
    NISTextDraw(0, 0);
    CopyBits(&game_port, Graf, &nis_clip, &nis_clip, 0); hNISPal = HGETRSRC(0x50414c54UL, OPENING_SCENE + 1);
    interrupted = FadeTo(hNISPal, -1, 0);
    if (!interrupted) {
        ClearNIS(&game_port, 212); AddNIS(&game_port, shapes_handle, 4, 98, 62, 10);
        GetSRect(&bounds, shapes_handle, 4, 98, 62); interrupted = WaitMidiPlayTime(97);
        if (!interrupted) interrupted = DissolveNIS(&game_port, 45, &bounds); if (!interrupted) interrupted = Wait(0, 120);
    }
    if (!interrupted) {
        ClearNIS(&game_port, 212); GetSRect(&bounds, shapes_handle, 4, 98, 62);
        interrupted = DissolveNIS(&game_port, 45, &bounds); if (!interrupted) interrupted = Wait(0, 48);
    }
    if (!interrupted) {
        AddNIS(&game_port, shapes_handle, 5, 75, 68, 10); GetSRect(&bounds, shapes_handle, 5, 75, 68);
        interrupted = WaitMidiPlayTime(98); if (!interrupted) interrupted = DissolveNIS(&game_port, 45, &bounds);
        if (!interrupted) interrupted = Wait(0, 120);
    }
    if (!interrupted) {
        ClearNIS(&game_port, 212); GetSRect(&bounds, shapes_handle, 5, 75, 68);
        interrupted = DissolveNIS(&game_port, 45, &bounds); if (!interrupted) interrupted = Wait(0, 48);
    }
    if (!interrupted) {
        ClearNIS(&game_port, 0);
        AddNIS(&game_port, shapes_handle, 6, 40, 32, 0);
        interrupted = FadeTo(0, -2, 0);
        if (!interrupted) {
            CopyBits(Graf, &game_port, &nis_clip, &nis_clip, 0);
            interrupted = WaitMidiPlayTime(99);
            if (!interrupted) interrupted = FadeTo(hNISPal, -1, 0);
        }
    }
    if (!interrupted) {
        SetWaitSnd(OPENING_SCENE + 1, 257);
        decoration_shape = 4;
        decoration_x = 88;
        decoration_y = 167;
        NISTextDraw(OPENING_SCENE, pending_decoration = 1);
        if (!interrupted)
            interrupted = Wait(0, 90);
    }
    if (!interrupted) {
        NISTextDraw(OPENING_SCENE, 2);
        interrupted = WaitCue(OPENING_SCENE + 1);
    }
    if (!interrupted) {
        Timer[0] = 45;
        hGetSnd(OPENING_SCENE + 2);
        interrupted = WaitTimer(0);
        if (!interrupted) interrupted = NISDialog(OPENING_SCENE, 3, 2, 184);
    }
    if (!interrupted) {
        Timer[0] = 85;
        hGetSnd(OPENING_SCENE + 3);
        WaitTimer(0);
        SetWaitSnd(OPENING_SCENE + 3, 254);
        Timer[0] = 170;
        NISTextDraw(OPENING_SCENE, 4);
        hFree(hNISPal);
        hNISPal = HGETRSRC(0x50414c54UL, OPENING_SCENE + 2);
        ClearNIS(&game_port, 0);
        AddNIS(&game_port, shapes_handle, 7, 39, 31, 0);
        interrupted = WaitTimer(0);
        if (!interrupted) {
            NISTextDraw(OPENING_SCENE, 5);
            interrupted = FadeTo(0, -2, 0);
            if (!interrupted) {
                CopyBits(Graf, &game_port, &nis_clip, &nis_clip, 0);
                interrupted = WaitMidiPlayTime(100);
                NISTextDraw(0, 0);
                if (!interrupted) interrupted = FadeTo(hNISPal, -1, 0);
            }
        }
    }
    if (!interrupted) {
        hGetSnd(OPENING_SCENE + 4);
        interrupted = WaitMidiPlayTime(101);
        if (!interrupted) interrupted = NISDialog(OPENING_SCENE, 6, 4, 223);
        if (!interrupted) interrupted = Wait(0, 20);
    }
    if (!interrupted) interrupted = NISDialog(OPENING_SCENE, 7, 5, 135);
    if (!interrupted) {
        interrupted = WaitMidiPlayTime(102);
        SetWaitSnd(OPENING_SCENE + 6, 197);
        NISTextDraw(OPENING_SCENE, 8); if (!interrupted) interrupted = Wait(0, 150);
    }
    if (!interrupted) {
        AddNIS(&game_port, shapes_handle, 8, 40, 31, 10); GetSRect(&bounds, shapes_handle, 8, 40, 31);
        interrupted = DissolveNIS(&game_port, 45, &bounds); if (!interrupted) interrupted = WaitCue(OPENING_SCENE + 6);
        if (!interrupted) interrupted = Wait(0, 30);
    }
    if (!interrupted) {
        interrupted = NISDialog(OPENING_SCENE, 9, 7, 131);
        if (!interrupted) interrupted = Wait(0, 20);
    }
    if (!interrupted) {
        NISTextDraw(0, 0);
        ClearNIS(&game_port, 0);
        AddNIS(&game_port, shapes_handle, 10, 39, 30, 0);
        interrupted = DissolveNIS(&game_port, 60, &nis_clip);
        if (!interrupted) interrupted = Wait(0, 18);
    }
    if (!interrupted) {
        interrupted = NISDialog(OPENING_SCENE, 10, 8, 250);
        if (!interrupted) interrupted = Wait(0, 20);
    }
    if (!interrupted) {
        SetWaitSnd(OPENING_SCENE + 9, 236);
        NISTextDraw(OPENING_SCENE, 11);
        AddNIS(&game_port, shapes_handle, 9, 39, 30, 10);
        interrupted = DissolveNIS(&game_port, 45, &nis_clip);
        if (!interrupted) interrupted = WaitCue(OPENING_SCENE + 9);
    }
    if (!interrupted) {
        hFree(hNISPal);
        hNISPal = HGETRSRC(0x50414c54UL, OPENING_SCENE + 3);
        ClearNIS(&game_port, 0);
        AddNIS(&game_port, shapes_handle, 12, 40, 32, 0);
        interrupted = WaitSnd(25010);
        if (!interrupted) {
            NISTextDraw(0, 0);
            interrupted = FadeNIS(&game_port, 0);
            midi_cue = 0;
            if (!interrupted) {
                SetSnd(25011);
            }
        }
    }
    if (!interrupted) {
        interrupted = NISDialog(OPENING_SCENE, 12, 10, 124);
        if (!interrupted) interrupted = Wait(0, 20);
    }
    if (!interrupted) {
        interrupted = NISDialog(OPENING_SCENE, 13, 11, 117);
        if (!interrupted) interrupted = Wait(0, 20);
    }
    if (!interrupted) interrupted = NISDialog(OPENING_SCENE, 14, 12, 176);
    if (!interrupted) {
        NISTextDraw(0, 0);
        hFree(hNISPal);
        hNISPal = HGETRSRC(0x50414c54UL, OPENING_SCENE + 4);
        ClearNIS(&game_port, 0);
        AddNIS(&game_port, shapes_handle, 11, 40, 32, 0);
        interrupted = WaitMidiPlayTime(97);
        if (!interrupted) interrupted = FadeNIS(&game_port, 0);
    }
    if (!interrupted) {
        hGetSnd(OPENING_SCENE + 13);
        interrupted = WaitMidiPlayTime(98);
        if (!interrupted) interrupted = NISDialog(OPENING_SCENE, 15, 13, 208); if (!interrupted) interrupted = Wait(0, 20);
    }
    if (!interrupted) {
        hGetSnd(OPENING_SCENE + 14);
        interrupted = WaitMidiPlayTime(99);
        if (!interrupted) interrupted = NISDialog(OPENING_SCENE, 16, 14, 145); if (!interrupted) interrupted = Wait(0, 90);
    }
    hFree(hNISPal);
    ForceFadeOut();
    if (interrupted) {
        ResetSound(0L);
    }
    FreeDialog();
    hFree(shapes_handle);
    FreePort(&game_port); return interrupted;
}

int Opening2(void)
{
    Handle background, left_actor, right_sprite;
    int was_interrupted;
    Rect curr_bounds;
    NisTemporaryPort temporary_port;
    int slide_step;
    Rect next_rect;
    Handle foreground, anim_table;

    BackPort(&temporary_port, &nis_clip);
    hBorderSL = hGetSL(25000, 1);
    anim_table = hGetSL(OPENING_SCENE, -1);
    GetDialog();
    SetDACBlock(0, 256, 0L, 1);
    PaintRLEShapeItem(hBorderSL, 1, 0, 0, 0);
    NISTextDraw(0, 0);
    hNISPal = HGETRSRC(0x50414c54UL, OPENING_SCENE + 5);
    was_interrupted = WaitMidiPlayTime(106);
    if (!was_interrupted) {
        was_interrupted = FadeTo(hNISPal, 1, 2);
        if (!was_interrupted) {
            AddNIS(&temporary_port, anim_table, 13, 9, 31, 0);
            AddNIS(&temporary_port, anim_table, 14, 160, 32, 10);
            AddNIS(&temporary_port, anim_table, 15, 193, 55, 10);
            GetSRect(&curr_bounds, anim_table, 15, 193, 55);
            CopyBits(Graf, &temporary_port, &nis_clip, &nis_clip, 0);
            hGetSnd(25012);
            was_interrupted = WaitSnd(25011);
        }
        if (!was_interrupted) {
            midi_cue = 0;
            SetSnd(25012);
            was_interrupted = FadeTo(hNISPal, -1, 0);
        }
    }
    if (!was_interrupted) {
        decoration_shape = 3;
        decoration_x = 99;
        decoration_y = 167;
        pending_decoration = 1;
        hGetSnd(OPENING_SCENE + 15);
        was_interrupted = WaitMidiPlayTime(97);
        if (!was_interrupted) {
            SetWaitSnd(OPENING_SCENE + 15, 231);
            NISTextDraw(OPENING_SCENE, 17);
            was_interrupted = Wait(0, 60);
            if (!was_interrupted)
                NISTextDraw(OPENING_SCENE, 18);
        }
    }
    if (!was_interrupted) {
        ClearNIS(&temporary_port, 0);
        AddNIS(&temporary_port, anim_table, 13, 9, 31, 0);
        AddNIS(&temporary_port, anim_table, 14, 160, 32, 0);
        AddNIS(&temporary_port, anim_table, 16, 192, 55, 10);
        GetSRect(&next_rect, anim_table, 16, 192, 55);
        hGetSnd(OPENING_SCENE + 16);
        WaitCue(OPENING_SCENE + 15);
        was_interrupted = WaitMidiPlayTime(98);
        if (!was_interrupted) {
            SetWaitSnd(OPENING_SCENE + 16, 164);
            NISTextDraw(OPENING_SCENE, 19);
            was_interrupted = DissolveNIS(&temporary_port, 300, UNIONRECT(&curr_bounds, &curr_bounds, &next_rect));
        }
    }
    if (!was_interrupted)
        WaitSnd(0);
    if (!was_interrupted) {
        NISTextDraw(0, 0);
        background = hNewShpItem1(anim_table, 17, 1);
        left_actor = hNewShpItem1(anim_table, 13, 1);
        right_sprite = hNewShpItem1(anim_table, 14, 1);
        foreground = hNewShpItem1(anim_table, 16, 1);
        SetSnd(30503);
        for (slide_step = 1; slide_step < 17; ++slide_step) {
            if (!was_interrupted) {
                Timer[0] = 14 - slide_step < 0 ? 0 : 14 - slide_step;
                Graf = &temporary_port;
                PaintRLEShape_asm(h2p(background), 39, 32, 0);
                PaintRLEShape_asm(h2p(left_actor), 9 - (slide_step << 3), 31, 0);
                PaintRLEShape_asm(h2p(right_sprite), (slide_step + 20) << 3, 32, 0);
                PaintRLEShape_asm(h2p(foreground), 192, 55, 10);
                Graf = &ScreenPort;
                CutNIS(&temporary_port);
                was_interrupted = WaitTimer(0);
            }
        }
        hFree(foreground);
        hFree(right_sprite);
        hFree(left_actor);
        hFree(background);
        if (!was_interrupted) was_interrupted = WaitSnd(30503);
    }
    if (!was_interrupted) {
        midi_cue = 0; SetSnd(25013);
        SetWaitSnd(OPENING_SCENE + 17, 240);
        NISTextDraw(OPENING_SCENE, 20);
    }
    if (!was_interrupted) {
        ClearNIS(&temporary_port, 0);
        AddNIS(&temporary_port, anim_table, 18, 40, 32, 0);
        was_interrupted = DissolveNIS(&temporary_port, 60, &nis_clip);
        if (!was_interrupted) was_interrupted = Wait(was_interrupted, 42);
    }
    if (!was_interrupted) {
        NISTextDraw(OPENING_SCENE, 21);
        ClearNIS(&temporary_port, 0);
        AddNIS(&temporary_port, anim_table, 19, 39, 32, 0);
        was_interrupted = DissolveNIS(&temporary_port, 60, &nis_clip);
        if (!was_interrupted) was_interrupted = Wait(was_interrupted, 42);
    }
    if (!was_interrupted) was_interrupted = WaitCue(OPENING_SCENE + 17);
    if (!was_interrupted) {
        ClearNIS(&temporary_port, 0);
        AddNIS(&temporary_port, anim_table, 20, 40, 32, 0);
        was_interrupted = DissolveNIS(&temporary_port, 60, &nis_clip);
        if (!was_interrupted) was_interrupted = Wait(was_interrupted, 42);
    }
    if (!was_interrupted) {
        ClearNIS(&temporary_port, 0);
        AddNIS(&temporary_port, anim_table, 21, 40, 31, 0);
        AddNIS(&temporary_port, anim_table, 24, 126, 44, 10);
        AddNIS(&temporary_port, anim_table, 23, 65, 79, 10);
        AddNIS(&temporary_port, anim_table, 25, 167, 50, 10);
        was_interrupted = DissolveNIS(&temporary_port, 60, &nis_clip);
        if (!was_interrupted) {
            NISTextDraw(was_interrupted, was_interrupted);
            was_interrupted = Wait(0, 42);
        }
    }
    if (!was_interrupted) {
        was_interrupted = NISDialog(OPENING_SCENE, 22, 18, 182);
        if (!was_interrupted) was_interrupted = Wait(was_interrupted, 20);
    }
    if (!was_interrupted) {
        hFree(hNISPal);
        hNISPal = HGETRSRC(0x50414c54UL, OPENING_SCENE + 6);
        NISTextDraw(0, 0);
        ClearNIS(&temporary_port, 0);
        AddNIS(&temporary_port, anim_table, 33, 39, 32, 0);
        AddNIS(&temporary_port, anim_table, 34, 40, 32, 10);
        if (!was_interrupted) was_interrupted = WaitMidiPlayTime(97);
        CopyNIS(&temporary_port);
        was_interrupted = Wait(0, 48);
    }
    if (!was_interrupted) was_interrupted = WaitMidiPlayTime(98);
    if (!was_interrupted) {
        was_interrupted = NISDialog(OPENING_SCENE, 23, 19, 274);
        if (!was_interrupted) was_interrupted = Wait(was_interrupted, 20);
    }
    if (!was_interrupted) {
        background = hNewShpItem1(anim_table, 21, 1);
        left_actor = hNewShpItem1(anim_table, 22, 1);
        right_sprite = hNewShpItem1(anim_table, 25, 1);
        hFree(hNISPal);
        hNISPal = HGETRSRC(0x50414c54UL, OPENING_SCENE + 5);
        NISTextDraw(0, 0);
        AddShpNIS(&temporary_port, background, 40, 31, 0);
        AddNIS(&temporary_port, anim_table, 26, 113, 51, 10);
        AddShpNIS(&temporary_port, left_actor, 58, 78, 10);
        AddShpNIS(&temporary_port, right_sprite, 167, 50, 10);
        was_interrupted = WaitMidiPlayTime(100);
        if (!was_interrupted)
            CopyNIS(&temporary_port);
    } else {
        background = 0;
    }
    if (!was_interrupted) {
        AddShpNIS(&temporary_port, background, 40, 31, 0);
        AddNIS(&temporary_port, anim_table, 27, 113, 50, 10);
        AddShpNIS(&temporary_port, left_actor, 58, 78, 10);
        AddShpNIS(&temporary_port, right_sprite, 167, 50, 10);
        CutNIS(&temporary_port);
    }
    if (!was_interrupted) {
        AddShpNIS(&temporary_port, background, 40, 31, 0);
        AddNIS(&temporary_port, anim_table, 28, 115, 50, 10);
        AddShpNIS(&temporary_port, left_actor, 58, 78, 10);
        AddShpNIS(&temporary_port, right_sprite, 167, 50, 10);
        CutNIS(&temporary_port);
        was_interrupted = Wait(0, 6);
    }
    if (!was_interrupted) {
        AddShpNIS(&temporary_port, background, 40, 31, 0);
        AddNIS(&temporary_port, anim_table, 29, 113, 48, 10);
        AddShpNIS(&temporary_port, left_actor, 58, 78, 10);
        AddShpNIS(&temporary_port, right_sprite, 167, 50, 10);
        CutNIS(&temporary_port);
        was_interrupted = Wait(0, 6);
    }
    if (!was_interrupted) {
        AddShpNIS(&temporary_port, background, 40, 31, 0);
        AddNIS(&temporary_port, anim_table, 30, 112, 48, 10);
        AddShpNIS(&temporary_port, left_actor, 58, 78, 10);
        AddShpNIS(&temporary_port, right_sprite, 167, 50, 10);
        CutNIS(&temporary_port);
        was_interrupted = Wait(0, 6);
    }
    if (!was_interrupted) {
        AddShpNIS(&temporary_port, background, 40, 31, 0);
        AddNIS(&temporary_port, anim_table, 31, 112, 46, 10);
        AddShpNIS(&temporary_port, left_actor, 58, 78, 10);
        AddShpNIS(&temporary_port, right_sprite, 167, 50, 10);
        CutNIS(&temporary_port);
        was_interrupted = Wait(0, 6);
    }
    if (!was_interrupted) {
        AddShpNIS(&temporary_port, background, 40, 31, 0);
        AddNIS(&temporary_port, anim_table, 32, 89, 47, 10);
        AddShpNIS(&temporary_port, left_actor, 58, 78, 10);
        AddShpNIS(&temporary_port, right_sprite, 167, 50, 10);
        CutNIS(&temporary_port);
        was_interrupted = Wait(0, 18);
    }
    if (background) {
        hFree(right_sprite);
        hFree(left_actor);
        hFree(background);
    }
    if (!was_interrupted) {
        if (!was_interrupted) was_interrupted = NISDialog(OPENING_SCENE, 24, 20, 129);
        if (!was_interrupted) was_interrupted = Wait(0, 20);
    }
    if (!was_interrupted) {
        hFree(hNISPal);
        hNISPal = HGETRSRC(0x50414c54UL, OPENING_SCENE + 6);
        NISTextDraw(0, 0);
        ClearNIS(&temporary_port, 0);
        AddNIS(&temporary_port, anim_table, 35, 40, 32, 0);
        CopyNIS(&temporary_port);
        midi_cue = 0;
        SetSnd(25014);
        was_interrupted = NISDialog(OPENING_SCENE, 25, 21, 174);
    }
    if (!was_interrupted) {
        NISTextDraw(0, 0);
        ClearNIS(&temporary_port, 0);
        AddNIS(&temporary_port, anim_table, 33, 39, 32, 0);
        AddNIS(&temporary_port, anim_table, 36, 72, 39, 10);
        CopyNIS(&temporary_port);
    }
    if (!was_interrupted) {
        Timer[0] = 12;
        ClearNIS(&temporary_port, 0);
        AddNIS(&temporary_port, anim_table, 33, 39, 32, 0);
        AddNIS(&temporary_port, anim_table, 37, 46, 38, 10);
        was_interrupted = WaitTimer(0);
        if (!was_interrupted) {
            SetWaitSnd(OPENING_SCENE + 22, 60);
            NISTextDraw(OPENING_SCENE, 26);
            CutNIS(&temporary_port);
            was_interrupted = WaitCue(OPENING_SCENE + 22);
            if (!was_interrupted) was_interrupted = Wait(was_interrupted, 30);
        }
    }
    if (!was_interrupted) {
        NISTextDraw(0, 0);
        ClearNIS(&temporary_port, 0);
        AddNIS(&temporary_port, anim_table, 38, 41, 32, 0);
        CopyNIS(&temporary_port);
        if (!was_interrupted) was_interrupted = Wait(0, 18);
    }
    if (!was_interrupted) {
        was_interrupted = NISDialog(OPENING_SCENE, 27, 23, 98);
        if (!was_interrupted) was_interrupted = Wait(was_interrupted, 20);
    }
    if (!was_interrupted) {
        SetWaitSnd(OPENING_SCENE + 24, 236);
        NISTextDraw(OPENING_SCENE, 28);
        ClearNIS(&temporary_port, 0);
        AddNIS(&temporary_port, anim_table, 33, 39, 32, 0);
        AddNIS(&temporary_port, anim_table, 39, 40, 32, 10);
        CopyNIS(&temporary_port);
    }
    if (!was_interrupted) was_interrupted = WaitCue(OPENING_SCENE + 24);
    if (!was_interrupted) {
        NISTextDraw(0, 0);
        ClearNIS(&temporary_port, 0);
        AddNIS(&temporary_port, anim_table, 43, 40, 32, 0);
        AddNIS(&temporary_port, anim_table, 40, 128, 31, 10);
        GetSRect(&curr_bounds, anim_table, 40, 128, 31);
        CopyNIS(&temporary_port);
        if (!was_interrupted) was_interrupted = Wait(0, 18);
    }
    if (!was_interrupted) {
        was_interrupted = NISDialog(OPENING_SCENE, 29, 25, 97);
        if (!was_interrupted) was_interrupted = Wait(was_interrupted, 18);
    }
    if (!was_interrupted) {
        ClearNIS(&temporary_port, 0);
        AddNIS(&temporary_port, anim_table, 43, 40, 32, 0);
        AddNIS(&temporary_port, anim_table, 41, 125, 28, 10);
        GetSRect(&next_rect, anim_table, 41, 125, 28);
        midi_cue = 0;
        SetSnd(25015);
        SetWaitSnd(OPENING_SCENE + 26, 93);
        NISTextDraw(OPENING_SCENE, 30);
        was_interrupted = DissolveNIS(&temporary_port, 75, UNIONRECT(&curr_bounds, &curr_bounds, &next_rect));
        if (!was_interrupted)
            was_interrupted = WaitCue(OPENING_SCENE + 26);
        if (!was_interrupted) was_interrupted = Wait(was_interrupted, 30);
    }
    if (!was_interrupted) {
        ClearNIS(&temporary_port, 0);
        AddNIS(&temporary_port, anim_table, 43, 40, 32, 0);
        AddNIS(&temporary_port, anim_table, 42, 125, 32, 10);
        GetSRect(&curr_bounds, anim_table, 42, 125, 32);
        was_interrupted = DissolveNIS(&temporary_port, 75, UNIONRECT(&curr_bounds, &curr_bounds, &next_rect));
        if (!was_interrupted) was_interrupted = Wait(was_interrupted, 30);
    }
    if (!was_interrupted) {
        hFree(hNISPal);
        hNISPal = HGETRSRC(0x50414c54UL, OPENING_SCENE + 7);
        NISTextDraw(0, 0);
        ClearNIS(&temporary_port, 0);
        AddNIS(&temporary_port, anim_table, 45, 40, 32, 0);
        CopyNIS(&temporary_port);
        was_interrupted = Wait(0, 48);
    }
    if (!was_interrupted) {
        AddNIS(&temporary_port, anim_table, 44, 82, 38, 10);
        GetSRect(&curr_bounds, anim_table, 44, 82, 38);
        hGetSnd(10049);
        ResetSnd(0); SetSnd(10049);
        was_interrupted = DissolveNIS(&temporary_port, 90, &curr_bounds);
        if (!was_interrupted) was_interrupted = Wait(was_interrupted, 30);
        if (!was_interrupted) was_interrupted = FadeTo(was_interrupted, -1, 2);
    }
    hFree(hNISPal);
    ForceFadeOut();
    if (was_interrupted) ResetSound(0L);
    FreeDialog(); hFree(anim_table);
    FreePort(&temporary_port);
    return was_interrupted;
}






extern void PaintRLEShape_asm(const void far *shape, int x, int y, int mode);

void ClearNIS(DrawPort *port, int color)
{
    DrawPort *saved = Graf;

    Graf = port;
    FillRect(&nis_clip, color);
    Graf = saved;
}

void AddNIS(DrawPort *port, Handle shapes, int item, int x, int y, int mode)
{
    DrawPort *saved = Graf;

    Graf = port;
    PaintRLEShapeItem(shapes, item, x, y, mode);
    Graf = saved;
}

void AddShpNIS(DrawPort *port, Handle shape, int x, int y, int mode)
{
    DrawPort *saved = Graf;

    Graf = port;
    PaintRLEShape_asm(h2p(shape), x, y, mode);
    Graf = saved;
}

void CopyNIS(const void *source)
{
    SetDACBlocks(-2, 0L);
    CopyBits(Graf, source, &nis_clip, &nis_clip, 0);
    Wait(0, 2);
    SetDACBlocks(-1, h2p(hNISPal));
}

void CutNIS(const void *source)
{
    CopyBits(Graf, source, &nis_clip, &nis_clip, 0);
}

int FadeNIS(const void *source, int duration)
{
    int interrupted = FadeTo(0, -2, duration);

    if (!interrupted)
        CopyBits(Graf, source, &nis_clip, &nis_clip, 0);
    if (!interrupted)
        interrupted = FadeTo(hNISPal, -1, duration);
    return interrupted;
}

int DissolveNIS(DrawPort *port, unsigned int duration, const Rect far *bounds)
{
    char saved_state[44];
    int interrupted;
    DrawPort *saved = Graf;

    Graf = BackPort(saved_state, bounds);
    CopyBits((DrawPort *)saved_state, &ScreenPort, bounds, bounds, 0);
    interrupted = DissolveDoAll(1, port, bounds, duration);
    Graf = saved;
    FreePort(saved_state);
    return interrupted;
}

int HappyEnding(void);
int discovery(void);

int NIS(int sequence)
{
    int interrupted;

    open_level_resources(nis_data_files);
    InitNISInput();
    {
        int opened_nis_file;

        if (sequence != 11) {
            opened_nis_file = 1;
            OpenRsrcVerify(nis_file);
        } else {
            opened_nis_file = 0;
        }

        if (sequence == 7)
            interrupted = Opening1();
        else if (sequence == 8)
            interrupted = Opening2();
        else if (sequence == 9)
            interrupted = AboardShip();
        else if (sequence == 10)
            interrupted = discovery();
        else if (sequence == 11)
            interrupted = HappyEnding();

        if (opened_nis_file)
            CloseRsrcFile(nis_file);
    }
    WaitFunc = SaveWaitFunc;
    close_level_resources(nis_data_files);
    return interrupted;
}

extern Handle hGetSnd(int cue);

int NISDialog(int resource, int line, int voice, unsigned int duration)
{
    int cue;
    hGetSnd(resource + voice);
    cue = resource + voice;
    NISTextDraw(resource, line);
    return WaitSnd1(cue, duration);
}

Rect *GetSRect(Rect far *bounds, Handle shapes, int item, int x, int y)
{
    const unsigned int far *list = h2p(shapes);
    const Rect far *shape = h2p(HGETRSRC(0x53484150UL, list[0] + item - 1));

    bounds->left = x;
    bounds->top = y;
    bounds->right = x + shape->left;
    bounds->bottom = y + shape->top;
    return (Rect *)bounds;
}

extern void select_resource_file(const char *name);
extern void ClipRect(const Rect far *bounds);
extern void ClipRestore(void);
extern void RestoreRect(ImagePort *patch);
extern Handle hPurge(Handle shape);
extern Rect ending_patch_bounds;

extern const char nis_music_file[];
int JaffarBurnBabyBurn(void);
int HappyFunc(AnimationState *playback, int frame);
int DiscFunc(AnimationState *playback, int frame);

enum { ENDING_SCENE = 30000, DISCOVERY_SCENE = 29000 };

int HappyEnding(void)
{
    int interrupted;
    ImagePort *portrait_patch;
    Rect bounds;
    NisTemporaryPort game_port;

    JaffarBurnBabyBurn();
    OpenRsrcVerify(nis_file);
    BackPort(&game_port, &nis_clip);
    hBorderSL = hGetSL(25000, 1);
    hHappySL = hGetSL(ENDING_SCENE, -1);
    GetDialog();
    SetDACBlock(0, 256, 0L, 1);
    PaintRLEShapeItem(hBorderSL, 1, 0, 0, 0);
    ClipRect(&nis_clip);
    PaintRLEShapeItem(hHappySL, 2, 40, 32, 0);
    PaintRLEShapeItem(hHappySL, 5, 52, 90, 10);
    ClipRestore();
    NISTextDraw(0, 0);
    CopyBits(&game_port, Graf, &nis_clip, &nis_clip, 0);
    hNISPal = HGETRSRC(0x50414c54UL, ENDING_SCENE + 1);
    SetSnd(ENDING_SCENE + 10);
    interrupted = ((int (far pascal *)(Handle, int, unsigned int))FadeTo)(hNISPal, -1, 2U);
    if (!interrupted) {
        interrupted = Wait(0, 48);
    }
    if (!interrupted) {
        AddNIS(&game_port, hHappySL, 3, 160, 45, 10);
        GetSRect(&bounds, hHappySL, 3, 160, 45);
        interrupted = WaitMidiPlayTime(97);
        if (!interrupted) interrupted = DissolveNIS(&game_port, 15, &bounds);
        if (!interrupted) interrupted = PlayAnim(hHappySL, ENDING_SCENE, 2, 1, HappyFunc, 0, 0, 1);
        if (!interrupted) Wait(0, 40);
    }
    if (!interrupted) interrupted = WaitMidiPlayTime(98);
    if (!interrupted) {
        decoration_shape = 7;
        decoration_x = 77; decoration_y = 159;
        pending_decoration = 1;
        SetWaitSnd(ENDING_SCENE + 1, 181);
        NISTextDraw(ENDING_SCENE, 1);
        AddNIS(&game_port, hHappySL, 7, 40, 32, 0);
        GetSRect(&bounds, hHappySL, 44, 161, 64);
        portrait_patch = SaveRect(&bounds);
        hGetSnd(ENDING_SCENE + 2);
        PaintRLEShapeItem(hHappySL, 44, 161, 64, 10);
        Timer[0] = 6; interrupted = WaitTimer(0);
        RestoreRect(portrait_patch);
        if (!interrupted) interrupted = Wait(0, 60);
        if (!interrupted) interrupted = FadeNIS(&game_port, 0);
        if (!interrupted) interrupted = WaitCue(ENDING_SCENE + 1);
        if (!interrupted) interrupted = Wait(0, 40);
    }
    if (!interrupted) interrupted = WaitMidiPlayTime(99);
    if (!interrupted) {
        SetWaitSnd(ENDING_SCENE + 2, 278);
        NISTextDraw(ENDING_SCENE, 2);
        if (!interrupted)
            interrupted = Wait(0, 90);
    }
    if (!interrupted) {
        NISTextDraw(ENDING_SCENE, 3);
        interrupted = WaitCue(ENDING_SCENE + 2);
        if (!interrupted) interrupted = Wait(0, 90);
    }
    if (!interrupted) interrupted = WaitMidiPlayTime(100);
    if (!interrupted) interrupted = FadeTo(0, -1, 3);
    if (!interrupted) {
        FillRect(&ending_fill_bounds, 48);
        PaintRLEShapeItem(hHappySL, 8, 0, 6, 0);
        alternate_port = SaveRect(&dialog_rect);
        hFlyingPal = HGETRSRC(0x50414c54UL, ENDING_SCENE + 2);
        hRosyPal = HGETRSRC(0x50414c54UL, ENDING_SCENE + 3);
        hIrisPal = HGETRSRC(0x50414c54UL, ENDING_SCENE + 4);
        hWitchPal = HGETRSRC(0x50414c54UL, ENDING_SCENE + 5);
        hGetSnd(ENDING_SCENE + 4);
        hGetSnd(ENDING_SCENE + 5);
        hGetSnd(ENDING_SCENE + 6);
        hGetSnd(ENDING_SCENE + 7);
        hGetSnd(ENDING_SCENE + 8);
        hWitchShp = hPurge(hNewShpItem1(hHappySL, 10, 1));
        interrupted = PlayAnim(hHappySL, ENDING_SCENE, 1, 1, HappyFunc, 0, 0, 1);
        hFree(hWitchShp);
        hFree(hWitchPal);
        hFree(hIrisPal);
        hFree(hRosyPal); hFree(hFlyingPal);
        RestoreRect(alternate_port);
    }
    ForceFadeOut();
    if (interrupted)
        ResetSound(0L);
    hFree(hNISPal); FreeDialog();
    hFree(hHappySL); FreePort(&game_port);
    CloseRsrcFile(nis_file);
    return interrupted;
}

int JaffarBurnBabyBurn(void)
{
    Handle foreground;
    int interrupted;

    SetDACBlock(0, 256, 0L, 1);
    InitNISInput();
    hTranSL = hGetSL(25000, -1);
    DrawPrincessRoom(hTranSL);
    foreground = hGetSL(25001, 1);
    PaintShapeItem(foreground, 15, 166, 139, 10, 0);
    hFree(foreground);
    hTransPalP = HGETRSRC(0x50414c54UL, 25000);
    interrupted = PlayAnim(hTranSL, 25000, 2, 1, TransFunc4, 0, 0, 1);
    hFree(hTransPalP);
    hFree(hTranSL);
    if (!interrupted)
        interrupted = Wait(0, 60);
    if (!interrupted)
        interrupted = FadeTo(0, -1, 0);
    return interrupted;
}

extern void HappyDialog(int resource, int line, int voice);
extern void SwapBackGround(AnimationState *);
extern void far *MATERIALIZE_HANDLE(Handle);

int HappyFunc(AnimationState *playback, int frame)
{
    Rect bounds;
    int aborted = 0;
    DrawPort *saved_port;
    ImagePort *patch;

    if (frame == 0) {
        aborted = WaitMidiPlayTime(101);
        if (!aborted) {
            playback->fade = InitFadeTo(hFlyingPal, -1, 1);
            HappyDialog(ENDING_SCENE, 4, 4);
        }
    } else if (frame == 1) {
        playback->fade = InitFadeTo(hRosyPal, 7, 1);
    } else if (frame == 2) {
        saved_port = Graf;
        CopyBits(Graf = &ScreenPort, alternate_port,
                  &alternate_port->clip, &alternate_port->clip, 0);
        Graf = saved_port;
        playback->fade = InitFadeTo(hIrisPal, -8, 2);
    } else if (frame == 3) {
        if (!MATERIALIZE_HANDLE(hWitchShp)) {
            hFree(hWitchShp);
            hWitchShp = hPurge(hNewShpItem1(hHappySL, 10, 1));
        }
        SwapBackGround(playback);
        PaintRLEShape_asm(h2p(hWitchShp), -playback->port.clip.left,
                  8 - playback->port.clip.top, 0);
        SwapBackGround(playback);

        saved_port = Graf;
        Graf = &ScreenPort;
        FillRect(&ending_fill_bounds, 0);
        PaintRLEShape_asm(h2p(hWitchShp), 0, 8, 10);
        Graf = saved_port;
        SetSnd(ENDING_SCENE + 20);
        playback->fade = InitFadeTo(hWitchPal, -1, 1);
    } else if (frame == 4) {
        InitFadeToBlack(playback, 1);
    } else if (frame == 5) {
        HappyDialog(ENDING_SCENE, 5, 5);
    } else if (frame == 6) {
        HappyDialog(ENDING_SCENE, 6, 6);
    } else if (frame == 7) {
        HappyDialog(ENDING_SCENE, 7, 7);
    } else if (frame == 8) {
        HappyDialog(ENDING_SCENE, 8, 8);
    } else if (frame == 98) {
        SwapBackGround(playback);
        PaintRLEShapeItem(hHappySL, 2, 40 - playback->port.clip.left,
                          32 - playback->port.clip.top, 0);
        SwapBackGround(playback);
    } else if (frame == 99) {
        saved_port = Graf;
        Graf = &ScreenPort;
        GetSRect(&bounds, hHappySL, 44, 161, 64);
        patch = SaveRect(&bounds);
        PaintRLEShapeItem(hHappySL, 44, 161, 64, 10);
        Timer[0] = 6;
        while (Timer[0])
            ;
        RestoreRect(patch);
        Graf = saved_port;
    }
    return aborted;
}

void HappyDialog(int resource, int line, int voice)
{
    DrawPort *saved = Graf;

    Graf = &ScreenPort;
    SetSnd(resource + voice);
    alternate_dialog = 1;
    NISTextDraw(resource, line);
    alternate_dialog = 0;
    Graf = saved;
}

extern int POLL_DOS_KEY(void);
extern void toggle_sound(void);
extern void read_controls(void);
extern struct { int y, x, center_y, center_x; unsigned char button; } joystick_state;

int NISInput(void)
{
    int key = POLL_DOS_KEY();

    if (key == 0x1000 || key == 0x11)
        abort_with_message(0, 0L);
    if (key == 0x1f00) {
        toggle_sound();
        key = 0;
    }
    if (key == 0x2600)
        return nis_skip_requested = 1;
    if (key == 27 || key == 32) {
        return 1;
    } else {
        read_controls();
        if (joystick_state.button) {
            if (!NISButtonDown)
                return 1;
        } else {
            NISButtonDown = 0;
        }
        return 0;
    }
}

void InitNISInput(void)
{
    SaveWaitFunc = WaitFunc;
    WaitFunc = NISInput;
    read_controls();
    NISButtonDown = joystick_state.button != 0;
}

int discovery(void)
{
    int aborted;
    Rect port_rect, next_bounds;
    NisTemporaryPort port;
    unsigned char white[240][3];

    SetSnd(29100);
    BackPort(&port, &nis_clip);
    hBorderSL = hGetSL(25000, 1);
    hDiscSL = hGetSL(DISCOVERY_SCENE, -1);
    GetDialog();
    SetDACBlock(0, 256, 0L, 1);
    PaintRLEShapeItem(hBorderSL, 1, 0, 0, 0);
    NISTextDraw(0, 0);
    PaintRLEShapeItem(hDiscSL, 2, 40, 32, 0);
    hNISPal = HGETRSRC(0x50414c54UL, DISCOVERY_SCENE + 1);
    aborted = FadeTo(hNISPal, -1, 0);
    if (!aborted)
        aborted = Wait(0, 240);
    if (!aborted) {
        SetWaitSnd(DISCOVERY_SCENE + 1, 270);
        decoration_shape = 6;
        decoration_x = 96;
        decoration_y = 168;
        NISTextDraw(DISCOVERY_SCENE, pending_decoration = 1);
        if (!aborted)
            aborted = Wait(0, 60);
    }
    if (!aborted) {
        NISTextDraw(DISCOVERY_SCENE, 2);
        aborted = WaitCue(DISCOVERY_SCENE + 1);
        if (!aborted)
            aborted = Wait(0, 90);
    }
    if (!aborted) {
        aborted = NISDialog(DISCOVERY_SCENE, 3, 2, 157);
        if (!aborted) aborted = Wait(0, 40);
    }
    if (!aborted) {
        aborted = NISDialog(DISCOVERY_SCENE, 4, 3, 187);
        if (!aborted) aborted = Wait(0, 20);
    }
    if (!aborted) {
        NISTextDraw(0, 0); ClearNIS(&port, 0);
        AddNIS(&port, hDiscSL, 3, 40, 32, 0);
        AddNIS(&port, hDiscSL, 4, 51, 40, 10);
        GetSRect(&port_rect, hDiscSL, 4, 51, 40);
        aborted = WaitMidiPlayTime(97);
        if (!aborted) aborted = FadeNIS(&port, 0);
        if (!aborted) aborted = Wait(0, 24);
    }
    if (!aborted) {
        aborted = NISDialog(DISCOVERY_SCENE, 5, 4, 168);
        if (!aborted) aborted = Wait(0, 20);
    }
    if (!aborted) {
        aborted = NISDialog(DISCOVERY_SCENE, 6, 5, 137);
        if (!aborted) aborted = Wait(0, 20);
    }
    if (!aborted) {
        aborted = NISDialog(DISCOVERY_SCENE, 7, 6, 220);
        if (!aborted) aborted = Wait(0, 60);
    }
    if (!aborted) aborted = WaitMidiPlayTime(98);
    if (!aborted) aborted = WaitSnd1(30501, 0);
    if (!aborted) {
        NISTextDraw(0, 0); ClearNIS(&port, 0);
        AddNIS(&port, hDiscSL, 3, 40, 32, 0);
        AddNIS(&port, hDiscSL, 5, 50, 33, 10);
        GetSRect(&next_bounds, hDiscSL, 5, 50, 33);
        if (!aborted) SetSnd(DISCOVERY_SCENE + 7);
        aborted = DissolveNIS(&port, 15, UNIONRECT(&port_rect, &port_rect, &next_bounds));
        if (!aborted) aborted = Wait(0, 30);
    }
    if (!aborted) {
        ClearNIS(&port, 0);
        AddNIS(&port, hDiscSL, 8, 40, 32, 0);
        CopyNIS(&port);
        aborted = Wait(0, 60);
    }
    if (!aborted) {
        ClearNIS(&port, 0);
        AddNIS(&port, hDiscSL, 3, 40, 32, 0);
        AddNIS(&port, hDiscSL, 5, 50, 33, 10);
        CopyNIS(&port);
        aborted = NISDialog(DISCOVERY_SCENE, 8, 8, 66);
        if (!aborted) aborted = Wait(0, 20);
    }
    if (!aborted) {
        SetSnd(30500); ClearNIS(&port, 0);
        AddNIS(&port, hDiscSL, 3, 40, 32, 0);
        AddNIS(&port, hDiscSL, 6, 50, 33, 10);
        NISTextDraw(0, 0); CutNIS(&port);
        aborted = Wait(0, 6);
    }
    if (!aborted) {
        ClearNIS(&port, 0);
        AddNIS(&port, hDiscSL, 3, 40, 32, 0);
        AddNIS(&port, hDiscSL, 7, 40, 32, 10);
        CutNIS(&port);
        aborted = Wait(0, 6);
    }
    if (!aborted) {
        memset(white, 63, sizeof white);
        SetDACBlock(16, 240, white, 1);
        SetSnd(29101); aborted = Wait(0, 6);
    }
    if (!aborted) {
        ClearNIS(&port, 0);
        AddNIS(&port, hDiscSL, 9, 39, 31, 0);
        CopyNIS(&port);
        aborted = Wait(0, 6);
    }
    if (!aborted) {
        ClearNIS(&port, 0);
        AddNIS(&port, hDiscSL, 8, 40, 32, 0);
        CutNIS(&port);
        aborted = Wait(0, 144);
    }
    if (!aborted) {
        AddNIS(&port, hDiscSL, 10, 50, 31, 10);
        GetSRect(&port_rect, hDiscSL, 10, 50, 31);
        aborted = DissolveNIS(&port, 75, &port_rect);
        if (!aborted) aborted = Wait(0, 156);
    }
    if (!aborted) aborted = FadeTo(0, -2, 2);
    if (!aborted) {
        ClearNIS(&port, 0);
        AddNIS(&port, hDiscSL, 11, 40, 32, 0);
        AddNIS(&port, hDiscSL, 13, 191, 89, 10);
        hFree(hNISPal);
        hNISPal = HGETRSRC(0x50414c54UL, DISCOVERY_SCENE + 2);
        if (!aborted)
            aborted = WaitSnd(0);
        if (!aborted)
            SetSnd(29102);
        if (!aborted) aborted = FadeNIS(&port, 2);
        if (!aborted) aborted = Wait(0, 60);
    }
    if (!aborted) {
        aborted = NISDialog(DISCOVERY_SCENE, 9, 9, 286);
        if (!aborted) {
            aborted = Wait(0, 20);
        }
    }
    if (!aborted) {
        aborted = NISDialog(DISCOVERY_SCENE, 10, 10, 286);
        if (!aborted) {
            aborted = Wait(0, 20);
        }
    }
    if (!aborted) {
        aborted = NISDialog(DISCOVERY_SCENE, 11, 11, 200);
        if (!aborted) {
            aborted = Wait(0, 120);
        }
    }
    if (!aborted) {
        aborted = PlayAnim(hDiscSL, DISCOVERY_SCENE, 1, 1, DiscFunc, 0, 0, 1);
    }
    if (!aborted) {
        aborted = PlayAnim(hDiscSL, DISCOVERY_SCENE, 2, 1, DiscFunc, 0, 0, 1);
        if (!aborted) {
            aborted = FadeTo(0, -1, 2);
        }
    }
    hFree(hNISPal);
    ForceFadeOut();
    if (aborted) {
        ResetSound(0L);
    }
    FreeDialog();
    hFree(hDiscSL);
    FreePort(&port);
    return aborted;
}


extern void SwapBackGround(AnimationState *);

int DiscFunc(AnimationState *playback, int frame)
{
    SwapBackGround(playback);
    PaintRLEShapeItem(hDiscSL, 11, 40 - playback->port.clip.left,
                      32 - playback->port.clip.top, 0);
    SwapBackGround(playback);
    return 0;
}

struct TitleAnimationState {
    char header[0x10];
    Rect saved_clip;
    char before_port[0x2e - 0x18];
    ImagePort port;
    Rect active_clip;
    char before_clock[0x5a - 0x4e];
    unsigned int frame_delay;
    unsigned long frame_clock;
};

extern int ForceFuncParam;
extern void DrawFrame(TitleAnimationState *playback);
extern void *cdecl memcpy(void *destination, const void *source, unsigned int length);

int TitleFunc(TitleAnimationState *playback, int frame)
{
    Rect title_bounds;
    int interrupted;
    int old_frame_suppression;
    DrawPort *saved_port;

    interrupted = 0;
    saved_port = Graf;

    if (frame == 0) {
        GetSRect(&title_bounds, hTranSL, 312, 43, 53);
        title_bounds.bottom = playback->port.clip.top;
        Graf = &ScreenPort;
        SkyMS = SaveRect(&title_bounds);
        Graf = saved_port;
        InitFadeToPrimary((AnimationState *)playback, 1);
        interrupted = WaitMidiPlayTime(100);
    } else if (frame == 2) {
        switch (midi_cue) {
        case 101:
            Graf = &ScreenPort;
            PaintRLEShape_asm(h2p(hTitleShp1), 43, 68, 10);
            Graf = saved_port;
            midi_cue = 114;
        case 114:
            PaintRLEShape_asm(h2p(hTitleShp1), 43 - playback->port.clip.left,
                      68 - playback->port.clip.top, 10);
            break;
        case 102:
        case 104:
            while ((TimeVal.words.low | ((unsigned long)TimeVal.words.high << 16)) <
                   playback->frame_clock + playback->frame_delay)
                ;
            CopyBits(Graf = &ScreenPort, SkyMS,
                      &SkyMS->clip, &SkyMS->clip, 0);
            Graf = saved_port;
            memcpy(&playback->active_clip, &playback->saved_clip, sizeof playback->active_clip);
            old_frame_suppression = ForceFuncParam;
            ForceFuncParam = 0;
            DrawFrame(playback);
            ForceFuncParam = old_frame_suppression;
            midi_cue = 120;
            break;
        case 103:
            Graf = &ScreenPort;
            PaintRLEShape_asm(h2p(hTitleShp2), 58, 53, 10);
            Graf = saved_port;
            midi_cue = 115;
        case 115:
            PaintRLEShape_asm(h2p(hTitleShp2), 58 - playback->port.clip.left,
                      53 - playback->port.clip.top, 10);
            break;
        case 105:
            InitFadeToBlack((AnimationState *)playback, 1);
            midi_cue = 98;
            break;
        }
    }
    if (midi_cue == 98 && !((AnimationState *)playback)->fade)
        interrupted = 1;
    return interrupted;
}

void nis_noop(void)
{
}

int query_nis_sound(void)
{
    return query_sound_payload(0L);
}
