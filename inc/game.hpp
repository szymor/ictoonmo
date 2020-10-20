#ifndef _H_GAME
#define _H_GAME

#include <random>
#include <list>
#include <SDL/SDL.h>

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
	std::list<Platform>::iterator lastCollidedPlatform;
	void draw();
	void jump();
};

class GameWorld
{
protected:
	std::random_device rd;
	std::mt19937 mt;
	int hiscore;
	int lastSavedHiscore;
	Player player;
	std::list<Platform> platforms;
	void addPlatform(int x, int y, int w, int no);
	void saveHiscore();
	void loadHiscore();
public:
	static constexpr int WALL_WIDTH = 4;
	static constexpr double BOUNCINESS = 0.7;
	static constexpr double PLATFORM_DISTANCE = 40;
	static constexpr double PACE_COEFFICIENT = 0.005;
	static constexpr Uint32 RESET_TIMEOUT = 2000;
	static constexpr char GAMEDIR[] = ".ictoonmo";
	static constexpr char HISCORE_FILE[] = "hiscore.dat";
	GameWorld();
	~GameWorld();
	void draw();
	void switchColors();
	void handleEvents();
	void process(Uint32 ms);
	bool gameFinished();
	void reset();
	void printScore();
};

#endif
