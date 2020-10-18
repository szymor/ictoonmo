#include <iostream>
#include <cstdlib>

#include <SDL/SDL.h>

#include "gfx.hpp"
#include "game.hpp"

using std::cout;
using std::cerr;
using std::endl;

int main(int argc, char *argv[])
{
	try
	{
		SDLGuard sdl;
		GameWorld gw;

		Uint32 resetTimer = 0;
		Uint32 lastTicks = SDL_GetTicks();
		while (true)
		{
			if (!frameLimiter())
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
