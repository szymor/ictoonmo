#include <iostream>
#include <list>
#include <cstdlib>
#include <ctime>
#include <cmath>

#include <SDL/SDL.h>

using std::cout;
using std::cerr;
using std::endl;
using std::list;
using std::srand;
using std::rand;
using std::time;
using std::fabs;

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
public:
	static constexpr int SCREEN_WIDTH = 320;
	static constexpr int SCREEN_HEIGHT = 240;
	static constexpr int SCREEN_BPP = 32;
	static constexpr int FPS = 60;
	SDLWrapper();
	~SDLWrapper();
	static SDL_Surface *getScreen();
	bool frameLimiter() const;
};

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
	void draw();
};

class Player
{
public:
	static constexpr double DEFAULT_ACCELERATION_X = 3000;
	static constexpr double DEFAULT_ACCELERATION_Y = 1000;
	static constexpr double FRICTION = 10;
	static constexpr double JUMP_POWER = 350;
	static constexpr double JUMP_COEFFICIENT = 0.4;
	CollisionBox cb;
	double vx;
	double vy;
	double ax;
	double ay;
	bool standing;
	bool wannaJump;
	void draw();
	void jump();
};

class Wall
{
public:
	static constexpr int DEFAULT_WIDTH = 16;
	CollisionBox cb;
	void draw();
};

class GameWorld
{
protected:
	Player player;
	list<Wall> walls;
	list<Platform> platforms;
	void addPlatform(int x, int y, int w);
	void addWall(int x);
public:
	static constexpr double PLATFORM_DISTANCE = 48;
	GameWorld();
	~GameWorld();
	void draw();
	void handleEvents();
	void process(Uint32 ms);
};

int main(int argc, char *argv[])
{
	try
	{
		SDLWrapper sdl;
		GameWorld gw;
		
		Uint32 lastTicks = SDL_GetTicks();
		while (true)
		{
			if (!sdl.frameLimiter())
				gw.draw();
			gw.handleEvents();
			Uint32 oldTicks = lastTicks;
			lastTicks = SDL_GetTicks();
			gw.process(lastTicks - oldTicks);
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
				cerr << "Application quitting gracefully..." << endl;
				break;
			default:
				cerr << "Unknown error occured." << endl;
		}
	}
	return 0;
}

SDL_Surface *SDLWrapper::screen = nullptr;

SDLWrapper::SDLWrapper()
{
	if (screen != nullptr)
		throw EC_SDLEXIST;
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) < 0)
		throw EC_SDLINIT;
	screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP, SDL_HWSURFACE | SDL_DOUBLEBUF);
	if (screen == nullptr)
		throw EC_SDLVIDEO;
	SDL_WM_SetCaption("ictoonmo", NULL);
	SDL_ShowCursor(SDL_DISABLE);
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

	SDL_Delay(1);

	return true;
}

void CollisionBox::draw()
{
	SDL_Surface *screen = SDLWrapper::getScreen();
	SDL_Rect r = {.x = (Sint16)x, .y = (Sint16)y, .w = (Uint16)w, .h = (Uint16)h};
	SDL_FillRect(screen, &r, SDL_MapRGB(screen->format, 0, 0, 0));
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
	cb.draw();
}

void Player::jump()
{
	standing = false;
	vy = -JUMP_POWER - fabs(vx * JUMP_COEFFICIENT);
}

void Wall::draw()
{
	cb.draw();
}

void GameWorld::addPlatform(int x, int y, int w)
{
	Platform platform;
	platform.cb.x = x;
	platform.cb.y = y;
	platform.cb.w = w;
	platform.cb.h = Platform::DEFAULT_HEIGHT;
	platforms.push_back(platform);
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

GameWorld::GameWorld()
{
	srand(time(nullptr));
	
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
	
	addPlatform(0, SDLWrapper::SCREEN_HEIGHT - Platform::DEFAULT_HEIGHT, SDLWrapper::SCREEN_WIDTH);
	for (int i = 1; i * PLATFORM_DISTANCE < SDLWrapper::SCREEN_HEIGHT; ++i)
	{
		int y = i * PLATFORM_DISTANCE - Platform::DEFAULT_HEIGHT;
		int w = rand() % (SDLWrapper::SCREEN_WIDTH / 6) + SDLWrapper::SCREEN_WIDTH / 6;
		int x = rand() % (SDLWrapper::SCREEN_WIDTH - w);
		addPlatform(x, y, w);
	}
	
	addWall(0);
	addWall(SDLWrapper::SCREEN_WIDTH - Wall::DEFAULT_WIDTH);
}

GameWorld::~GameWorld()
{
}

void GameWorld::process(Uint32 ms)
{
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
	for (auto &p: platforms)
	{
		if (player.cb.collides(p.cb))
		{
			if (player.vy > 0 && (player.cb.y + player.cb.h - p.cb.y) < 1.5 )
			{
				player.standing = true;
				player.vy = 0;
				player.cb.y = oldY;
			}
			break;
		}
	}
	
	if (player.standing && player.wannaJump)
	{
		player.jump();
	}
}

void GameWorld::draw()
{
	SDL_Surface *screen = SDLWrapper::getScreen();
	SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 255, 255, 255));
	
	for (auto &p: platforms)
		p.draw();
	player.draw();
	for (auto &w: walls)
		w.draw();
	
	SDL_Flip(screen);
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
						if (!key_right_pressed)
							player.ax = 0;
						break;
					case SDLK_RIGHT:
						key_right_pressed = false;
						if (!key_left_pressed)
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
