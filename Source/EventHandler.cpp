/*
Copyright 2017-2019 Wilfried Rabouin

This file is part of CppGB.

CppGB is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

CppGB is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with CppGB.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <SDL.h>

#include "EventHandler.h"

void EventHandler::updateP1(u8& P1)
{
	const u8* keyboardState = SDL_GetKeyboardState(nullptr);

	P1 |= 0x0F; // input ports reset

	if ((P1 & 0x10) == 0)
	{
		u8 right = keyboardState[SDL_SCANCODE_RIGHT];
		u8 left = keyboardState[SDL_SCANCODE_LEFT];
		u8 up = keyboardState[SDL_SCANCODE_UP];
		u8 down = keyboardState[SDL_SCANCODE_DOWN];

		P1 ^= (down << 3) | (up << 2) | (left << 1) | right;
	}
	else if ((P1 & 0x20) == 0)
	{
		u8 a = keyboardState[SDL_SCANCODE_Q];
		u8 b = keyboardState[SDL_SCANCODE_W];
		u8 select = keyboardState[SDL_SCANCODE_SPACE];
		u8 start = keyboardState[SDL_SCANCODE_RETURN];

		P1 ^= (start << 3) | (select << 2) | (b << 1) | a;
	}
}

bool EventHandler::isQuitRequested()
{
	SDL_Event event;
	SDL_PollEvent(&event);
	return event.type == SDL_QUIT;
}
