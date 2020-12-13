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
using std::unique_ptr;
using std::make_unique;

constexpr char GameWorld::GAMEDIR[];
constexpr char GameWorld::HISCORE_FILE[];

static std::random_device rd;
static std::mt19937 mt(rd());

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

	if (player.vy < 0)
	{
		player.lastCollidedPlatform = platforms.end();
	}

	for (auto it = platforms.begin(); it != platforms.end(); ++it)
	{
		auto &p = *it;
		if (player.cb.collides(p->cb))
		{
			// going up or standing
			if (player.vy < 0 ||
				(player.standingPlatform &&
				player.standingPlatform != p.get()))
			{
				player.lastCollidedPlatform = it;
			}

			// going down
			if (player.vy > 0 && player.lastCollidedPlatform != it)
			{
				// if collision is not from side, then proceed
				double cl = player.cb.x > p->cb.x ? player.cb.x : p->cb.x;
				double cr = (player.cb.x + player.cb.w) < (p->cb.x + p->cb.w) ?
							(player.cb.x + player.cb.w) : (p->cb.x + p->cb.w);
				double cw = cr - cl;
				double cu = player.cb.y > p->cb.y ? player.cb.y : p->cb.y;
				double cd = (player.cb.y + player.cb.h) < (p->cb.y + p->cb.h) ?
							(player.cb.y + player.cb.h) : (p->cb.y + p->cb.h);
				double ch = cd - cu;
				if ((cw > ch && (player.cb.y + player.cb.h) < (p->cb.y + p->cb.h)) ||
					((oldY + player.cb.h) <= p->cb.y))
				{
					player.standingPlatform = p.get();
					DisappearingPlatform *dp = dynamic_cast<DisappearingPlatform*>(p.get());
					if (dp)
						dp->running = true;
					player.vy = 0;
					player.cb.y = p->cb.y - player.cb.h;
					if (p->no > player.floorNo)
					{
						player.floorNo = p->no;
						if (player.floorNo > hiscore)
							hiscore = player.floorNo;
					}
					SpringPlatform *sp = dynamic_cast<SpringPlatform*>(p.get());
					if (sp)
					{
						player.standingPlatform = nullptr;
						player.vy = -Player::JUMP_POWER * 2.0;
					}
				}
				else
				{
					player.lastCollidedPlatform = it;
				}
			}
		}
		else
		{
			if (player.lastCollidedPlatform == it)
				player.lastCollidedPlatform = platforms.end();
		}
	}
	if (player.vy > 0)
	{
		player.standingPlatform = nullptr;
	}

	if (player.standingPlatform && player.wannaJump)
	{
		player.jump();
	}

	// pacemaker
	double pace = sqrt((double)((*platforms.rbegin())->no)) * GameWorld::PACE_COEFFICIENT * ms;
	travelledDistance += pace;
	player.cb.y += pace;
	for (auto &p: platforms)
	{
		p->cb.y += pace;
	}

	// perspective adjustment
	int yDiff = SCREEN_HEIGHT / 6 - player.cb.y;
	if (yDiff > 0)
	{
		player.cb.y += yDiff;
		travelledDistance += yDiff;
		for (auto &p: platforms)
		{
			p->cb.y += yDiff;
		}
	}

	// platform generation
	if ((*platforms.begin())->cb.y > (GameWorld::PLATFORM_DISTANCE - IPlatform::DEFAULT_HEIGHT))
	{
		int y = (*platforms.begin())->cb.y - PLATFORM_DISTANCE;
		int no = (*platforms.begin())->no + 1;
		unique_ptr<IPlatform> platform;
		if (no % 100 == 0)
		{
			platform = make_unique<BasicPlatform>(this, no, y);
			platform->cb.w = SCREEN_WIDTH;
			platform->cb.x = 0;
			BasicPlatform *raw = static_cast<BasicPlatform*>(platform.get());
			if (100 == no)
				raw->label = "desert";
			else if (200 == no)
				raw->label = "volcano";
			else if (300 == no)
				raw->label = "sky";
		}
		else
		{
			// meadow
			if (no < 30)
			{
				platform = make_unique<FriendlyPlatform>(this, no, y);
			}
			else if (no < 100)
			{
				std::uniform_int_distribution<int> roll(1, 100);
				int chance = roll(mt);
				if (chance <= 50)
				{
					platform = make_unique<FriendlyPlatform>(this, no, y);
				}
				else
				{
					platform = make_unique<BasicPlatform>(this, no, y);
				}
			}
			// desert
			else if (no < 200)
			{
				std::uniform_int_distribution<int> roll(1, 100);
				int chance = roll(mt);
				if (chance <= 50)
				{
					platform = make_unique<EvasivePlatform>(this, no, y);
				}
				else
				{
					platform = make_unique<BasicPlatform>(this, no, y);
				}
			}
			// volcano
			else if (no < 300)
			{
				std::uniform_int_distribution<int> roll(1, 100);
				int chance = roll(mt);
				if (chance <= 50)
				{
					platform = make_unique<DisappearingPlatform>(this, no, y);
				}
				else
				{
					platform = make_unique<BasicPlatform>(this, no, y);
				}
			}
			// sky
			else if (no < 400)
			{
				std::uniform_int_distribution<int> roll(1, 100);
				int chance = roll(mt);
				if (chance <= 30)
				{
					platform = make_unique<MovingPlatform>(this, no, y);
				}
				else if (chance <= 40)
				{
					platform = make_unique<EvasivePlatform>(this, no, y);
				}
				else if (chance <= 50)
				{
					platform = make_unique<FriendlyPlatform>(this, no, y);
				}
				else if (chance <= 70)
				{
					platform = make_unique<DisappearingPlatform>(this, no, y);
				}
				else
				{
					platform = make_unique<BasicPlatform>(this, no, y);
				}
			}
			else
			{
				std::uniform_int_distribution<int> roll(1, 100);
				int chance = roll(mt);
				if (chance <= 50)
				{
					platform = make_unique<MovingPlatform>(this, no, y);
				}
				else
				{
					platform = make_unique<BasicPlatform>(this, no, y);
				}
			}
		}
		platforms.push_front(std::move(platform));
	}

	// active platform processing
	for (auto &p: platforms)
		p->process(ms);

	// platform destruction
	if ((*platforms.rbegin())->cb.y > SCREEN_HEIGHT)
	{
		platforms.pop_back();
	}
	for (auto i = platforms.begin(); i != platforms.end(); ++i)
	{
		if ((*i)->deleteFlag)
		{
			if (player.standingPlatform == (*i).get())
				player.standingPlatform = nullptr;
			if (player.lastCollidedPlatform == i)
				player.lastCollidedPlatform = platforms.end();
			i = platforms.erase(i);
		}
	}
}

bool GameWorld::gameFinished()
{
	return player.cb.y > SCREEN_HEIGHT;
}

void GameWorld::reset()
{
	travelledDistance = 0.0;
	saveHiscore();

	player.reset();
	platforms.clear();
	player.lastCollidedPlatform = platforms.end();

	auto base = make_unique<BasicPlatform>(this, 0, SCREEN_HEIGHT - IPlatform::DEFAULT_HEIGHT);
	base->cb.x = 0;
	base->cb.w = SCREEN_WIDTH;
	base->label = "meadow";
	platforms.push_front(std::move(base));
	for (int i = 1; i * PLATFORM_DISTANCE < SCREEN_HEIGHT; ++i)
	{
		if (1 == i && hiscore >= 500)
		{
			auto platform = make_unique<ElevatorPlatform>(this, i, SCREEN_HEIGHT - IPlatform::DEFAULT_HEIGHT - i * PLATFORM_DISTANCE);
			platforms.push_front(std::move(platform));
			continue;
		}
		auto platform = make_unique<FriendlyPlatform>(this, i, SCREEN_HEIGHT - IPlatform::DEFAULT_HEIGHT - i * PLATFORM_DISTANCE);
		platforms.push_front(std::move(platform));
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
	constexpr SDL_Color green = {.r = 144, .g = 255, .b = 144};
	constexpr SDL_Color yellow = {.r = 255, .g = 255, .b = 144};
	constexpr SDL_Color red = {.r = 255, .g = 144, .b = 144};
	constexpr SDL_Color blue = {.r = 144, .g = 144, .b = 255};
	constexpr SDL_Color gray = {.r = 224, .g = 224, .b = 224};
	SDL_Color fc = {.r = 0, .g = 0, .b = 0};
	Uint32 finalColor;
	if (travelledDistance < PLATFORM_DISTANCE * 100)
	{
		double ratio = travelledDistance / (PLATFORM_DISTANCE * 100);
		fc.r = (1 - ratio) * green.r + ratio * yellow.r;
		fc.g = (1 - ratio) * green.g + ratio * yellow.g;
		fc.b = (1 - ratio) * green.b + ratio * yellow.b;
	}
	else if (travelledDistance < PLATFORM_DISTANCE * 200)
	{
		double ratio = (travelledDistance - PLATFORM_DISTANCE * 100) / (PLATFORM_DISTANCE * 100);
		fc.r = (1 - ratio) * yellow.r + ratio * red.r;
		fc.g = (1 - ratio) * yellow.g + ratio * red.g;
		fc.b = (1 - ratio) * yellow.b + ratio * red.b;
	}
	else if (travelledDistance < PLATFORM_DISTANCE * 300)
	{
		double ratio = (travelledDistance - PLATFORM_DISTANCE * 200) / (PLATFORM_DISTANCE * 100);
		fc.r = (1 - ratio) * red.r + ratio * blue.r;
		fc.g = (1 - ratio) * red.g + ratio * blue.g;
		fc.b = (1 - ratio) * red.b + ratio * blue.b;
	}
	else if (travelledDistance < PLATFORM_DISTANCE * 400)
	{
		double ratio = (travelledDistance - PLATFORM_DISTANCE * 300) / (PLATFORM_DISTANCE * 100);
		fc.r = (1 - ratio) * blue.r + ratio * gray.r;
		fc.g = (1 - ratio) * blue.g + ratio * gray.g;
		fc.b = (1 - ratio) * blue.b + ratio * gray.b;
	}
	else
	{
		fc = gray;
	}
	if (darkMode)
	{
		fc.r = 255 - fc.r;
		fc.g = 255 - fc.g;
		fc.b = 255 - fc.b;
	}
	finalColor = SDL_MapRGB(screen->format, fc.r, fc.g, fc.b);
	SDL_FillRect(screen, NULL, finalColor);
	backgroundColor = finalColor;

	SDL_Rect r = {.x = 0, .y = 0, .w = WALL_WIDTH, .h = SCREEN_HEIGHT};
	SDL_FillRect(screen, &r, primaryColor);
	r.x = SCREEN_WIDTH - WALL_WIDTH;
	SDL_FillRect(screen, &r, primaryColor);

	for (auto &p: platforms)
		p->draw();
	player.draw();

	string status = std::to_string(player.floorNo) + "/" + std::to_string(hiscore);
	int xpos = SCREEN_WIDTH - (status.length() + 1) * 8;
	int ypos = 4;
	psp_sdl_print(xpos, ypos, status.c_str(), primaryColor);
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
						if (player.standingPlatform)
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
	SDL_FillRect(screen, &r, primaryColor);
}

bool CollisionBox::collides(const CollisionBox &cb)
{
	return !((this->x + this->w) < cb.x ||
		this->x > (cb.x + cb.w) ||
		(this->y + this->h) < cb.y ||
		this->y > (cb.y + cb.h));
}

BasicPlatform::BasicPlatform(GameWorld *gw, int no, double y)
	: gw{gw}
{
	this->no = no;
	this->cb.y = y;
	this->cb.h = IPlatform::DEFAULT_HEIGHT;
	std::uniform_int_distribution<int> udw(SCREEN_WIDTH / 6, 2 * SCREEN_WIDTH / 6);
	this->cb.w = udw(mt);
	std::uniform_int_distribution<int> udx(GameWorld::WALL_WIDTH + Player::SIZE / 2, SCREEN_WIDTH - this->cb.w - GameWorld::WALL_WIDTH - Player::SIZE / 2);
	this->cb.x = udx(mt);
}

void BasicPlatform::draw()
{
	cb.draw();
	if (label != "")
	{
		int posx = cb.x + GameWorld::WALL_WIDTH + 2;
		int posy = cb.y + 2;
		psp_change_font(4);
		if (posy > 0 && posy < (SCREEN_HEIGHT - psp_font_height))
		{
			psp_sdl_print(posx, posy, label.c_str(), secondaryColor);
		}
		psp_change_font(2);
	}
}

void BasicPlatform::process(Uint32 ms)
{
	(void)ms;
}

DisappearingPlatform::DisappearingPlatform(GameWorld *gw, int no, double y, double maxt)
	: BasicPlatform(gw, no, y), t{0.0}, maxt{maxt}, running{false}
{
	if (0 == maxt)
	{
		std::uniform_real_distribution<> udmt(0.3, 1.0);
		this->maxt = udmt(mt);
	}
}

void DisappearingPlatform::draw()
{
	Uint8 br, bg, bb, fr, fg, fb;
	SDL_GetRGB(backgroundColor, screen->format, &br, &bg, &bb);
	SDL_GetRGB(primaryColor, screen->format, &fr, &fg, &fb);
	double ratio = t / maxt;
	Uint8 r = ratio * br + (1 - ratio) * fr;
	Uint8 g = ratio * bg + (1 - ratio) * fg;
	Uint8 b = ratio * bb + (1 - ratio) * fb;
	Uint32 finalColor = SDL_MapRGB(screen->format, r, g, b);

	SDL_Rect rect = {.x = (Sint16)cb.x, .y = (Sint16)cb.y, .w = (Uint16)cb.w, .h = (Uint16)cb.h};
	SDL_FillRect(screen, &rect, finalColor);
}

void DisappearingPlatform::process(Uint32 ms)
{
	if (running)
	{
		t += ms / 1000.0;
		if (t > maxt)
		{
			deleteFlag = true;
		}
	}
}

FriendlyPlatform::FriendlyPlatform(GameWorld *gw, int no, double y)
	: BasicPlatform(gw, no, y)
{
}

void FriendlyPlatform::process(Uint32 ms)
{
	if (this == gw->player.standingPlatform)
	{
		if (gw->player.cb.x < cb.x - Player::SIZE / 2)
		{
			double dx = cb.x - gw->player.cb.x;
			cb.x -= 5.0 * dx * ms / 1000.0;
		}
		else if (gw->player.cb.x + gw->player.cb.w > cb.x + cb.w + Player::SIZE / 2)
		{
			double dx = (gw->player.cb.x + gw->player.cb.w) - (cb.x + cb.w);
			cb.x += 5.0 * dx * ms / 1000.0;
		}
	}
	if ((cb.y > SCREEN_HEIGHT - (GameWorld::PLATFORM_DISTANCE + IPlatform::DEFAULT_HEIGHT)) &&
		(cb.y > gw->player.cb.y) &&
		(gw->player.cb.y > SCREEN_HEIGHT / 2))
	{
		double center = cb.x + cb.w / 2;
		double pcenter = gw->player.cb.x + gw->player.cb.w / 2;
		if (gw->player.vy > 300.0)
		{
			cb.x += 10.0 * (pcenter - center) * ms / 1000.0;
		}
	}
}

EvasivePlatform::EvasivePlatform(GameWorld *gw, int no, double y)
	: BasicPlatform(gw, no, y)
{
}

void EvasivePlatform::process(Uint32 ms)
{
	if (this == gw->player.standingPlatform)
	{
		if ((gw->player.cb.x < cb.x - Player::SIZE / 4) &&
			(gw->player.vx <= 0))
		{
			double dx = cb.x - gw->player.cb.x;
			cb.x += 20.0 * dx * ms / 1000.0;
		}
		else if ((gw->player.cb.x + gw->player.cb.w > cb.x + cb.w + Player::SIZE / 4) &&
				(gw->player.vx >= 0))
		{
			double dx = (gw->player.cb.x + gw->player.cb.w) - (cb.x + cb.w);
			cb.x -= 20.0 * dx * ms / 1000.0;
		}
	}
}

RestlessPlatform::RestlessPlatform(GameWorld *gw, int no, double y)
	: BasicPlatform(gw, no, y), targetx(cb.x)
{
		std::uniform_real_distribution<> dist(0.5, 2.0);
		t = dist(mt);
}

void RestlessPlatform::process(Uint32 ms)
{
	t -= ms / 1000.0;
	if (t < 0.0)
	{
		std::uniform_real_distribution<> dist(0.5, 2.0);
		t = dist(mt);
		std::uniform_real_distribution<> pos(GameWorld::WALL_WIDTH, SCREEN_WIDTH - GameWorld::WALL_WIDTH - cb.w);
		targetx = pos(mt);
	}
	double dx = targetx - cb.x;
	double delta = 10.0 * dx * ms / 1000.0;
	cb.x += delta;
	if (this == gw->player.standingPlatform)
		gw->player.cb.x += delta;
}

ElevatorPlatform::ElevatorPlatform(GameWorld *gw, int no, double y)
	: BasicPlatform(gw, no, y), ay(0), vy(0)
{
}

void ElevatorPlatform::draw()
{
	Uint32 finalColor;
	if (darkMode)
		finalColor = SDL_MapRGB(screen->format, 0, 255, 255);
	else
		finalColor = SDL_MapRGB(screen->format, 255, 0, 0);

	SDL_Rect rect = {.x = (Sint16)cb.x, .y = (Sint16)cb.y, .w = (Uint16)cb.w, .h = (Uint16)cb.h};
	SDL_FillRect(screen, &rect, finalColor);
}

void ElevatorPlatform::process(Uint32 ms)
{
	if (this == gw->player.standingPlatform)
	{
		ay = -100.0;
	}
	else
	{
		ay = 100.0;
	}
	vy += ay * ms / 1000.0;
	if (vy > 0)
		vy = 0;
	if (vy < -MAX_SPEED)
		vy = -MAX_SPEED;
	double delta = vy * ms / 1000.0;
	cb.y += delta;
	if (this == gw->player.standingPlatform)
	{
		gw->player.cb.y += delta;
	}
	if (cb.y < -SCREEN_HEIGHT || (*(gw->platforms.begin()))->no > 401)
	{
		deleteFlag = true;
	}
}

SpringPlatform::SpringPlatform(GameWorld *gw, int no, double y)
	: BasicPlatform(gw, no, y)
{
}

MovingPlatform::MovingPlatform(GameWorld *gw, int no, double y, double freq)
	: gw{gw}, centerx{SCREEN_WIDTH / 2}, spanx{SCREEN_WIDTH / 2}, freq{freq}, t{0.0}
{
	this->no = no;
	this->cb.y = y;
	this->cb.h = IPlatform::DEFAULT_HEIGHT;
	std::uniform_int_distribution<int> udw(SCREEN_WIDTH / 6, 2 * SCREEN_WIDTH / 6);
	this->cb.w = udw(mt);
	if (0 == freq)
	{
		std::uniform_real_distribution<> udf(0.05, 0.2);
		this->freq = udf(mt);
	}
	const double pi = std::acos(-1);
	std::uniform_real_distribution<> udt(0, 2 * pi);
	this->t = udt(mt);
}

void MovingPlatform::draw()
{
	cb.draw();
}

void MovingPlatform::process(Uint32 ms)
{
	const double pi = std::acos(-1);
	t += ms / 1000.0;
	if (t > (1 / freq))
		t -= (1 / freq);
	double newx = centerx + (spanx / 2) * sin(2*pi*freq*t) - cb.w / 2;
	double delta = newx - cb.x;
	cb.x = newx;
	if (this == gw->player.standingPlatform)
		gw->player.cb.x += delta;
}

Player::Player()
{
	reset();
}

void Player::reset()
{
	cb.x = SCREEN_WIDTH / 2;
	cb.y = SCREEN_HEIGHT - 40;
	cb.w = Player::SIZE;
	cb.h = Player::SIZE;
	vx = 0;
	vy = 0;
	ax = 0;
	ay = Player::DEFAULT_ACCELERATION_Y;
	standingPlatform = nullptr;
	wannaJump = false;
	floorNo = 0;
}

void Player::draw()
{
	SDL_Rect r = {.x = (Sint16)cb.x, .y = (Sint16)cb.y, .w = (Uint16)(cb.w), .h = (Uint16)(cb.h)};
	SDL_FillRect(screen, &r, playerColor);
}

void Player::jump()
{
	standingPlatform = nullptr;
	vy = -JUMP_POWER - fabs(vx * vx * JUMP_COEFFICIENT);
}
