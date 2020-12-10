#ifndef _H_GAME
#define _H_GAME

#include <string>
#include <memory>
#include <random>
#include <list>
#include <SDL/SDL.h>

#include "gfx.hpp"

class GameWorld;

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

class IPlatform
{
public:
	static constexpr int DEFAULT_HEIGHT = 16;
	CollisionBox cb;
	int no;
	bool deleteFlag = false;
	virtual ~IPlatform() = default;
	virtual void draw() = 0;
	virtual void process(Uint32 ms) = 0;
};

class BasicPlatform : public IPlatform
{
public:
	std::string label = "";
	explicit BasicPlatform(GameWorld *gw, int no, double y);
	void draw() override;
	void process(Uint32 ms) override;
protected:
	GameWorld *gw;
};

class DisappearingPlatform : public BasicPlatform
{
public:
	bool running;
	explicit DisappearingPlatform(GameWorld *gw, int no, double y, double maxt = 0);
	void draw() override;
	void process(Uint32 ms) override;
private:
	double t;
	double maxt;
};

class FriendlyPlatform : public BasicPlatform
{
public:
	explicit FriendlyPlatform(GameWorld *gw, int no, double y);
	void process(Uint32 ms) override;
};

class EvilPlatform : public BasicPlatform
{
public:
	explicit EvilPlatform(GameWorld *gw, int no, double y);
	void process(Uint32 ms) override;
};

class ElevatorPlatform : public BasicPlatform
{
public:
	static constexpr double MAX_SPEED = 800.0;
	explicit ElevatorPlatform(GameWorld *gw, int no, double y);
	void draw() override;
	void process(Uint32 ms) override;
private:
	double ay;
	double vy;
};

class MovingPlatform : public IPlatform
{
public:
	explicit MovingPlatform(GameWorld *gw, int no, double y, double freq = 0);
	void draw() override;
	void process(Uint32 ms) override;
private:
	GameWorld *gw;
	double centerx;
	double spanx;
	double freq;
	double t;
};

class Player
{
public:
	static constexpr int SIZE = 16;
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
	IPlatform *standingPlatform;
	bool wannaJump;
	int floorNo;
	std::list<std::unique_ptr<IPlatform>>::iterator lastCollidedPlatform;
	Player();
	void reset();
	void draw();
	void jump();
};

class GameWorld
{
protected:
	double travelledDistance = 0.0;
	int hiscore = 0;
	int lastSavedHiscore = 0;
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
	Player player;
	std::list<std::unique_ptr<IPlatform>> platforms;
	GameWorld();
	~GameWorld();
	void draw();
	void handleEvents();
	void process(Uint32 ms);
	bool gameFinished();
	void reset();
	void printScore();
};

#endif
