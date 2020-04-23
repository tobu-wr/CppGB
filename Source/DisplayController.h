/*
Copyright 2017-2020 Wilfried Rabouin

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

#pragma once

#include <array>

#include "Types.h"

class Memory;
class Cpu;
struct SDL_Window;

class DisplayController
{
public:
	DisplayController(Memory& memory, Cpu& cpu);
	~DisplayController();

	void doCycle();
	void writeToLCDC(u8 value);

	u8 readBgPaletteColor();
	u8 readObjPaletteColor();
	void updateBgPaletteColor();
	void updateObjPaletteColor();

private:
	enum
	{
		SCREEN_WIDTH = 160,
		SCREEN_HEIGHT = 144
	};

	enum ModeFlag : u8
	{
		HBLANK_MODE_FLAG = 0,
		VBLANK_MODE_FLAG = 1,
		OAMSEARCH_MODE_FLAG = 2,
		PIXELTRANSFER_MODE_FLAG = 3
	};

#pragma warning(push)
#pragma warning(disable : 4201)
	union Color
	{
		struct { u8 L, H; };
		struct { u16 red : 5, green : 5, blue : 5; };
	};
#pragma warning(pop)

	struct ColorPalette
	{
		std::array<Color, 4> color{};
	};

	struct Pixel
	{
		u8 backgroundValue = 0;
		bool backgroundPriority = false;
		u8 dmgColor = 0;
		Color cgbColor{};
	};

	void updateLY(u8 value);
	void changeMode(ModeFlag flag);

	void transferPixelLine();
	void transferPixelLine_background();
	void transferPixelLine_objects();
	void transferPixelLine_window();

	void drawFrame();

	Memory& m_memory;
	Cpu& m_cpu;
	
	u8 m_cycleCounter = 0;
	std::array<Pixel, SCREEN_HEIGHT * SCREEN_WIDTH> m_frameBuffer;
	SDL_Window* m_window;

	std::array<ColorPalette, 8> m_bgColorPalettes;
	std::array<ColorPalette, 8> m_objColorPalettes;
};
