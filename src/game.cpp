#include "game.hpp"
#include "gfx.hpp"

#include <SDL/SDL.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <string>
#include <cmath>
#include <fstream>
#include <iostream>

using std::string;
using std::fabs;
using std::sqrt;
using std::ifstream;
using std::ofstream;
using std::endl;
using std::cout;
using std::cerr;

constexpr char GameWorld::GAMEDIR[];
constexpr char GameWorld::HISCORE_FILE[];

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

GameWorld::GameWorld() : mt(rd())
{
	loadHiscore();
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

	double msd = ms / 1000.0;
	double oldX = player.cb.x;
	double oldY = player.cb.y;

	player.cb.x += player.vx * msd;
	if (player.cb.x < WALL_WIDTH)
	{
		player.cb.x = WALL_WIDTH;
		player.vx = -BOUNCINESS * player.vx;
	}
	if (player.cb.x + player.cb.w > SCREEN_WIDTH - WALL_WIDTH)
	{
		player.cb.x = SCREEN_WIDTH - player.cb.w - WALL_WIDTH;
		player.vx = -BOUNCINESS * player.vx;
	}
	player.cb.y += player.vy * msd;
	player.vx += (player.ax - Player::FRICTION * player.vx) * msd;
	player.vy += player.ay * msd;

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
				if ((cw > ch && (player.cb.y + player.cb.h) < (p.cb.y + p.cb.h)) ||
					((oldY + player.cb.h) <= p.cb.y))
				{
					player.standing = true;
					player.vy = 0;
					player.cb.y = p.cb.y - player.cb.h;
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
	int yDiff = SCREEN_HEIGHT / 6 - player.cb.y;
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
			w = SCREEN_WIDTH;
			x = 0;
		}
		else
		{
			std::uniform_int_distribution<int> udw(SCREEN_WIDTH / 6, 2 * SCREEN_WIDTH / 6);
			w = udw(mt);
			std::uniform_int_distribution<int> udx(WALL_WIDTH, SCREEN_WIDTH - w - WALL_WIDTH);
			x = udx(mt);
		}
		addPlatform(x, y, w, no);
	}

	// platform destruction
	if (platforms.rbegin()->cb.y > SCREEN_HEIGHT)
	{
		platforms.pop_back();
	}
}

bool GameWorld::gameFinished()
{
	return player.cb.y > SCREEN_HEIGHT;
}

void GameWorld::reset()
{
	saveHiscore();

	player.cb.x = SCREEN_WIDTH / 2;
	player.cb.y = SCREEN_HEIGHT - 40;
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
	addPlatform(0, SCREEN_HEIGHT - Platform::DEFAULT_HEIGHT, SCREEN_WIDTH, 0);
	for (int i = 1; i * PLATFORM_DISTANCE < SCREEN_HEIGHT; ++i)
	{
		int y = SCREEN_HEIGHT - Platform::DEFAULT_HEIGHT - i * PLATFORM_DISTANCE;
		std::uniform_int_distribution<int> udw(SCREEN_WIDTH / 6, 2 * SCREEN_WIDTH / 6);
		int w = udw(mt);
		std::uniform_int_distribution<int> udx(WALL_WIDTH, SCREEN_WIDTH - w - WALL_WIDTH);
		int x = udx(mt);
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
	SDL_FillRect(screen, NULL, backgroundColor);
	SDL_Rect r = {.x = 0, .y = 0, .w = WALL_WIDTH, .h = SCREEN_HEIGHT};
	SDL_FillRect(screen, &r, foregroundColor);
	r.x = SCREEN_WIDTH - WALL_WIDTH;
	SDL_FillRect(screen, &r, foregroundColor);

	for (auto &p: platforms)
		p.draw();
	player.draw();

	string status = std::to_string(player.floorNo) + "/" + std::to_string(hiscore);
	int xpos = SCREEN_WIDTH - (status.length() + 1) * 8;
	int ypos = 4;
	psp_sdl_print(xpos, ypos, status.c_str(), foregroundColor);
	SDL_Flip(screen);
}

void GameWorld::switchColors()
{
	Uint32 temp = foregroundColor;
	foregroundColor = backgroundColor;
	backgroundColor = temp;
	temp = playerColor;
	playerColor = playerNegativeColor;
	playerNegativeColor = temp;
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
						switchColors();
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

void CollisionBox::draw()
{
	SDL_Rect r = {.x = (Sint16)x, .y = (Sint16)y, .w = (Uint16)w, .h = (Uint16)h};
	SDL_FillRect(screen, &r, foregroundColor);
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
	SDL_Rect r = {.x = (Sint16)cb.x, .y = (Sint16)cb.y, .w = (Uint16)(cb.w), .h = (Uint16)(cb.h)};
	SDL_FillRect(screen, &r, playerColor);
}

void Player::jump()
{
	standing = false;
	vy = -JUMP_POWER - fabs(vx * vx * JUMP_COEFFICIENT);
}
