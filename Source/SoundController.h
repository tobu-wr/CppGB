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

#include "types.h"

class Memory;

class SoundController
{
public:
	SoundController(Memory& memory);
	~SoundController();

	void generateSamples(u8* stream, int streamLength);

	void writeToNR13(u8 value);
	void writeToNR14(u8 value);
	void writeToNR23(u8 value);
	void writeToNR24(u8 value);
	void writeToNR33(u8 value);
	void writeToNR34(u8 value);
	void writeToNR44(u8 value);

private:
	void generateSamples_channel1(u8* stream, int streamLength);
	void generateSamples_channel2(u8* stream, int streamLength);
	void generateSamples_channel3(u8* stream, int streamLength);
	void generateSamples_channel4(u8* stream, int streamLength);

	Memory& m_memory;

	u8 m_levelDivisor_so1 = 0;
	u8 m_levelDivisor_so2 = 0;

	struct Channel
	{
		u32 sampleCounter = 0;
		f32 waveStepCountOffset = 0;
		f32 timeOffset = 0;
		f32 waveStepsPerSample = 0;
	} m_channel2, m_channel3;

	struct : Channel
	{
		u8 sweepShiftCounter = 0;
		u16 xShadowRegister = 0;
	} m_channel1;

	struct
	{
		u32 sampleCounter = 0;
		u16 lfsr = 0x7FFF;
	} m_channel4;
};
