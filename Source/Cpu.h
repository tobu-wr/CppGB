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

#pragma once

#include "EventHandler.h"
#include "DisplayController.h"
#include "SoundController.h"
#include "Memory.h"

class Cpu
{
public:
	enum InterruptFlag : u8
	{
		VBLANK_INTERRUPT_FLAG = 0x01,
		LCDSTAT_INTERRUPT_FLAG = 0x02,
		TIMER_INTERRUPT_FLAG = 0x04, 
		SERIALTRANSFER_INTERRUPT_FLAG = 0x08, 
		JOYPAD_INTERRUPT_FLAG = 0x10,
	};

	Cpu(Memory& memory);
	void run();
	void requestInterrupt(InterruptFlag flag);
	bool isCgbMode();
	
private:
	void executeNextInstruction();

	void handleInterrupts();
	void performInterrupt(InterruptFlag flag, Memory::InterruptAddress address);

	u8 fetch_u8();
	u16 fetch_u16();

	void doCycle(u8 cycleCount = 1);
	void incrementDIV();
	void incrementTIMA();

	u8 readMemory_u8(u16 address);
	u16 readMemory_u16(u16 address);
	void writeToMemory(u16 address, u8 value);

	void push(u16 value);
	u16 pop();
	void add_A(u8 value);
	void add_HL(u16 value);
	void adc(u8 value);
	void sub(u8 value);
	void sbc(u8 value);
	void and_(u8 value);
	void or_(u8 value);
	void xor_(u8 value);
	void cp(u8 value);
	void inc(u8& value);
	void dec(u8& value);
	void swap(u8& value);
	void rlc(u8& value);
	void rl(u8& value);
	void rrc(u8& value);
	void rr(u8& value);
	void sla(u8& value);
	void sra(u8& value);
	void srl(u8& value);
	void bit(u8 value, u8 n);
	void set(u8& value, u8 n);
	void res(u8& value, u8 n);
	void call(bool cc);
	void rst(u16 address);
	void daa();
	u16 add_SP_e();
	void jr(bool cc);
	void ret(bool cc);
	void jp(bool cc);

#pragma warning(push)
#pragma warning(disable : 4201)
	union
	{
		struct { u16 AF, BC, DE, HL, SP, PC; };
		struct
		{
			struct { u8: 4, C : 1, H : 1, N : 1, Z : 1; } F;
			u8 A, C, B, E, D, L, H;
			u8 SP_lowbyte, SP_highbyte;
		};
	} m_registers{};
#pragma warning(pop)

	Memory& m_memory;
	DisplayController m_displayController;
	SoundController m_soundController;

	bool m_ime = false;
	bool m_haltMode = false;
};
