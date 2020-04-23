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

#include <list>

#include <SDL.h>

#include "Error.h"
#include "Cpu.h"
#include "DisplayController.h"
#include "Memory.h"

constexpr u8 CHARACTER_DATA_SIZE = 16;
constexpr u8 CHARACTER_WIDTH = 8;
constexpr u8 CHARACTERS_PER_LINE = 32;
constexpr u8 SCREEN_SCALE = 2;

DisplayController::DisplayController(Memory& memory, Cpu& cpu) : m_memory(memory), m_cpu(cpu)
{
	if (SDL_InitSubSystem(SDL_INIT_VIDEO))
		throwError("Failed to init video: ", SDL_GetError());

	m_window = SDL_CreateWindow("CppGB", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH * SCREEN_SCALE, SCREEN_HEIGHT * SCREEN_SCALE, SDL_WINDOW_SHOWN);
	
	if (!m_window)
		throwError("Failed to create window: ", SDL_GetError());
}

DisplayController::~DisplayController()
{
	SDL_DestroyWindow(m_window);
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

void DisplayController::doCycle()
{
	if ((m_memory.LCDC & 0x80) == 0)
		return;

	++m_cycleCounter;

	switch (m_memory.STAT & 0x03)
	{
	case HBLANK_MODE_FLAG:
		if (m_cycleCounter == 51)
		{
			updateLY(m_memory.LY + 1);

			if (m_memory.LY < 144)
				changeMode(OAMSEARCH_MODE_FLAG);
			else
			{
				drawFrame();
				changeMode(VBLANK_MODE_FLAG);
				m_cpu.requestInterrupt(Cpu::VBLANK_INTERRUPT_FLAG);
			}
		}
		break;

	case VBLANK_MODE_FLAG:
		if ((m_cycleCounter == 1) && (m_memory.LY == 153))
			updateLY(0);
		else if (m_cycleCounter == 114)
		{
			if (m_memory.LY == 0)
				changeMode(OAMSEARCH_MODE_FLAG);
			else
			{
				m_cycleCounter = 0;
				updateLY(m_memory.LY + 1);
			}
		}
		break;

	case OAMSEARCH_MODE_FLAG:
		if (m_cycleCounter == 20)
		{
			changeMode(PIXELTRANSFER_MODE_FLAG);
			transferPixelLine();
		}
		break;

	case PIXELTRANSFER_MODE_FLAG:
		if (m_cycleCounter == 43)
		{
			changeMode(HBLANK_MODE_FLAG);

			if ((m_memory.HDMA5 & 0x80) == 0)
				m_memory.performHdmaTransfer(0);

			if (m_memory.STAT & 0x08)
				m_cpu.requestInterrupt(Cpu::LCDSTAT_INTERRUPT_FLAG);
		}
		break;
	}
}

void DisplayController::writeToLCDC(u8 value)
{
	u8 oldValue = m_memory.LCDC;
	m_memory.LCDC = value;

	if ((value & 0x80) == 0 && (oldValue & 0x80)) // check if display is disabled
	{
		m_memory.LY = 0;
		changeMode(HBLANK_MODE_FLAG);
	}
}

u8 DisplayController::readBgPaletteColor()
{
	u8 paletteNumber = (m_memory.BCPS >> 3) & 0x07;
	u8 colorNumber = (m_memory.BCPS >> 1) & 0x03;

	if (m_memory.BCPS & 1)
		return m_bgColorPalettes[paletteNumber].color[colorNumber].H;
	else
		return m_bgColorPalettes[paletteNumber].color[colorNumber].L;
}

u8 DisplayController::readObjPaletteColor()
{
	u8 paletteNumber = (m_memory.OCPS >> 3) & 0x07;
	u8 colorNumber = (m_memory.OCPS >> 1) & 0x03;

	if (m_memory.OCPS & 1)
		return m_objColorPalettes[paletteNumber].color[colorNumber].H;
	else
		return m_objColorPalettes[paletteNumber].color[colorNumber].L;
}

void DisplayController::updateBgPaletteColor()
{
	u8 paletteNumber = (m_memory.BCPS >> 3) & 0x07;
	u8 colorNumber = (m_memory.BCPS >> 1) & 0x03;

	if (m_memory.BCPS & 1)
		m_bgColorPalettes[paletteNumber].color[colorNumber].H = m_memory.BCPD;
	else
		m_bgColorPalettes[paletteNumber].color[colorNumber].L = m_memory.BCPD;

	if (m_memory.BCPS & 0x80)
		m_memory.BCPS = (m_memory.BCPS & 0xBF) + 1;
}

void DisplayController::updateObjPaletteColor()
{
	u8 paletteNumber = (m_memory.OCPS >> 3) & 0x07;
	u8 colorNumber = (m_memory.OCPS >> 1) & 0x03;

	if (m_memory.OCPS & 1)
		m_objColorPalettes[paletteNumber].color[colorNumber].H = m_memory.OCPD;
	else
		m_objColorPalettes[paletteNumber].color[colorNumber].L = m_memory.OCPD;

	if (m_memory.OCPS & 0x80)
		m_memory.OCPS = (m_memory.OCPS & 0xBF) + 1;
}

void DisplayController::updateLY(u8 value)
{
	m_memory.LY = value;

	if ((m_memory.LY == m_memory.LYC) && (m_memory.STAT & 0x40))
		m_cpu.requestInterrupt(Cpu::LCDSTAT_INTERRUPT_FLAG);
}

void DisplayController::changeMode(ModeFlag flag)
{
	m_cycleCounter = 0;
	m_memory.STAT = (m_memory.STAT & 0xFC) | flag;
}

void DisplayController::transferPixelLine()
{
	if (m_memory.LCDC & 0x01)
		transferPixelLine_background();

	if (m_memory.LCDC & 0x20)
		transferPixelLine_window();

	if (m_memory.LCDC & 0x02)
		transferPixelLine_objects();
}

void DisplayController::transferPixelLine_background()
{
	u16 characterCodeAreaAddress = (m_memory.LCDC & 0x08) ? 0x9C00 : 0x9800;

	u8 y_background = m_memory.LY + m_memory.SCY;
	u8 characterLine = y_background / CHARACTER_WIDTH;

	for (u8 x_screen = 0; x_screen < SCREEN_WIDTH; ++x_screen)
	{
		u8 x_background = x_screen + m_memory.SCX;
		u8 characterColumn = x_background / CHARACTER_WIDTH;
		u8 x_character = x_background % CHARACTER_WIDTH;

		u16 characterNumber = characterLine * CHARACTERS_PER_LINE + characterColumn;
		u16 characterAddress = characterCodeAreaAddress + characterNumber;
		u8 characterCode = m_memory.readDisplayRam(characterAddress, 0);
		u8 characterAttributes = m_memory.readDisplayRam(characterAddress, 1);

		u8 colorPaletteNumber = characterAttributes & 0x07;
		u8 characterDataBankNumber = (characterAttributes & 0x08) >> 3;
		bool horizontalFlip = characterAttributes & 0x20;
		bool verticalFlip = characterAttributes & 0x40;
		bool backgroundPriority = characterAttributes & 0x80;

		u8 y_character = verticalFlip ? (7 - y_background % CHARACTER_WIDTH) : (y_background % CHARACTER_WIDTH);

		u16 characterDataAddress = (m_memory.LCDC & 0x10) ? (0x8000 + characterCode * CHARACTER_DATA_SIZE) : (0x9000 + (s8)characterCode * CHARACTER_DATA_SIZE);
		u8 byte0 = m_memory.readDisplayRam(characterDataAddress + y_character * 2, characterDataBankNumber);
		u8 byte1 = m_memory.readDisplayRam(characterDataAddress + y_character * 2 + 1, characterDataBankNumber);
		u8 bitNumber = horizontalFlip ? x_character : 7 - x_character;
		u8 bit0 = (byte0 >> bitNumber) & 1;
		u8 bit1 = (byte1 >> bitNumber) & 1;
		u8 pixel = (bit1 << 1) | bit0;
		u16 pixelOffset = m_memory.LY * SCREEN_WIDTH + x_screen;

		m_frameBuffer[pixelOffset].backgroundValue = pixel;
		m_frameBuffer[pixelOffset].backgroundPriority = backgroundPriority;
		m_frameBuffer[pixelOffset].dmgColor = (m_memory.BGP >> (pixel * 2)) & 0x03;
		m_frameBuffer[pixelOffset].cgbColor = m_bgColorPalettes[colorPaletteNumber].color[pixel];
	}
}

void DisplayController::transferPixelLine_objects()
{
	constexpr u8 OBJECT_COUNT = 40;
	constexpr u8 MAX_VISIBLE_OBJECTS = 10;
	constexpr u8 BYTES_PER_OBJECT = 4;
	constexpr u8 MAX_OBJECT_HEIGHT = 16;
	constexpr u8 OBJECT_WIDTH = 8;

	u8 objectHeight = (m_memory.LCDC & 0x04) ? 16 : 8;

	std::list<u16> visibleObjectAddresses;

	for (u8 objectNumber = 0; (objectNumber < OBJECT_COUNT) && (visibleObjectAddresses.size() < MAX_VISIBLE_OBJECTS); ++objectNumber)
	{
		u16 objectAddress = Memory::OAM_ADDRESS + objectNumber * BYTES_PER_OBJECT;
		s16 objectY = m_memory.read(objectAddress) - MAX_OBJECT_HEIGHT;

		if ((objectY <= m_memory.LY) && (m_memory.LY < objectY + objectHeight))
			visibleObjectAddresses.push_front(objectAddress);
	}

	for (u16 objectAddress : visibleObjectAddresses)
	{
		u8 objectY = m_memory.read(objectAddress) - MAX_OBJECT_HEIGHT;
		u8 objectX = m_memory.read(objectAddress + 1) - OBJECT_WIDTH;
		u8 characterCode = m_memory.read(objectAddress + 2);
		u8 objectAttributes = m_memory.read(objectAddress + 3);

		if (objectHeight == 16)
			characterCode &= 0xFE;

		u8 colorPaletteNumber = objectAttributes & 0x07;
		u8 characterDataBankNumber = (objectAttributes & 0x08) >> 3;
		u8 OBP = (objectAttributes & 0x10) ? m_memory.OBP1 : m_memory.OBP0;
		bool horizontalFlip = objectAttributes & 0x20;
		bool verticalFlip = objectAttributes & 0x40;
		bool backgroundPriority = objectAttributes & 0x80;

		u8 y_object = verticalFlip ? (objectHeight - 1 - m_memory.LY + objectY) : (m_memory.LY - objectY);
		
		u16 characterDataAddress = 0x8000 + characterCode * CHARACTER_DATA_SIZE;
		u8 byte0 = m_memory.readDisplayRam(characterDataAddress + y_object * 2, characterDataBankNumber);
		u8 byte1 = m_memory.readDisplayRam(characterDataAddress + y_object * 2 + 1, characterDataBankNumber);

		for (u8 x_screen = (objectX < SCREEN_WIDTH ? objectX : 0), x_object = (objectX < SCREEN_WIDTH ? 0 : - objectX); (x_screen < SCREEN_WIDTH) && (x_object < OBJECT_WIDTH); ++x_screen, ++x_object)
		{
			u16 pixelOffset = m_memory.LY * SCREEN_WIDTH + x_screen;

			if ((!backgroundPriority && !m_frameBuffer[pixelOffset].backgroundPriority) || (m_frameBuffer[pixelOffset].backgroundValue == 0))
			{
				u8 bitNumber = horizontalFlip ? x_object : 7 - x_object;
				u8 bit0 = (byte0 >> bitNumber) & 1;
				u8 bit1 = (byte1 >> bitNumber) & 1;
				u8 pixel = (bit1 << 1) | bit0;

				if (pixel != 0) // 0 => transparent
				{
					m_frameBuffer[pixelOffset].dmgColor = (OBP >> (pixel * 2)) & 0x03;
					m_frameBuffer[pixelOffset].cgbColor = m_objColorPalettes[colorPaletteNumber].color[pixel];
				}
			}
		}
	}
}

void DisplayController::transferPixelLine_window()
{
	if (m_memory.WY > m_memory.LY)
		return;

	u16 characterCodeAreaAddress = (m_memory.LCDC & 0x40) ? 0x9C00 : 0x9800;

	u8 y_window = m_memory.LY - m_memory.WY;
	u8 characterLine = y_window / CHARACTER_WIDTH;
	u8 y_character = y_window % CHARACTER_WIDTH;

	u8 windowX = m_memory.WX - 7;

	for (u8 x_screen = (m_memory.WX > 7 ? windowX : 0); x_screen < SCREEN_WIDTH; ++x_screen)
	{
		u8 x_window = x_screen - windowX;
		u8 characterColumn = x_window / CHARACTER_WIDTH;
		u8 x_character = x_window % CHARACTER_WIDTH;

		u16 characterNumber = characterLine * CHARACTERS_PER_LINE + characterColumn;
		u16 characterAddress = characterCodeAreaAddress + characterNumber;
		u8 characterCode = m_memory.readDisplayRam(characterAddress, 0);
		u8 characterAttributes = m_memory.readDisplayRam(characterAddress, 1);

		u8 colorPaletteNumber = characterAttributes & 0x07;
		u8 characterDataBankNumber = (characterAttributes & 0x08) >> 3;
		bool horizontalFlip = characterAttributes & 0x20;
		bool backgroundPriority = characterAttributes & 0x80;

		u16 characterDataAddress = (m_memory.LCDC & 0x10) ? (0x8000 + characterCode * CHARACTER_DATA_SIZE) : (0x9000 + (s8)characterCode * CHARACTER_DATA_SIZE);
		u8 byte0 = m_memory.readDisplayRam(characterDataAddress + y_character * 2, characterDataBankNumber);
		u8 byte1 = m_memory.readDisplayRam(characterDataAddress + y_character * 2 + 1, characterDataBankNumber);
		u8 bitNumber = horizontalFlip ? x_character : 7 - x_character;
		u8 bit0 = (byte0 >> bitNumber) & 1;
		u8 bit1 = (byte1 >> bitNumber) & 1;
		u8 pixel = (bit1 << 1) | bit0;
		u16 pixelOffset = m_memory.LY * SCREEN_WIDTH + x_screen;

		m_frameBuffer[pixelOffset].backgroundValue = pixel;
		m_frameBuffer[pixelOffset].backgroundPriority = backgroundPriority;
		m_frameBuffer[pixelOffset].dmgColor = (m_memory.BGP >> (pixel * 2)) & 0x03;
		m_frameBuffer[pixelOffset].cgbColor = m_bgColorPalettes[colorPaletteNumber].color[pixel];
	}
}

void DisplayController::drawFrame()
{
	SDL_Surface* windowSurface = SDL_GetWindowSurface(m_window);

	SDL_Rect rect;
	rect.w = SCREEN_SCALE;
	rect.h = SCREEN_SCALE;

	for (u16 pixelOffset = 0; pixelOffset < m_frameBuffer.size(); ++pixelOffset)
	{
		rect.x = (pixelOffset % SCREEN_WIDTH) * SCREEN_SCALE;
		rect.y = (pixelOffset / SCREEN_WIDTH) * SCREEN_SCALE;

		u32 color = [&]
		{
			if (m_cpu.isCgbMode())
			{
				Color pixelColor = m_frameBuffer[pixelOffset].cgbColor;
				u8 red = (u8)((0xFF * pixelColor.red) / 0x1F);
				u8 green = (u8)((0xFF * pixelColor.green) / 0x1F);
				u8 blue = (u8)((0xFF * pixelColor.blue) / 0x1F);
				return SDL_MapRGB(windowSurface->format, red, green, blue);
			}
			else
			{
				switch (m_frameBuffer[pixelOffset].dmgColor)
				{
				case 0: return SDL_MapRGB(windowSurface->format, 0xFF, 0xFF, 0xFF); // white
				case 1: return SDL_MapRGB(windowSurface->format, 0xAA, 0xAA, 0xAA); // light gray
				case 2: return SDL_MapRGB(windowSurface->format, 0x55, 0x55, 0x55); // dark gray
				default: return SDL_MapRGB(windowSurface->format, 0, 0, 0); // black
				}
			}
		}();

		SDL_FillRect(windowSurface, &rect, color);
	}

	SDL_UpdateWindowSurface(m_window);
}
