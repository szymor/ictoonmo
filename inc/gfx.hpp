#ifndef _H_GFX
#define _H_GFX

#include <SDL/SDL.h>

enum ExceptionCode
{
	EC_SDLEXIST,
	EC_SDLINIT,
	EC_SDLVIDEO,
	EC_QUIT
};

class SDLGuard
{
public:
	SDLGuard();
	~SDLGuard();
};

constexpr int SCREEN_WIDTH = 320;
constexpr int SCREEN_HEIGHT = 240;
#if defined(_BITTBOY)
constexpr int SCREEN_BPP = 16;
constexpr int FPS = 40;
#elif defined(__EMSCRIPTEN__)
constexpr int SCREEN_BPP = 32;
constexpr int FPS = 60;
#else
constexpr int SCREEN_BPP = 32;
constexpr int FPS = 60;
#endif

extern unsigned char *psp_font;
extern int            psp_font_width;
extern int            psp_font_height;

extern SDL_Surface *screen;
extern Uint32 foregroundColor;
extern Uint32 backgroundColor;
extern Uint32 playerColor;
extern Uint32 playerNegativeColor;

bool frameLimiter();
void psp_change_font(int id);
void psp_sdl_print(int x, int y, const char *str, Uint32 color);
unsigned char psp_convert_utf8_to_iso_8859_1(unsigned char c1, unsigned char c2);

#endif
