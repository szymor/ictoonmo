#include <iostream>
#include <fstream>
#include <list>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <string>

#include <SDL/SDL.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

using std::cout;
using std::cerr;
using std::endl;
using std::list;
using std::srand;
using std::rand;
using std::time;
using std::fabs;
using std::sqrt;
using std::string;
using std::ifstream;
using std::ofstream;

enum ExceptionCode
{
	EC_SDLEXIST,
	EC_SDLINIT,
	EC_SDLVIDEO,
	EC_QUIT
};

class SDLWrapper;
class GameWorld;
class CollisionBox;
class Platform;
class Player;
class Wall;

class SDLWrapper
{
protected:
	static SDL_Surface *screen;
	static SDL_Surface *realScreen;
public:
	static constexpr int SCREEN_WIDTH = 320;
	static constexpr int SCREEN_HEIGHT = 240;
#if defined(_BITTBOY)
	static constexpr int SCREEN_BPP = 16;
	static constexpr int FPS = 40;
#elif defined(__EMSCRIPTEN__)
	static constexpr int SCREEN_BPP = 32;
	static constexpr int FPS = 60;
#else
	static constexpr int SCREEN_BPP = 16;
	static constexpr int FPS = 60;
#endif
	static Uint32 foregroundColor;
	static Uint32 backgroundColor;
	static Uint32 playerColor;
	static Uint32 playerNegativeColor;
	SDLWrapper(bool bufferedScreen = false);
	~SDLWrapper();
	static SDL_Surface *getScreen();
	bool frameLimiter() const;
	static void flip();
	static void switchColors();
};

Uint32 SDLWrapper::foregroundColor = 0;
Uint32 SDLWrapper::backgroundColor = 0;
Uint32 SDLWrapper::playerColor = 0;
Uint32 SDLWrapper::playerNegativeColor = 0;

class CollisionBox
{
public:
	double x;
	double y;
	double w;
	double h;
	void draw();
	bool collides(const CollisionBox &cb);
};

class Platform
{
public:
	static constexpr int DEFAULT_HEIGHT = 16;
	CollisionBox cb;
	int no;
	void draw();
};

class Player
{
public:
	static constexpr double DEFAULT_ACCELERATION_X = 2000;
	static constexpr double DEFAULT_ACCELERATION_Y = 1000;
	static constexpr double FRICTION = 5;
	static constexpr double JUMP_POWER = 300;
	static constexpr double JUMP_COEFFICIENT = 0.002;
	CollisionBox cb;
	double vx;
	double vy;
	double ax;
	double ay;
	bool standing;
	bool wannaJump;
	int floorNo;
	list<Platform>::iterator lastCollidedPlatform;
	void draw();
	void jump();
};

class Wall
{
public:
	static constexpr int DEFAULT_WIDTH = 4;
	CollisionBox cb;
	void draw();
};

class GameWorld
{
protected:
	int hiscore;
	int lastSavedHiscore;
	Player player;
	list<Wall> walls;
	list<Platform> platforms;
	void addPlatform(int x, int y, int w, int no);
	void addWall(int x);
	void saveHiscore();
	void loadHiscore();
public:
	static constexpr double PLATFORM_DISTANCE = 40;
	static constexpr double PACE_COEFFICIENT = 0.005;
	static constexpr Uint32 RESET_TIMEOUT = 2000;
	static constexpr char GAMEDIR[] = ".ictoonmo";
	static constexpr char HISCORE_FILE[] = "hiscore.dat";
	GameWorld();
	~GameWorld();
	void draw();
	void handleEvents();
	void process(Uint32 ms);
	bool gameFinished();
	void reset();
	void printScore();
};

constexpr char GameWorld::GAMEDIR[];
constexpr char GameWorld::HISCORE_FILE[];

// font api
extern SDL_Surface *screen_surface;
void psp_sdl_print(int x, int y, const char *str, int color);

int main(int argc, char *argv[])
{
	try
	{
#ifdef _BITTBOY
		SDLWrapper sdl(true);
#else
		SDLWrapper sdl;
#endif
		GameWorld gw;
		// workaround for font module dependency
		screen_surface = sdl.getScreen();

		Uint32 resetTimer = 0;
		Uint32 lastTicks = SDL_GetTicks();
		while (true)
		{
			if (!sdl.frameLimiter())
			{
				gw.draw();
			}
			gw.handleEvents();
			Uint32 oldTicks = lastTicks;
			lastTicks = SDL_GetTicks();
			gw.process(lastTicks - oldTicks);
			if (gw.gameFinished())
			{
				resetTimer += lastTicks - oldTicks;
				if (resetTimer > GameWorld::RESET_TIMEOUT)
				{
					resetTimer = 0;
					gw.printScore();
					gw.reset();
				}
			}
		}
	}
	catch (ExceptionCode ec)
	{
		switch (ec)
		{
			case EC_SDLEXIST:
				cerr << "Double SDL initialization." << endl;
				break;
			case EC_SDLINIT:
				cerr << "SDL initialization failed." << endl;
				break;
			case EC_SDLVIDEO:
				cerr << "SDL video mode setting failed." << endl;
				break;
			case EC_QUIT:
				// cerr << "Application quitting gracefully..." << endl;
				break;
			default:
				cerr << "Unknown error occured." << endl;
		}
	}
	return 0;
}

SDL_Surface *SDLWrapper::screen = nullptr;
SDL_Surface *SDLWrapper::realScreen = nullptr;

SDLWrapper::SDLWrapper(bool bufferedScreen)
{
	if (screen != nullptr)
		throw EC_SDLEXIST;
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) < 0)
		throw EC_SDLINIT;
	if (bufferedScreen)
	{
		realScreen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP, SDL_HWSURFACE | SDL_DOUBLEBUF);
		if (realScreen == nullptr)
			throw EC_SDLVIDEO;
		screen = SDL_CreateRGBSurface(SDL_HWSURFACE, realScreen->w, realScreen->h,
			realScreen->format->BitsPerPixel, realScreen->format->Rmask, realScreen->format->Gmask,
			realScreen->format->Bmask, realScreen->format->Amask);
		if (screen == nullptr)
			throw EC_SDLVIDEO;
	}
	else
	{
		realScreen = nullptr;
		screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP, SDL_HWSURFACE | SDL_DOUBLEBUF);
		if (screen == nullptr)
			throw EC_SDLVIDEO;
	}
	SDL_WM_SetCaption("ictoonmo", NULL);
	SDL_ShowCursor(SDL_DISABLE);

	foregroundColor = SDL_MapRGB(screen->format, 0, 0, 0);
	backgroundColor = SDL_MapRGB(screen->format, 255, 255, 255);
	playerColor = SDL_MapRGB(screen->format, 0, 0, 255);
	playerNegativeColor = SDL_MapRGB(screen->format, 255, 255, 0);
}

SDLWrapper::~SDLWrapper()
{
	SDL_Quit();
	screen = nullptr;
}

SDL_Surface *SDLWrapper::getScreen()
{
	return screen;
}

bool SDLWrapper::frameLimiter() const
{
	static Uint32 curTicks;
	static Uint32 lastTicks;
	float t;

#if NO_FRAMELIMIT
	return false;
#endif

	curTicks = SDL_GetTicks();
	t = curTicks - lastTicks;

	if (t >= 1000.0/FPS)
	{
		lastTicks = curTicks;
		return false;
	}

#ifdef __EMSCRIPTEN__
	emscripten_sleep(1);
#else
	SDL_Delay(1);
#endif

	return true;
}

void SDLWrapper::flip()
{
	if (realScreen == nullptr)
	{
		SDL_Flip(screen);
	}
	else
	{
		SDL_BlitSurface(screen, NULL, realScreen, NULL);
		SDL_Flip(realScreen);
	}
}

void SDLWrapper::switchColors()
{
	Uint32 temp = foregroundColor;
	foregroundColor = backgroundColor;
	backgroundColor = temp;
	temp = playerColor;
	playerColor = playerNegativeColor;
	playerNegativeColor = temp;
}

void CollisionBox::draw()
{
	SDL_Surface *screen = SDLWrapper::getScreen();
	SDL_Rect r = {.x = (Sint16)x, .y = (Sint16)y, .w = (Uint16)w, .h = (Uint16)h};
	SDL_FillRect(screen, &r, SDLWrapper::foregroundColor);
}

bool CollisionBox::collides(const CollisionBox &cb)
{
	return !((this->x + this->w) < cb.x ||
		this->x > (cb.x + cb.w) ||
		(this->y + this->h) < cb.y ||
		this->y > (cb.y + cb.h));
}

void Platform::draw()
{
	cb.draw();
}

void Player::draw()
{
	SDL_Surface *screen = SDLWrapper::getScreen();
	SDL_Rect r = {.x = (Sint16)cb.x, .y = (Sint16)cb.y, .w = (Uint16)(cb.w + 1), .h = (Uint16)(cb.h + 1)};
	SDL_FillRect(screen, &r, SDLWrapper::playerColor);
}

void Player::jump()
{
	standing = false;
	vy = -JUMP_POWER - fabs(vx * vx * JUMP_COEFFICIENT);
}

void Wall::draw()
{
	cb.draw();
}

void GameWorld::addPlatform(int x, int y, int w, int no)
{
	Platform platform;
	platform.cb.x = x;
	platform.cb.y = y;
	platform.cb.w = w;
	platform.cb.h = Platform::DEFAULT_HEIGHT;
	platform.no = no;
	platforms.push_front(platform);
}

void GameWorld::addWall(int x)
{
	Wall wall;
	wall.cb.x = x;
	wall.cb.y = -SDLWrapper::SCREEN_HEIGHT;
	wall.cb.w = Wall::DEFAULT_WIDTH;
	wall.cb.h = SDLWrapper::SCREEN_HEIGHT * 2;
	walls.push_back(wall);
}

void GameWorld::saveHiscore()
{
	if (hiscore > lastSavedHiscore)
	{
		const char *home = getenv("HOME");
		string path = string(home) + "/" + GAMEDIR;
		mkdir(path.c_str(), 0744);
		path += string("/") + HISCORE_FILE;
		ofstream ofs(path);
		ofs << hiscore;
		lastSavedHiscore = hiscore;
	}
}

void GameWorld::loadHiscore()
{
	const char *home = getenv("HOME");
	string path = string(home) + "/" + GAMEDIR + "/" + HISCORE_FILE;
	ifstream ifs(path);
	if (ifs.good())
	{
		ifs >> hiscore;
		lastSavedHiscore = hiscore;
	}
	else
	{
		hiscore = 0;
		lastSavedHiscore = 0;
	}
}

GameWorld::GameWorld()
{
	srand(time(nullptr));
	loadHiscore();

	addWall(0);
	addWall(SDLWrapper::SCREEN_WIDTH - Wall::DEFAULT_WIDTH);

	reset();
}

GameWorld::~GameWorld()
{
	saveHiscore();
	printScore();
}

void GameWorld::process(Uint32 ms)
{
	if (gameFinished())
		return;

	double oldX = player.cb.x;
	double oldY = player.cb.y;
	player.vx += (player.ax - Player::FRICTION * player.vx) * ms / 1000.0;
	player.vy += player.ay * ms / 1000.0;

	player.cb.x += player.vx * ms / 1000.0;
	for (auto &w: walls)
	{
		if (player.cb.collides(w.cb))
		{
			player.vx = -player.vx;
			player.cb.x = oldX;
			break;
		}
	}

	player.cb.y += player.vy * ms / 1000.0;

	if (player.vy <= 0)
	{
		player.lastCollidedPlatform = platforms.end();
	}

	for (auto it = platforms.begin(); it != platforms.end(); ++it)
	{
		auto &p = *it;
		if (player.cb.collides(p.cb))
		{
			// going up
			if (player.vy < 0)
			{
				player.lastCollidedPlatform = it;
			}

			// going down
			if (player.vy > 0 && player.lastCollidedPlatform != it)
			{
				// if collision is not from side, then proceed
				double cl = player.cb.x > p.cb.x ? player.cb.x : p.cb.x;
				double cr = (player.cb.x + player.cb.w) < (p.cb.x + p.cb.w) ?
							(player.cb.x + player.cb.w) : (p.cb.x + p.cb.w);
				double cw = cr - cl;
				double cu = player.cb.y > p.cb.y ? player.cb.y : p.cb.y;
				double cd = (player.cb.y + player.cb.h) < (p.cb.y + p.cb.h) ?
							(player.cb.y + player.cb.h) : (p.cb.y + p.cb.h);
				double ch = cd - cu;
				if (cw > ch && (player.cb.y + player.cb.h) < (p.cb.y + p.cb.h))
				{
					player.standing = true;
					player.vy = 0;
					player.cb.y = oldY;
					if (p.no > player.floorNo)
					{
						player.floorNo = p.no;
						if (player.floorNo > hiscore)
							hiscore = player.floorNo;
					}
				}
				else
				{
					player.lastCollidedPlatform = it;
				}
			}
			break;
		}
	}
	if (player.vy > 0)
	{
		player.standing = false;
	}

	if (player.standing && player.wannaJump)
	{
		player.jump();
	}

	// pacemaker
	double pace = sqrt((double)platforms.rbegin()->no) * GameWorld::PACE_COEFFICIENT * ms;
	player.cb.y += pace;
	for (auto &p: platforms)
	{
		p.cb.y += pace;
	}

	// perspective adjustment
	int yDiff = SDLWrapper::SCREEN_HEIGHT / 6 - player.cb.y;
	if (yDiff > 0)
	{
		player.cb.y += yDiff;
		for (auto &p: platforms)
		{
			p.cb.y += yDiff;
		}
	}

	// platform generation
	if (platforms.begin()->cb.y > (GameWorld::PLATFORM_DISTANCE - Platform::DEFAULT_HEIGHT))
	{
		int y = platforms.begin()->cb.y - PLATFORM_DISTANCE;
		int no = platforms.begin()->no + 1;
		int w = 0;
		int x = 0;
		if (no % 100 == 0)
		{
			w = SDLWrapper::SCREEN_WIDTH;
			x = 0;
		}
		else
		{
			w = rand() % (SDLWrapper::SCREEN_WIDTH / 6) + SDLWrapper::SCREEN_WIDTH / 6;
			x = rand() % (SDLWrapper::SCREEN_WIDTH - w - 2 * Wall::DEFAULT_WIDTH) + Wall::DEFAULT_WIDTH;
		}
		addPlatform(x, y, w, no);
	}

	// platform destruction
	if (platforms.rbegin()->cb.y > SDLWrapper::SCREEN_HEIGHT)
	{
		platforms.pop_back();
	}
}

bool GameWorld::gameFinished()
{
	return player.cb.y > SDLWrapper::SCREEN_HEIGHT;
}

void GameWorld::reset()
{
	saveHiscore();

	player.cb.x = SDLWrapper::SCREEN_WIDTH / 2;
	player.cb.y = SDLWrapper::SCREEN_HEIGHT - 40;
	player.cb.w = 16;
	player.cb.h = 16;
	player.vx = 0;
	player.vy = 0;
	player.ax = 0;
	player.ay = Player::DEFAULT_ACCELERATION_Y;
	player.standing = false;
	player.wannaJump = false;
	player.floorNo = 0;

	platforms.clear();
	player.lastCollidedPlatform = platforms.end();
	addPlatform(0, SDLWrapper::SCREEN_HEIGHT - Platform::DEFAULT_HEIGHT, SDLWrapper::SCREEN_WIDTH, 0);
	for (int i = 1; i * PLATFORM_DISTANCE < SDLWrapper::SCREEN_HEIGHT; ++i)
	{
		int y = SDLWrapper::SCREEN_HEIGHT - Platform::DEFAULT_HEIGHT - i * PLATFORM_DISTANCE;
		int w = rand() % (SDLWrapper::SCREEN_WIDTH / 6) + SDLWrapper::SCREEN_WIDTH / 6;
		int x = rand() % (SDLWrapper::SCREEN_WIDTH - w);
		addPlatform(x, y, w, i);
	}
}

void GameWorld::printScore()
{
	string postfix;
	switch (player.floorNo % 100)
	{
		case 11:
		case 12:
		case 13:
			postfix = "th";
			break;
		default:
			switch (player.floorNo % 10)
			{
				case 1:
					postfix = "st";
					break;
				case 2:
					postfix = "nd";
					break;
				case 3:
					postfix = "rd";
					break;
				default:
					postfix = "th";
					break;
			}
			break;
	}
	cout << "You have reached the " << player.floorNo << postfix << " floor." << endl;
}

void GameWorld::draw()
{
	SDL_Surface *screen = SDLWrapper::getScreen();

	SDL_FillRect(screen, NULL, SDLWrapper::backgroundColor);

	for (auto &p: platforms)
		p.draw();
	player.draw();
	for (auto &w: walls)
		w.draw();

#ifndef __EMSCRIPTEN__
	string status = std::to_string(player.floorNo) + "/" + std::to_string(hiscore);
	int xpos = SDLWrapper::SCREEN_WIDTH - (status.length() + 1) * 8;
	int ypos = 4;
	psp_sdl_print(xpos, ypos, status.c_str(), SDLWrapper::foregroundColor);
#endif
	SDLWrapper::flip();
}

void GameWorld::handleEvents()
{
	static bool key_left_pressed = false;
	static bool key_right_pressed = false;
	SDL_Event event;

	if (SDL_PollEvent(&event))
		switch (event.type)
		{
			case SDL_KEYUP:
				switch (event.key.keysym.sym)
				{
					case SDLK_LEFT:
						key_left_pressed = false;
						if (key_right_pressed)
							player.ax = Player::DEFAULT_ACCELERATION_X;
						else
							player.ax = 0;
						break;
					case SDLK_RIGHT:
						key_right_pressed = false;
						if (key_left_pressed)
							player.ax = -Player::DEFAULT_ACCELERATION_X;
						else
							player.ax = 0;
						break;
					case SDLK_SPACE:
						player.wannaJump = false;
						break;
				}
				break;
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym)
				{
					case SDLK_RETURN:
						SDLWrapper::switchColors();
						break;
					case SDLK_SPACE:
						player.wannaJump = true;
						if (player.standing)
						{
							player.jump();
						}
						break;
					case SDLK_LEFT:
						key_left_pressed = true;
						player.ax = -Player::DEFAULT_ACCELERATION_X;
						break;
					case SDLK_RIGHT:
						key_right_pressed = true;
						player.ax = Player::DEFAULT_ACCELERATION_X;
						break;
					case SDLK_ESCAPE:
					{
						SDL_Event ev;
						ev.type = SDL_QUIT;
						SDL_PushEvent(&ev);
						break;
					}
				}
				break;
			case SDL_QUIT:
				throw EC_QUIT;
				break;
		}
}
