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

#include "error.h"
#include "Cpu.h"
#include "EventHandler.h"

Cpu::Cpu(Memory& memory) : m_memory(memory), m_displayController(memory, *this), m_soundController(memory)
{
	m_registers.SP = 0xFFFE;
	m_registers.PC = 0x100;
	m_registers.A = 0x11;
}

void Cpu::run()
{
	while (!EventHandler::isQuitRequested())
	{
		EventHandler::updateP1(m_memory.P1);
		handleInterrupts();

		if (m_haltMode)
			doCycle();
		else
			executeNextInstruction();
	}
}

void Cpu::executeNextInstruction()
{
	u8 opcode = fetch_u8();

	switch (opcode)
	{
		// NOP
	case 0x00:
		break;

		// LD BC,nn
	case 0x01:
		m_registers.BC = fetch_u16();
		break;

		// LD (BC),A
	case 0x02:
		writeToMemory(m_registers.BC, m_registers.A);
		break;

		// INC BC
	case 0x03:
		++m_registers.BC;
		doCycle();
		break;

		// INC B
	case 0x04:
		inc(m_registers.B);
		break;

		// DEC B
	case 0x05:
		dec(m_registers.B);
		break;

		// LD B,n
	case 0x06:
		m_registers.B = fetch_u8();
		break;

		// RLCA
	case 0x07:
		m_registers.F.C = m_registers.A >> 7;
		m_registers.A = (m_registers.A << 1) | m_registers.F.C;
		m_registers.F.Z = 0;
		m_registers.F.N = 0;
		m_registers.F.H = 0;
		break;

		// LD (nn),SP
	case 0x08:
	{
		u16 address = fetch_u16();
		writeToMemory(address, m_registers.SP_lowbyte);
		writeToMemory(address + 1, m_registers.SP_highbyte);
		break;
	}

		// ADD HL,BC
	case 0x09:
		add_HL(m_registers.BC);
		break;

		// LD A,(BC)
	case 0x0A:
		m_registers.A = readMemory_u8(m_registers.BC);
		break;

		// DEC BC
	case 0x0B:
		--m_registers.BC;
		doCycle();
		break;

		// INC C
	case 0x0C:
		inc(m_registers.C);
		break;

		// DEC C
	case 0x0D:
		dec(m_registers.C);
		break;

		// LD C,n
	case 0x0E:
		m_registers.C = fetch_u8();
		break;

		// RRCA
	case 0x0F:
		m_registers.F.C = m_registers.A & 1;
		m_registers.A = (m_registers.A >> 1) | (m_registers.F.C << 7);
		m_registers.F.Z = 0;
		m_registers.F.N = 0;
		m_registers.F.H = 0;
		break;

		// STOP (not implemented)
	case 0x10:
		// speed switching
		if (m_memory.KEY1 & 0x01)
			m_memory.KEY1 ^= 0x81;
		
		++m_registers.PC;
		break;

		// LD DE,nn
	case 0x11:
		m_registers.DE = fetch_u16();
		break;

		// LD (DE),A
	case 0x12:
		writeToMemory(m_registers.DE, m_registers.A);
		break;

		// INC DE
	case 0x13:
		++m_registers.DE;
		doCycle();
		break;

		// INC D
	case 0x14:
		inc(m_registers.D);
		break;

		// DEC D
	case 0x15:
		dec(m_registers.D);
		break;

		// LD D,n
	case 0x16:
		m_registers.D = fetch_u8();
		break;

		// RLA
	case 0x17:
	{
		u8 oldCarry = m_registers.F.C;
		m_registers.F.C = m_registers.A >> 7;
		m_registers.A = (m_registers.A << 1) | oldCarry;
		m_registers.F.Z = 0;
		m_registers.F.N = 0;
		m_registers.F.H = 0;
		break;
	}

		// JR e
	case 0x18:
	{
		s8 value = fetch_u8();
		m_registers.PC += value;
		doCycle();
		break;
	}
		
		// ADD HL,DE
	case 0x19:
		add_HL(m_registers.DE);
		break;

		// LD A,(DE)
	case 0x1A:
		m_registers.A = readMemory_u8(m_registers.DE);
		break;

		// DEC DE
	case 0x1B:
		--m_registers.DE;
		doCycle();
		break;

		// INC E
	case 0x1C:
		inc(m_registers.E);
		break;

		// DEC E
	case 0x1D:
		dec(m_registers.E);
		break;

		// LD E,n
	case 0x1E:
		m_registers.E = fetch_u8();
		break;

		// RRA
	case 0x1F:
	{
		u8 oldCarry = m_registers.F.C;
		m_registers.F.C = m_registers.A & 1;
		m_registers.A = (m_registers.A >> 1) | (oldCarry << 7);
		m_registers.F.Z = 0;
		m_registers.F.N = 0;
		m_registers.F.H = 0;
		break;
	}

		// JR NZ,e
	case 0x20:
		jr(!m_registers.F.Z);
		break;

		// LD HL,nn
	case 0x21:
		m_registers.HL = fetch_u16();
		break;

		// LD (HLI),A
	case 0x22:
		writeToMemory(m_registers.HL, m_registers.A);
		++m_registers.HL;
		break;

		// INC HL
	case 0x23:
		++m_registers.HL;
		doCycle();
		break;

		// INC H
	case 0x24:
		inc(m_registers.H);
		break;

		// DEC H
	case 0x25:
		dec(m_registers.H);
		break;

		// LD H,n
	case 0x26:
		m_registers.H = fetch_u8();
		break;

		// DAA
	case 0x27:
		daa();
		break;

		// JR Z,e
	case 0x28:
		jr(m_registers.F.Z);
		break;

		// ADD HL,HL
	case 0x29:
		add_HL(m_registers.HL);
		break;

		// LD A,(HLI)
	case 0x2A:
		m_registers.A = readMemory_u8(m_registers.HL);
		++m_registers.HL;
		break;

		// DEC HL
	case 0x2B:
		--m_registers.HL;
		doCycle();
		break;

		// INC L
	case 0x2C:
		inc(m_registers.L);
		break;

		// DEC L
	case 0x2D:
		dec(m_registers.L);
		break;

		// LD L,n
	case 0x2E:
		m_registers.L = fetch_u8();
		break;

		// CPL
	case 0x2F:
		m_registers.A = ~m_registers.A;
		m_registers.F.N = 1;
		m_registers.F.H = 1;
		break;

		// JR NC,e
	case 0x30:
		jr(!m_registers.F.C);
		break;

		// LD SP,nn
	case 0x31:
		m_registers.SP = fetch_u16();
		break;

		// LD (HLD),A
	case 0x32:
		writeToMemory(m_registers.HL, m_registers.A);
		--m_registers.HL;
		break;

		// INC SP
	case 0x33:
		++m_registers.SP;
		doCycle();
		break;

		// INC (HL)
	case 0x34:
	{
		u8 value = readMemory_u8(m_registers.HL);
		inc(value);
		writeToMemory(m_registers.HL, value);
		break;
	}

		// DEC (HL)
	case 0x35:
	{
		u8 value = readMemory_u8(m_registers.HL);
		dec(value);
		writeToMemory(m_registers.HL, value);
		break;
	}

		// LD (HL),n
	case 0x36:
	{
		u8 value = fetch_u8();
		writeToMemory(m_registers.HL, value);
		break;
	}

		// SCF
	case 0x37:
		m_registers.F.C = 1;
		m_registers.F.N = 0;
		m_registers.F.H = 0;
		break;

		// JR C,e
	case 0x38:
		jr(m_registers.F.C);
		break;

		// ADD HL,SP
	case 0x39:
		add_HL(m_registers.SP);
		break;

		// LD A,(HLD)
	case 0x3A:
		m_registers.A = readMemory_u8(m_registers.HL);
		--m_registers.HL;
		break;

		// DEC SP
	case 0x3B:
		--m_registers.SP;
		doCycle();
		break;

		// INC A
	case 0x3C:
		inc(m_registers.A);
		break;

		// DEC A
	case 0x3D:
		dec(m_registers.A);
		break;

		// LD A,n
	case 0x3E:
		m_registers.A = fetch_u8();
		break;

		// CCF
	case 0x3F:
		m_registers.F.C = ~m_registers.F.C;
		m_registers.F.N = 0;
		m_registers.F.H = 0;
		break;

		// LD B,B
	case 0x40:
		break;

		// LD B,C
	case 0x41:
		m_registers.B = m_registers.C;
		break;

		// LD B,D
	case 0x42:
		m_registers.B = m_registers.D;
		break;

		// LD B,E
	case 0x43:
		m_registers.B = m_registers.E;
		break;

		// LD B,H
	case 0x44:
		m_registers.B = m_registers.H;
		break;

		// LD B,L
	case 0x45:
		m_registers.B = m_registers.L;
		break;

		// LD B,(HL)
	case 0x46:
		m_registers.B = readMemory_u8(m_registers.HL);
		break;

		// LD B,A
	case 0x47:
		m_registers.B = m_registers.A;
		break;

		// LD C,B
	case 0x48:
		m_registers.C = m_registers.B;
		break;

		// LD C,C
	case 0x49:
		break;

		// LD C,D
	case 0x4A:
		m_registers.C = m_registers.D;
		break;

		// LD C,E
	case 0x4B:
		m_registers.C = m_registers.E;
		break;

		// LD C,H
	case 0x4C:
		m_registers.C = m_registers.H;
		break;

		// LD C,L
	case 0x4D:
		m_registers.C = m_registers.L;
		break;

		// LD C,(HL)
	case 0x4E:
		m_registers.C = readMemory_u8(m_registers.HL);
		break;

		// LD C,A
	case 0x4F:
		m_registers.C = m_registers.A;
		break;

		// LD D,B
	case 0x50:
		m_registers.D = m_registers.B;
		break;

		// LD D,C
	case 0x51:
		m_registers.D = m_registers.C;
		break;

		// LD D,D
	case 0x52:
		break;

		// LD D,E
	case 0x53:
		m_registers.D = m_registers.E;
		break;

		// LD D,H
	case 0x54:
		m_registers.D = m_registers.H;
		break;

		// LD D,L
	case 0x55:
		m_registers.D = m_registers.L;
		break;

		// LD D,(HL)
	case 0x56:
		m_registers.D = readMemory_u8(m_registers.HL);
		break;

		// LD D,A
	case 0x57:
		m_registers.D = m_registers.A;
		break;

		// LD E,B
	case 0x58:
		m_registers.E = m_registers.B;
		break;

		// LD E,C
	case 0x59:
		m_registers.E = m_registers.C;
		break;

		// LD E,D
	case 0x5A:
		m_registers.E = m_registers.D;
		break;

		// LD E,E
	case 0x5B:
		break;

		// LD E,H
	case 0x5C:
		m_registers.E = m_registers.H;
		break;

		// LD E,L
	case 0x5D:
		m_registers.E = m_registers.L;
		break;

		// LD E,(HL)
	case 0x5E:
		m_registers.E = readMemory_u8(m_registers.HL);
		break;

		// LD E,A
	case 0x5F:
		m_registers.E = m_registers.A;
		break;

		// LD H,B
	case 0x60:
		m_registers.H = m_registers.B;
		break;

		// LD H,C
	case 0x61:
		m_registers.H = m_registers.C;
		break;

		// LD H,D
	case 0x62:
		m_registers.H = m_registers.D;
		break;

		// LD H,E
	case 0x63:
		m_registers.H = m_registers.E;
		break;

		// LD H,H
	case 0x64:
		break;

		// LD H,L
	case 0x65:
		m_registers.H = m_registers.L;
		break;

		// LD H,(HL)
	case 0x66:
		m_registers.H = readMemory_u8(m_registers.HL);
		break;

		// LD H,A
	case 0x67:
		m_registers.H = m_registers.A;
		break;

		// LD L,B
	case 0x68:
		m_registers.L = m_registers.B;
		break;

		// LD L,C
	case 0x69:
		m_registers.L = m_registers.C;
		break;

		// LD L,D
	case 0x6A:
		m_registers.L = m_registers.D;
		break;

		// LD L,E
	case 0x6B:
		m_registers.L = m_registers.E;
		break;

		// LD L,H
	case 0x6C:
		m_registers.L = m_registers.H;
		break;

		// LD L,L
	case 0x6D:
		break;

		// LD L,(HL)
	case 0x6E:
		m_registers.L = readMemory_u8(m_registers.HL);
		break;

		// LD L,A
	case 0x6F:
		m_registers.L = m_registers.A;
		break;

		// LD (HL),B
	case 0x70:
		writeToMemory(m_registers.HL, m_registers.B);
		break;

		// LD (HL),C
	case 0x71:
		writeToMemory(m_registers.HL, m_registers.C);
		break;

		// LD (HL),D
	case 0x72:
		writeToMemory(m_registers.HL, m_registers.D);
		break;

		// LD (HL),E
	case 0x73:
		writeToMemory(m_registers.HL, m_registers.E);
		break;

		// LD (HL),H
	case 0x74:
		writeToMemory(m_registers.HL, m_registers.H);
		break;

		// LD (HL),L
	case 0x75:
		writeToMemory(m_registers.HL, m_registers.L);
		break;

		// HALT
	case 0x76:
		m_haltMode = true;
		break;

		// LD (HL),A
	case 0x77:
		writeToMemory(m_registers.HL, m_registers.A);
		break;

		// LD A,B
	case 0x78:
		m_registers.A = m_registers.B;
		break;

		// LD A,C
	case 0x79:
		m_registers.A = m_registers.C;
		break;

		// LD A,D
	case 0x7A:
		m_registers.A = m_registers.D;
		break;

		// LD A,E
	case 0x7B:
		m_registers.A = m_registers.E;
		break;

		// LD A,H
	case 0x7C:
		m_registers.A = m_registers.H;
		break;

		// LD A,L
	case 0x7D:
		m_registers.A = m_registers.L;
		break;

		// LD A,(HL)
	case 0x7E:
		m_registers.A = readMemory_u8(m_registers.HL);
		break;

		// LD A,A
	case 0x7F:
		break;

		// ADD A,B
	case 0x80:
		add_A(m_registers.B);
		break;

		// ADD A,C
	case 0x81:
		add_A(m_registers.C);
		break;

		// ADD A,D
	case 0x82:
		add_A(m_registers.D);
		break;

		// ADD A,E
	case 0x83:
		add_A(m_registers.E);
		break;

		// ADD A,H
	case 0x84:
		add_A(m_registers.H);
		break;

		// ADD A,L
	case 0x85:
		add_A(m_registers.L);
		break;

		// ADD A,(HL)
	case 0x86:
	{
		u8 value = readMemory_u8(m_registers.HL);
		add_A(value);
		break;
	}

		// ADD A,A
	case 0x87:
		add_A(m_registers.A);
		break;

		// ADC A,B
	case 0x88:
		adc(m_registers.B);
		break;

		// ADC A,C
	case 0x89:
		adc(m_registers.C);
		break;

		// ADC A,D
	case 0x8A:
		adc(m_registers.D);
		break;

		// ADC A,E
	case 0x8B:
		adc(m_registers.E);
		break;

		// ADC A,H
	case 0x8C:
		adc(m_registers.H);
		break;

		// ADC A,L
	case 0x8D:
		adc(m_registers.L);
		break;

		// ADC A,(HL)
	case 0x8E:
	{
		u8 value = readMemory_u8(m_registers.HL);
		adc(value);
		break;
	}

		// ADC A,A
	case 0x8F:
		adc(m_registers.A);
		break;

		// SUB A,B
	case 0x90:
		sub(m_registers.B);
		break;

		// SUB C
	case 0x91:
		sub(m_registers.C);
		break;

		// SUB A,D
	case 0x92:
		sub(m_registers.D);
		break;

		// SUB A,E
	case 0x93:
		sub(m_registers.E);
		break;

		// SUB A,H
	case 0x94:
		sub(m_registers.H);
		break;

		// SUB A,L
	case 0x95:
		sub(m_registers.L);
		break;

		// SUB A,(HL)
	case 0x96:
	{
		u8 value = readMemory_u8(m_registers.HL);
		sub(value);
		break;
	}

		// SUB A,A
	case 0x97:
		sub(m_registers.A);
		break;

		// SBC A,B
	case 0x98:
		sbc(m_registers.B);
		break;

		// SBC A,C
	case 0x99:
		sbc(m_registers.C);
		break;

		// SBC A,D
	case 0x9A:
		sbc(m_registers.D);
		break;

		// SBC A,E
	case 0x9B:
		sbc(m_registers.E);
		break;

		// SBC A,H
	case 0x9C:
		sbc(m_registers.H);
		break;

		// SBC A,L
	case 0x9D:
		sbc(m_registers.L);
		break;

		// SBC A,(HL)
	case 0x9E:
	{
		u8 value = readMemory_u8(m_registers.HL);
		sbc(value);
		break;
	}

		// SBC A,A
	case 0x9F:
		sbc(m_registers.A);
		break;

		// AND B
	case 0xA0:
		and_(m_registers.B);
		break;

		// AND C
	case 0xA1:
		and_(m_registers.C);
		break;

		// AND D
	case 0xA2:
		and_(m_registers.D);
		break;

		// AND E
	case 0xA3:
		and_(m_registers.E);
		break;

		// AND H
	case 0xA4:
		and_(m_registers.H);
		break;

		// AND L
	case 0xA5:
		and_(m_registers.L);
		break;

		// AND (HL)
	case 0xA6:
	{
		u8 value = readMemory_u8(m_registers.HL);
		and_(value);
		break;
	}

		// AND A
	case 0xA7:
		and_(m_registers.A);
		break;

		// XOR B
	case 0xA8:
		xor_(m_registers.B);
		break;

		// XOR C
	case 0xA9:
		xor_(m_registers.C);
		break;

		// XOR D
	case 0xAA:
		xor_(m_registers.D);
		break;

		// XOR E
	case 0xAB:
		xor_(m_registers.E);
		break;

		// XOR H
	case 0xAC:
		xor_(m_registers.H);
		break;

		// XOR L
	case 0xAD:
		xor_(m_registers.L);
		break;

		// XOR (HL)
	case 0xAE:
	{
		u8 value = readMemory_u8(m_registers.HL);
		xor_(value);
		break;
	}

		// XOR A
	case 0xAF:
		xor_(m_registers.A);
		break;

		// OR B
	case 0xB0:
		or_(m_registers.B);
		break;

		// OR C
	case 0xB1:
		or_(m_registers.C);
		break;

		// OR D
	case 0xB2:
		or_(m_registers.D);
		break;

		// OR E
	case 0xB3:
		or_(m_registers.E);
		break;

		// OR H
	case 0xB4:
		or_(m_registers.H);
		break;

		// OR L
	case 0xB5:
		or_(m_registers.L);
		break;

		// OR (HL)
	case 0xB6:
	{
		u8 value = readMemory_u8(m_registers.HL);
		or_(value);
		break;
	}

		// OR A
	case 0xB7:
		or_(m_registers.A);
		break;

		// CP B
	case 0xB8:
		cp(m_registers.B);
		break;

		// CP C
	case 0xB9:
		cp(m_registers.C);
		break;

		// CP D
	case 0xBA:
		cp(m_registers.D);
		break;

		// CP E
	case 0xBB:
		cp(m_registers.E);
		break;

		// CP H
	case 0xBC:
		cp(m_registers.H);
		break;

		// CP L
	case 0xBD:
		cp(m_registers.L);
		break;

		// CP (HL)
	case 0xBE:
	{
		u8 value = readMemory_u8(m_registers.HL);
		cp(value);
		break;
	}

		// CP A
	case 0xBF:
		cp(m_registers.A);
		break;

		// RET NZ
	case 0xC0:
		ret(!m_registers.F.Z);
		break;

		// POP BC
	case 0xC1:
		m_registers.BC = pop();
		break;

		// JP NZ,nn
	case 0xC2:
		jp(!m_registers.F.Z);
		break;

		// JP nn
	case 0xC3:
		m_registers.PC = readMemory_u16(m_registers.PC);
		doCycle();
		break;

		// CALL NZ,nn
	case 0xC4:
		call(!m_registers.F.Z);
		break;

		// PUSH BC
	case 0xC5:
		push(m_registers.BC);
		break;

		// ADD A,n
	case 0xC6:
	{
		u8 value = fetch_u8();
		add_A(value);
		break;
	}

		// RST 0
	case 0xC7:
		rst(0x00);
		break;

		// RET Z
	case 0xC8:
		ret(m_registers.F.Z);
		break;

		// RET
	case 0xC9:
		m_registers.PC = pop();
		doCycle();
		break;

		// JP Z,nn
	case 0xCA:
		jp(m_registers.F.Z);
		break;

	case 0xCB:
		opcode = fetch_u8();
		switch (opcode)
		{
			// RLC B
		case 0x00:
			rlc(m_registers.B);
			break;

			// RLC C
		case 0x01:
			rlc(m_registers.C);
			break;

			// RLC D
		case 0x02:
			rlc(m_registers.D);
			break;

			// RLC E
		case 0x03:
			rlc(m_registers.E);
			break;

			// RLC H
		case 0x04:
			rlc(m_registers.H);
			break;

			// RLC L
		case 0x05:
			rlc(m_registers.L);
			break;

			// RLC (HL)
		case 0x06:
		{
			u8 value = readMemory_u8(m_registers.HL);
			rlc(value);
			writeToMemory(m_registers.HL, value);
			break;
		}

			// RLC A
		case 0x07:
			rlc(m_registers.A);
			break;

			// RRC B
		case 0x08:
			rrc(m_registers.B);
			break;

			// RRC C
		case 0x09:
			rrc(m_registers.C);
			break;

			// RRC D
		case 0x0A:
			rrc(m_registers.D);
			break;

			// RRC E
		case 0x0B:
			rrc(m_registers.E);
			break;

			// RRC H
		case 0x0C:
			rrc(m_registers.H);
			break;

			// RRC L
		case 0x0D:
			rrc(m_registers.L);
			break;

			// RRC (HL)
		case 0x0E:
		{
			u8 value = readMemory_u8(m_registers.HL);
			rrc(value);
			writeToMemory(m_registers.HL, value);
			break;
		}

			// RRC A
		case 0x0F:
			rrc(m_registers.A);
			break;

			// RL B
		case 0x10:
			rl(m_registers.B);
			break;

			// RL C
		case 0x11:
			rl(m_registers.C);
			break;

			// RL D
		case 0x12:
			rl(m_registers.D);
			break;

			// RL E
		case 0x13:
			rl(m_registers.E);
			break;

			// RL H
		case 0x14:
			rl(m_registers.H);
			break;

			// RL L
		case 0x15:
			rl(m_registers.L);
			break;

			// RL (HL)
		case 0x16:
		{
			u8 value = readMemory_u8(m_registers.HL);
			rl(value);
			writeToMemory(m_registers.HL, value);
			break;
		}

			// RL A
		case 0x17:
			rl(m_registers.A);
			break;

			// RR B
		case 0x18:
			rr(m_registers.B);
			break;

			// RR C
		case 0x19:
			rr(m_registers.C);
			break;

			// RR D
		case 0x1A:
			rr(m_registers.D);
			break;

			// RR E
		case 0x1B:
			rr(m_registers.E);
			break;

			// RR H
		case 0x1C:
			rr(m_registers.H);
			break;

			// RR L
		case 0x1D:
			rr(m_registers.L);
			break;

			// RR (HL)
		case 0x1E:
		{
			u8 value = readMemory_u8(m_registers.HL);
			rr(value);
			writeToMemory(m_registers.HL, value);
			break;
		}

			// RR A
		case 0x1F:
			rr(m_registers.A);
			break;

			// SLA B
		case 0x20:
			sla(m_registers.B);
			break;

			// SLA C
		case 0x21:
			sla(m_registers.C);
			break;

			// SLA D
		case 0x22:
			sla(m_registers.D);
			break;

			// SLA E
		case 0x23:
			sla(m_registers.E);
			break;

			// SLA H
		case 0x24:
			sla(m_registers.H);
			break;

			// SLA L
		case 0x25:
			sla(m_registers.L);
			break;

			// SLA (HL)
		case 0x26:
		{
			u8 value = readMemory_u8(m_registers.HL);
			sla(value);
			writeToMemory(m_registers.HL, value);
			break;
		}

			// SLA A
		case 0x27:
			sla(m_registers.A);
			break;

			// SRA B
		case 0x28:
			sra(m_registers.B);
			break;

			// SRA C
		case 0x29:
			sra(m_registers.C);
			break;

			// SRA D
		case 0x2A:
			sra(m_registers.D);
			break;

			// SRA E
		case 0x2B:
			sra(m_registers.E);
			break;

			// SRA H
		case 0x2C:
			sra(m_registers.H);
			break;

			// SRA L
		case 0x2D:
			sra(m_registers.L);
			break;

			// SRA (HL)
		case 0x2E:
		{
			u8 value = readMemory_u8(m_registers.HL);
			sra(value);
			writeToMemory(m_registers.HL, value);
			break;
		}

			// SRA A
		case 0x2F:
			sra(m_registers.A);
			break;

			// SWAP B
		case 0x30:
			swap(m_registers.B);
			break;

			// SWAP C
		case 0x31:
			swap(m_registers.C);
			break;

			// SWAP D
		case 0x32:
			swap(m_registers.D);
			break;

			// SWAP E
		case 0x33:
			swap(m_registers.E);
			break;

			// SWAP H
		case 0x34:
			swap(m_registers.H);
			break;

			// SWAP L
		case 0x35:
			swap(m_registers.L);
			break;

			// SWAP (HL)
		case 0x36:
		{
			u8 value = readMemory_u8(m_registers.HL);
			swap(value);
			writeToMemory(m_registers.HL, value);
			break;
		}

			// SWAP A
		case 0x37:
			swap(m_registers.A);
			break;

			// SRL B
		case 0x38:
			srl(m_registers.B);
			break;

			// SRL C
		case 0x39:
			srl(m_registers.C);
			break;

			// SRL D
		case 0x3A:
			srl(m_registers.D);
			break;

			// SRL E
		case 0x3B:
			srl(m_registers.E);
			break;

			// SRL H
		case 0x3C:
			srl(m_registers.H);
			break;

			// SRL L
		case 0x3D:
			srl(m_registers.L);
			break;

			// SRL (HL)
		case 0x3E:
		{
			u8 value = readMemory_u8(m_registers.HL);
			srl(value);
			writeToMemory(m_registers.HL, value);
			break;
		}

			// SRL A
		case 0x3F:
			srl(m_registers.A);
			break;

			// BIT 0,B
		case 0x40:
			bit(m_registers.B, 0);
			break;

			// BIT 0,C
		case 0x41:
			bit(m_registers.C, 0);
			break;

			// BIT 0,D
		case 0x42:
			bit(m_registers.D, 0);
			break;

			// BIT 0,E
		case 0x43:
			bit(m_registers.E, 0);
			break;

			// BIT 0,H
		case 0x44:
			bit(m_registers.H, 0);
			break;

			// BIT 0,L
		case 0x45:
			bit(m_registers.L, 0);
			break;

			// BIT 0,(HL)
		case 0x46:
		{
			u8 value = readMemory_u8(m_registers.HL);
			bit(value, 0);
			break;
		}

			// BIT 0,A
		case 0x47:
			bit(m_registers.A, 0);
			break;

			// BIT 1,B
		case 0x48:
			bit(m_registers.B, 1);
			break;

			// BIT 1,C
		case 0x49:
			bit(m_registers.C, 1);
			break;

			// BIT 1,D
		case 0x4A:
			bit(m_registers.D, 1);
			break;

			// BIT 1,E
		case 0x4B:
			bit(m_registers.E, 1);
			break;

			// BIT 1,H
		case 0x4C:
			bit(m_registers.H, 1);
			break;

			// BIT 1,L
		case 0x4D:
			bit(m_registers.L, 1);
			break;

			// BIT 1,(HL)
		case 0x4E:
		{
			u8 value = readMemory_u8(m_registers.HL);
			bit(value, 1);
			break;
		}

			// BIT 1,A
		case 0x4F:
			bit(m_registers.A, 1);
			break;

			// BIT 2,B
		case 0x50:
			bit(m_registers.B, 2);
			break;

			// BIT 2,C
		case 0x51:
			bit(m_registers.C, 2);
			break;

			// BIT 2,D
		case 0x52:
			bit(m_registers.D, 2);
			break;

			// BIT 2,E
		case 0x53:
			bit(m_registers.E, 2);
			break;

			// BIT 2,H
		case 0x54:
			bit(m_registers.H, 2);
			break;

			// BIT 2,L
		case 0x55:
			bit(m_registers.L, 2);
			break;

			// BIT 2,(HL)
		case 0x56:
		{
			u8 value = readMemory_u8(m_registers.HL);
			bit(value, 2);
			break;
		}

			// BIT 2,A
		case 0x57:
			bit(m_registers.A, 2);
			break;

			// BIT 3,B
		case 0x58:
			bit(m_registers.B, 3);
			break;

			// BIT 3,C
		case 0x59:
			bit(m_registers.C, 3);
			break;

			// BIT 3,D
		case 0x5A:
			bit(m_registers.D, 3);
			break;

			// BIT 3,E
		case 0x5B:
			bit(m_registers.E, 3);
			break;

			// BIT 3,H
		case 0x5C:
			bit(m_registers.H, 3);
			break;

			// BIT 3,L
		case 0x5D:
			bit(m_registers.L, 3);
			break;

			// BIT 3,(HL)
		case 0x5E:
		{
			u8 value = readMemory_u8(m_registers.HL);
			bit(value, 3);
			break;
		}

			// BIT 3,A
		case 0x5F:
			bit(m_registers.A, 3);
			break;

			// BIT 4,B
		case 0x60:
			bit(m_registers.B, 4);
			break;

			// BIT 4,C
		case 0x61:
			bit(m_registers.C, 4);
			break;

			// BIT 4,D
		case 0x62:
			bit(m_registers.D, 4);
			break;

			// BIT 4,E
		case 0x63:
			bit(m_registers.E, 4);
			break;

			// BIT 4,H
		case 0x64:
			bit(m_registers.H, 4);
			break;

			// BIT 4,L
		case 0x65:
			bit(m_registers.L, 4);
			break;

			// BIT 4,(HL)
		case 0x66:
		{
			u8 value = readMemory_u8(m_registers.HL);
			bit(value, 4);
			break;
		}

			// BIT 4,A
		case 0x67:
			bit(m_registers.A, 4);
			break;

			// BIT 5,B
		case 0x68:
			bit(m_registers.B, 5);
			break;

			// BIT 5,C
		case 0x69:
			bit(m_registers.C, 5);
			break;

			// BIT 5,D
		case 0x6A:
			bit(m_registers.D, 5);
			break;

			// BIT 5,E
		case 0x6B:
			bit(m_registers.E, 5);
			break;

			// BIT 5,H
		case 0x6C:
			bit(m_registers.H, 5);
			break;

			// BIT 5,L
		case 0x6D:
			bit(m_registers.L, 5);
			break;

			// BIT 5,(HL)
		case 0x6E:
		{
			u8 value = readMemory_u8(m_registers.HL);
			bit(value, 5);
			break;
		}

			// BIT 5,A
		case 0x6F:
			bit(m_registers.A, 5);
			break;

			// BIT 6,B
		case 0x70:
			bit(m_registers.B, 6);
			break;

			// BIT 6,C
		case 0x71:
			bit(m_registers.C, 6);
			break;

			// BIT 6,D
		case 0x72:
			bit(m_registers.D, 6);
			break;

			// BIT 6,E
		case 0x73:
			bit(m_registers.E, 6);
			break;

			// BIT 6,H
		case 0x74:
			bit(m_registers.H, 6);
			break;

			// BIT 6,L
		case 0x75:
			bit(m_registers.L, 6);
			break;

			// BIT 6,(HL)
		case 0x76:
		{
			u8 value = readMemory_u8(m_registers.HL);
			bit(value, 6);
			break;
		}

			// BIT 6,A
		case 0x77:
			bit(m_registers.A, 6);
			break;

			// BIT 7,B
		case 0x78:
			bit(m_registers.B, 7);
			break;

			// BIT 7,C
		case 0x79:
			bit(m_registers.C, 7);
			break;

			// BIT 7,D
		case 0x7A:
			bit(m_registers.D, 7);
			break;

			// BIT 7,E
		case 0x7B:
			bit(m_registers.E, 7);
			break;

			// BIT 7,H
		case 0x7C:
			bit(m_registers.H, 7);
			break;

			// BIT 7,L
		case 0x7D:
			bit(m_registers.L, 7);
			break;

			// BIT 7,(HL)
		case 0x7E:
		{
			u8 value = readMemory_u8(m_registers.HL);
			bit(value, 7);
			break;
		}

			// BIT 7,A
		case 0x7F:
			bit(m_registers.A, 7);
			break;

			// RES 0,B
		case 0x80:
			res(m_registers.B, 0);
			break;

			// RES 0,C
		case 0x81:
			res(m_registers.C, 0);
			break;

			// RES 0,D
		case 0x82:
			res(m_registers.D, 0);
			break;

			// RES 0,E
		case 0x83:
			res(m_registers.E, 0);
			break;

			// RES 0,H
		case 0x84:
			res(m_registers.H, 0);
			break;

			// RES 0,L
		case 0x85:
			res(m_registers.L, 0);
			break;

			// RES 0,(HL)
		case 0x86:
		{
			u8 value = readMemory_u8(m_registers.HL);
			res(value, 0);
			writeToMemory(m_registers.HL, value);
			break;
		}

			// RES 0,A
		case 0x87:
			res(m_registers.A, 0);
			break;

			// RES 1,B
		case 0x88:
			res(m_registers.B, 1);
			break;

			// RES 1,C
		case 0x89:
			res(m_registers.C, 1);
			break;

			// RES 1,D
		case 0x8A:
			res(m_registers.D, 1);
			break;

			// RES 1,E
		case 0x8B:
			res(m_registers.E, 1);
			break;

			// RES 1,H
		case 0x8C:
			res(m_registers.H, 1);
			break;

			// RES 1,L
		case 0x8D:
			res(m_registers.L, 1);
			break;

			// RES 1,(HL)
		case 0x8E:
		{
			u8 value = readMemory_u8(m_registers.HL);
			res(value, 1);
			writeToMemory(m_registers.HL, value);
			break;
		}

			// RES 1,A
		case 0x8F:
			res(m_registers.A, 1);
			break;

			// RES 2,B
		case 0x90:
			res(m_registers.B, 2);
			break;

			// RES 2,C
		case 0x91:
			res(m_registers.C, 2);
			break;

			// RES 2,D
		case 0x92:
			res(m_registers.D, 2);
			break;

			// RES 2,E
		case 0x93:
			res(m_registers.E, 2);
			break;

			// RES 2,H
		case 0x94:
			res(m_registers.H, 2);
			break;

			// RES 2,L
		case 0x95:
			res(m_registers.L, 2);
			break;

			// RES 2,(HL)
		case 0x96:
		{
			u8 value = readMemory_u8(m_registers.HL);
			res(value, 2);
			writeToMemory(m_registers.HL, value);
			break;
		}

			// RES 2,A
		case 0x97:
			res(m_registers.A, 2);
			break;

			// RES 3,B
		case 0x98:
			res(m_registers.B, 3);
			break;

			// RES 3,C
		case 0x99:
			res(m_registers.C, 3);
			break;

			// RES 3,D
		case 0x9A:
			res(m_registers.D, 3);
			break;

			// RES 3,E
		case 0x9B:
			res(m_registers.E, 3);
			break;

			// RES 3,H
		case 0x9C:
			res(m_registers.H, 3);
			break;

			// RES 3,L
		case 0x9D:
			res(m_registers.L, 3);
			break;

			// RES 3,(HL)
		case 0x9E:
		{
			u8 value = readMemory_u8(m_registers.HL);
			res(value, 3);
			writeToMemory(m_registers.HL, value);
			break;
		}

			// RES 3,A
		case 0x9F:
			res(m_registers.A, 3);
			break;

			// RES 4,B
		case 0xA0:
			res(m_registers.B, 4);
			break;

			// RES 4,C
		case 0xA1:
			res(m_registers.C, 4);
			break;

			// RES 4,D
		case 0xA2:
			res(m_registers.D, 4);
			break;

			// RES 4,E
		case 0xA3:
			res(m_registers.E, 4);
			break;

			// RES 4,H
		case 0xA4:
			res(m_registers.H, 4);
			break;

			// RES 4,L
		case 0xA5:
			res(m_registers.L, 4);
			break;

			// RES 4,(HL)
		case 0xA6:
		{
			u8 value = readMemory_u8(m_registers.HL);
			res(value, 4);
			writeToMemory(m_registers.HL, value);
			break;
		}

			// RES 4,A
		case 0xA7:
			res(m_registers.A, 4);
			break;

			// RES 5,B
		case 0xA8:
			res(m_registers.B, 5);
			break;

			// RES 5,C
		case 0xA9:
			res(m_registers.C, 5);
			break;

			// RES 5,D
		case 0xAA:
			res(m_registers.D, 5);
			break;

			// RES 5,E
		case 0xAB:
			res(m_registers.E, 5);
			break;

			// RES 5,H
		case 0xAC:
			res(m_registers.H, 5);
			break;

			// RES 5,L
		case 0xAD:
			res(m_registers.L, 5);
			break;

			// RES 5,(HL)
		case 0xAE:
		{
			u8 value = readMemory_u8(m_registers.HL);
			res(value, 5);
			writeToMemory(m_registers.HL, value);
			break;
		}

			// RES 5,A
		case 0xAF:
			res(m_registers.A, 5);
			break;

			// RES 6,B
		case 0xB0:
			res(m_registers.B, 6);
			break;

			// RES 6,C
		case 0xB1:
			res(m_registers.C, 6);
			break;

			// RES 6,D
		case 0xB2:
			res(m_registers.D, 6);
			break;

			// RES 6,E
		case 0xB3:
			res(m_registers.E, 6);
			break;

			// RES 6,H
		case 0xB4:
			res(m_registers.H, 6);
			break;

			// RES 6,L
		case 0xB5:
			res(m_registers.L, 6);
			break;

			// RES 6,(HL)
		case 0xB6:
		{
			u8 value = readMemory_u8(m_registers.HL);
			res(value, 6);
			writeToMemory(m_registers.HL, value);
			break;
		}

			// RES 6,A
		case 0xB7:
			res(m_registers.A, 6);
			break;

			// RES 7,B
		case 0xB8:
			res(m_registers.B, 7);
			break;

			// RES 7,C
		case 0xB9:
			res(m_registers.C, 7);
			break;

			// RES 7,D
		case 0xBA:
			res(m_registers.D, 7);
			break;

			// RES 7,E
		case 0xBB:
			res(m_registers.E, 7);
			break;

			// RES 7,H
		case 0xBC:
			res(m_registers.H, 7);
			break;

			// RES 7,L
		case 0xBD:
			res(m_registers.L, 7);
			break;

			// RES 7,(HL)
		case 0xBE:
		{
			u8 value = readMemory_u8(m_registers.HL);
			res(value, 7);
			writeToMemory(m_registers.HL, value);
			break;
		}

			// RES 7,A
		case 0xBF:
			res(m_registers.A, 7);
			break;

			// SET 0,B
		case 0xC0:
			set(m_registers.B, 0);
			break;

			// SET 0,C
		case 0xC1:
			set(m_registers.C, 0);
			break;

			// SET 0,D
		case 0xC2:
			set(m_registers.D, 0);
			break;

			// SET 0,E
		case 0xC3:
			set(m_registers.E, 0);
			break;

			// SET 0,H
		case 0xC4:
			set(m_registers.H, 0);
			break;

			// SET 0,L
		case 0xC5:
			set(m_registers.L, 0);
			break;

			// SET 0,(HL)
		case 0xC6:
		{
			u8 value = readMemory_u8(m_registers.HL);
			set(value, 0);
			writeToMemory(m_registers.HL, value);
			break;
		}

			// SET 0,A
		case 0xC7:
			set(m_registers.A, 0);
			break;

			// SET 1,B
		case 0xC8:
			set(m_registers.B, 1);
			break;

			// SET 1,C
		case 0xC9:
			set(m_registers.C, 1);
			break;

			// SET 1,D
		case 0xCA:
			set(m_registers.D, 1);
			break;

			// SET 1,E
		case 0xCB:
			set(m_registers.E, 1);
			break;

			// SET 1,H
		case 0xCC:
			set(m_registers.H, 1);
			break;

			// SET 1,L
		case 0xCD:
			set(m_registers.L, 1);
			break;

			// SET 1,(HL)
		case 0xCE:
		{
			u8 value = readMemory_u8(m_registers.HL);
			set(value, 1);
			writeToMemory(m_registers.HL, value);
			break;
		}

			// SET 1,A
		case 0xCF:
			set(m_registers.A, 1);
			break;

			// SET 2,B
		case 0xD0:
			set(m_registers.B, 2);
			break;

			// SET 2,C
		case 0xD1:
			set(m_registers.C, 2);
			break;

			// SET 2,D
		case 0xD2:
			set(m_registers.D, 2);
			break;

			// SET 2,E
		case 0xD3:
			set(m_registers.E, 2);
			break;

			// SET 2,H
		case 0xD4:
			set(m_registers.H, 2);
			break;

			// SET 2,L
		case 0xD5:
			set(m_registers.L, 2);
			break;

			// SET 2,(HL)
		case 0xD6:
		{
			u8 value = readMemory_u8(m_registers.HL);
			set(value, 2);
			writeToMemory(m_registers.HL, value);
			break;
		}

			// SET 2,A
		case 0xD7:
			set(m_registers.A, 2);
			break;

			// SET 3,B
		case 0xD8:
			set(m_registers.B, 3);
			break;

			// SET 3,C
		case 0xD9:
			set(m_registers.C, 3);
			break;

			// SET 3,D
		case 0xDA:
			set(m_registers.D, 3);
			break;

			// SET 3,E
		case 0xDB:
			set(m_registers.E, 3);
			break;

			// SET 3,H
		case 0xDC:
			set(m_registers.H, 3);
			break;

			// SET 3,L
		case 0xDD:
			set(m_registers.L, 3);
			break;

			// SET 3,(HL)
		case 0xDE:
		{
			u8 value = readMemory_u8(m_registers.HL);
			set(value, 3);
			writeToMemory(m_registers.HL, value);
			break;
		}

			// SET 3,A
		case 0xDF:
			set(m_registers.A, 3);
			break;

			// SET 4,B
		case 0xE0:
			set(m_registers.B, 4);
			break;

			// SET 4,C
		case 0xE1:
			set(m_registers.C, 4);
			break;

			// SET 4,D
		case 0xE2:
			set(m_registers.D, 4);
			break;

			// SET 4,E
		case 0xE3:
			set(m_registers.E, 4);
			break;

			// SET 4,H
		case 0xE4:
			set(m_registers.H, 4);
			break;

			// SET 4,L
		case 0xE5:
			set(m_registers.L, 4);
			break;

			// SET 4,(HL)
		case 0xE6:
		{
			u8 value = readMemory_u8(m_registers.HL);
			set(value, 4);
			writeToMemory(m_registers.HL, value);
			break;
		}

			// SET 4,A
		case 0xE7:
			set(m_registers.A, 4);
			break;

			// SET 5,B
		case 0xE8:
			set(m_registers.B, 5);
			break;

			// SET 5,C
		case 0xE9:
			set(m_registers.C, 5);
			break;

			// SET 5,D
		case 0xEA:
			set(m_registers.D, 5);
			break;

			// SET 5,E
		case 0xEB:
			set(m_registers.E, 5);
			break;

			// SET 5,H
		case 0xEC:
			set(m_registers.H, 5);
			break;

			// SET 5,L
		case 0xED:
			set(m_registers.L, 5);
			break;

			// SET 5,(HL)
		case 0xEE:
		{
			u8 value = readMemory_u8(m_registers.HL);
			set(value, 5);
			writeToMemory(m_registers.HL, value);
			break;
		}

			// SET 5,A
		case 0xEF:
			set(m_registers.A, 5);
			break;

			// SET 6,B
		case 0xF0:
			set(m_registers.B, 6);
			break;

			// SET 6,C
		case 0xF1:
			set(m_registers.C, 6);
			break;

			// SET 6,D
		case 0xF2:
			set(m_registers.D, 6);
			break;

			// SET 6,E
		case 0xF3:
			set(m_registers.E, 6);
			break;

			// SET 6,H
		case 0xF4:
			set(m_registers.H, 6);
			break;

			// SET 6,L
		case 0xF5:
			set(m_registers.L, 6);
			break;

			// SET 6,(HL)
		case 0xF6:
		{
			u8 value = readMemory_u8(m_registers.HL);
			set(value, 6);
			writeToMemory(m_registers.HL, value);
			break;
		}

			// SET 6,A
		case 0xF7:
			set(m_registers.A, 6);
			break;

			// SET 7,B
		case 0xF8:
			set(m_registers.B, 7);
			break;

			// SET 7,C
		case 0xF9:
			set(m_registers.C, 7);
			break;

			// SET 7,D
		case 0xFA:
			set(m_registers.D, 7);
			break;

			// SET 7,E
		case 0xFB:
			set(m_registers.E, 7);
			break;

			// SET 7,H
		case 0xFC:
			set(m_registers.H, 7);
			break;

			// SET 7,L
		case 0xFD:
			set(m_registers.L, 7);
			break;

			// SET 7,(HL)
		case 0xFE:
		{
			u8 value = readMemory_u8(m_registers.HL);
			set(value, 7);
			writeToMemory(m_registers.HL, value);
			break;
		}

			// SET 7,A
		case 0xFF:
			set(m_registers.A, 7);
			break;
		}
		break;

		// CALL Z,nn
	case 0xCC:
		call(m_registers.F.Z);
		break;

		// CALL nn
	case 0xCD:
		push(m_registers.PC + 2);
		m_registers.PC = readMemory_u16(m_registers.PC);
		break;

		// ADC A,n
	case 0xCE:
	{
		u8 value = fetch_u8();
		adc(value);
		break;
	}

		// RST 1
	case 0xCF:
		rst(0x08);
		break;

		// RET NC
	case 0xD0:
		ret(!m_registers.F.C);
		break;

		// POP DE
	case 0xD1:
		m_registers.DE = pop();
		break;

		// JP NC,nn
	case 0xD2:
		jp(!m_registers.F.C);
		break;

		// CALL NC,nn
	case 0xD4:
		call(!m_registers.F.C);
		break;

		// PUSH DE
	case 0xD5:
		push(m_registers.DE);
		break;

		// SUB n
	case 0xD6:
	{
		u8 value = fetch_u8();
		sub(value);
		break;
	}

		// RST 2
	case 0xD7:
		rst(0x10);
		break;

		// RET C
	case 0xD8:
		ret(m_registers.F.C);
		break;

		// RETI
	case 0xD9:
		m_registers.PC = pop();
		m_ime = true;
		doCycle();
		break;

		// JP C,nn
	case 0xDA:
		jp(m_registers.F.C);
		break;

		// CALL C,nn
	case 0xDC:
		call(m_registers.F.C);
		break;

		// SBC A,n
	case 0xDE:
	{
		u8 value = fetch_u8();
		sbc(value);
		break;
	}

		// RST 3
	case 0xDF:
		rst(0x18);
		break;

		// LD (n),A
	case 0xE0:
	{
		u8 value = fetch_u8();
		writeToMemory(0xFF00 | value, m_registers.A);
		break;
	}

		// POP HL
	case 0xE1:
		m_registers.HL = pop();
		break;

		// LD (C),A
	case 0xE2:
		writeToMemory(0xFF00 | m_registers.C, m_registers.A);
		break;

		// PUSH HL
	case 0xE5:
		push(m_registers.HL);
		break;

		// AND n
	case 0xE6:
	{
		u8 value = fetch_u8();
		and_(value);
		break;
	}

		// RST 4
	case 0xE7:
		rst(0x20);
		break;

		// ADD SP,e
	case 0xE8:
		m_registers.SP = add_SP_e();
		doCycle();
		break;

		// JP (HL)
	case 0xE9:
		m_registers.PC = m_registers.HL;
		break;

		// LD (nn),A
	case 0xEA:
	{
		u16 address = fetch_u16();
		writeToMemory(address, m_registers.A);
		break;
	}

		// XOR n
	case 0xEE:
	{
		u8 value = fetch_u8();
		xor_(value);
		break;
	}

		// RST 5
	case 0xEF:
		rst(0x28);
		break;

		// LD A,(n)
	case 0xF0:
	{
		u8 value = fetch_u8();
		m_registers.A = readMemory_u8(0xFF00 | value);
		break;
	}

		// POP AF
	case 0xF1:
		m_registers.AF = pop() & 0xFFF0;
		break;

		// LD A,(C)
	case 0xF2:
		m_registers.A = readMemory_u8(0xFF00 | m_registers.C);
		break;

		// DI
	case 0xF3:
		m_ime = false;
		break;

		// PUSH AF
	case 0xF5:
		push(m_registers.AF);
		break;

		// OR n
	case 0xF6:
	{
		u8 value = fetch_u8();
		or_(value);
		break;
	}

		// RST 6
	case 0xF7:
		rst(0x30);
		break;

		// LDHL SP,e
	case 0xF8:
		m_registers.HL = add_SP_e();
		break;

		// LD SP,HL
	case 0xF9:
		m_registers.SP = m_registers.HL;
		doCycle();
		break;

		// LD A,(nn)
	case 0xFA:
	{
		u16 address = fetch_u16();
		m_registers.A = readMemory_u8(address);
		break;
	}

		// EI
	case 0xFB:
		m_ime = true;
		break;

		// CP n
	case 0xFE:
	{
		u8 value = fetch_u8();
		cp(value);
		break;
	}

		// RST 7
	case 0xFF:
		rst(0x38);
		break;

	default:
		throwError("Unknown opcode 0x", std::hex, (u16)opcode);
	}
}

void Cpu::doCycle(u8 cycleCount)
{
	for (u8 counter = 0; counter < cycleCount; ++counter)
	{
		incrementDIV();
		incrementTIMA();

		if (m_memory.KEY1 & 0x80) // double speed mode ?
		{
			static bool displaySwitch = false;
			displaySwitch = !displaySwitch;

			if (displaySwitch)
				continue;
		}

		m_displayController.doCycle();
		m_displayController.regulateFps();
	}
}

void Cpu::incrementDIV()
{
	constexpr u8 PERIOD = 128;

	static u8 cycleCounter = 0;
	++cycleCounter;

	if (cycleCounter == PERIOD)
	{
		cycleCounter = 0;
		++m_memory.DIV;
	}
}

void Cpu::incrementTIMA()
{
	if (m_memory.TAC & 0x04)
	{
		static u16 cycleCounter = 0;
		++cycleCounter;

		u16 period = [this]() -> u16
		{
			switch (m_memory.TAC & 0x03)
			{
			case 0: return 256;
			case 1: return 4;
			case 2: return 16;
			case 3: return 64;
			default: return 0;
			}
		}();

		if (cycleCounter == period)
		{
			cycleCounter = 0;

			if (m_memory.TIMA == 0xFF) // overflow ?
			{
				m_memory.TIMA = m_memory.TMA;
				requestInterrupt(TIMER_INTERRUPT_FLAG);
			}
			else
				++m_memory.TIMA;
		}
	}
}

void Cpu::handleInterrupts()
{
	if (m_memory.IF & m_memory.IE & VBLANK_INTERRUPT_FLAG)
		performInterrupt(VBLANK_INTERRUPT_FLAG, Memory::VBLANK_INTERRUPT_ADDRESS);
	else if (m_memory.IF & m_memory.IE & LCDSTAT_INTERRUPT_FLAG)
		performInterrupt(LCDSTAT_INTERRUPT_FLAG, Memory::LCDSTAT_INTERRUPT_ADDRESS);
	else if (m_memory.IF & m_memory.IE & TIMER_INTERRUPT_FLAG)
		performInterrupt(TIMER_INTERRUPT_FLAG, Memory::TIMER_INTERRUPT_ADDRESS);
	else if (m_memory.IF & m_memory.IE & SERIALTRANSFER_INTERRUPT_FLAG)
		performInterrupt(SERIALTRANSFER_INTERRUPT_FLAG, Memory::SERIALTRANSFER_INTERRUPT_ADDRESS);
	else if (m_memory.IF & m_memory.IE & JOYPAD_INTERRUPT_FLAG)
		performInterrupt(JOYPAD_INTERRUPT_FLAG, Memory::JOYPAD_INTERRUPT_ADDRESS);
}

void Cpu::performInterrupt(InterruptFlag flag, Memory::InterruptAddress address)
{
	m_haltMode = false;

	if (m_ime)
	{
		// perform the interrupt
		m_ime = false;
		rst(address);

		// reset the corresponding flag
		m_memory.IF ^= flag;
	}
}

void Cpu::requestInterrupt(InterruptFlag flag)
{
	m_memory.IF |= flag;
}

bool Cpu::isCgbMode()
{
	u8 cgbSupportCode = m_memory.read(0x143);
	return (cgbSupportCode == 0x80) || (cgbSupportCode == 0xC0);
}

u8 Cpu::readMemory_u8(u16 address)
{
	u8 value = [&]
	{
		switch (address)
		{
		case Memory::BCPD_ADDRESS: return m_displayController.readBgPaletteColor();
		case Memory::OCPD_ADDRESS: return m_displayController.readObjPaletteColor();
		default: return m_memory.read(address);
		}
	}();

	doCycle();
	return value;
}

u16 Cpu::readMemory_u16(u16 address)
{
	u8 lowByte = readMemory_u8(address);
	u8 highByte = readMemory_u8(address + 1);
	return (highByte << 8) | lowByte;
}

void Cpu::writeToMemory(u16 address, u8 value)
{
	switch (address)
	{
	case Memory::SC_ADDRESS:
		if ((value & 0x81) == 0x81) // check if the serial transfer is started
		{
			m_memory.SB = 0xFF; // no external gameboy
			requestInterrupt(SERIALTRANSFER_INTERRUPT_FLAG);
		}
		break;

	case Memory::NR13_ADDRESS:
		m_soundController.writeToNR13(value);
		break;

	case Memory::NR14_ADDRESS:
		m_soundController.writeToNR14(value);
		break;

	case Memory::NR23_ADDRESS:
		m_soundController.writeToNR23(value);
		break;

	case Memory::NR24_ADDRESS:
		m_soundController.writeToNR24(value);
		break;

	case Memory::NR30_ADDRESS:
		if ((value & 0x80) == 0)
			m_memory.NR52 &= 0xFB;

		m_memory.NR30 = value;
		break;

	case Memory::NR33_ADDRESS:
		m_soundController.writeToNR33(value);
		break;

	case Memory::NR34_ADDRESS:
		m_soundController.writeToNR34(value);
		break;

	case Memory::NR44_ADDRESS:
		m_soundController.writeToNR44(value);
		break;

	case Memory::NR52_ADDRESS:
		m_memory.NR52 = (m_memory.NR52 & 0x0F) | (value & 0x80);
		break;

	case Memory::LCDC_ADDRESS:
		m_displayController.writeToLCDC(value);
		break;

	case Memory::STAT_ADDRESS:
		m_memory.STAT = (value & 0xF8) | (m_memory.STAT & 0x07);
		break;

	case Memory::DMA_ADDRESS:
		m_memory.DMA = value;
		m_memory.performDmaTransfer();
		break;

	case Memory::KEY1_ADDRESS:
		m_memory.KEY1 = (m_memory.KEY1 & 0x80) | (value & 0x01);
		break;

	case Memory::VBK_ADDRESS:
		m_memory.VBK = value & 0x01;
		break;

	case Memory::HDMA5_ADDRESS:
	{
		u8 oldValue = m_memory.HDMA5;
		m_memory.HDMA5 = value & 0x7F;

		if ((value & 0x80) == 0)
		{
			if (oldValue & 0x80)
				m_memory.performHdmaTransfer(m_memory.HDMA5);
			else
				m_memory.HDMA5 |= 0x80;
		}
		break;
	}
	case Memory::BCPD_ADDRESS:
		m_memory.BCPD = value;
		m_displayController.updateBgPaletteColor();
		break;

	case Memory::OCPD_ADDRESS:
		m_memory.OCPD = value;
		m_displayController.updateObjPaletteColor();
		break;

	default:
		m_memory.write(address, value);
	}

	doCycle();
}

u8 Cpu::fetch_u8()
{
	u8 value = readMemory_u8(m_registers.PC);
	++m_registers.PC;
	return value;
}

u16 Cpu::fetch_u16()
{
	u16 value = readMemory_u16(m_registers.PC);
	m_registers.PC += 2;
	return value;
}

void Cpu::push(u16 value)
{
	// delay
	doCycle();

	// high byte
	--m_registers.SP;
	writeToMemory(m_registers.SP, value >> 8);

	// low byte
	--m_registers.SP;
	writeToMemory(m_registers.SP, value & 0x00FF);
}

u16 Cpu::pop()
{
	u8 lowByte = readMemory_u8(m_registers.SP);
	++m_registers.SP;

	u8 highByte = readMemory_u8(m_registers.SP);
	++m_registers.SP;

	return (highByte << 8) | lowByte;
}

void Cpu::add_A(u8 value)
{
	u16 result = m_registers.A + value;
	m_registers.F.C = (result > 0xFF);
	m_registers.F.H = ((m_registers.A & 0x0F) + (value & 0x0F) > 0x0F);
	m_registers.A = (u8)result;
	m_registers.F.Z = (m_registers.A == 0);
	m_registers.F.N = 0;
}

void Cpu::add_HL(u16 value)
{
	u32 result = m_registers.HL + value;
	m_registers.F.C = (result > 0xFFFF);
	m_registers.F.H = ((m_registers.HL & 0x0FFF) + (value & 0x0FFF) > 0x0FFF);
	m_registers.HL = (u16)result;
	m_registers.F.N = 0;
	doCycle();
}

void Cpu::adc(u8 value)
{
	u16 result = m_registers.A + value + m_registers.F.C;
	m_registers.F.H = ((m_registers.A & 0x0F) + (value & 0x0F) + m_registers.F.C > 0x0F);
	m_registers.F.C = (result > 0xFF);
	m_registers.A = (u8)result;
	m_registers.F.Z = (m_registers.A == 0);
	m_registers.F.N = 0;
}

void Cpu::sub(u8 value)
{
	cp(value);
	m_registers.A -= value;
}

void Cpu::sbc(u8 value)
{
	s16 result = m_registers.A - value - m_registers.F.C;
	m_registers.F.H = ((m_registers.A & 0x0F) < (value & 0x0F) + m_registers.F.C);
	m_registers.F.C = (result < 0);
	m_registers.A = (u8)result;
	m_registers.F.Z = (m_registers.A == 0);
	m_registers.F.N = 1;
}

void Cpu::and_(u8 value)
{
	m_registers.A &= value;
	m_registers.F.Z = (m_registers.A == 0);
	m_registers.F.N = 0;
	m_registers.F.H = 1;
	m_registers.F.C = 0;
}

void Cpu::or_(u8 value)
{
	m_registers.A |= value;
	m_registers.F.Z = (m_registers.A == 0);
	m_registers.F.N = 0;
	m_registers.F.H = 0;
	m_registers.F.C = 0;
}

void Cpu::xor_(u8 value)
{
	m_registers.A ^= value;
	m_registers.F.Z = (m_registers.A == 0);
	m_registers.F.N = 0;
	m_registers.F.H = 0;
	m_registers.F.C = 0;
}

void Cpu::cp(u8 value)
{
	m_registers.F.Z = (m_registers.A == value);
	m_registers.F.C = (m_registers.A < value);
	m_registers.F.H = ((m_registers.A & 0x0F) < (value & 0x0F));
	m_registers.F.N = 1;
}

void Cpu::inc(u8& value)
{
	m_registers.F.H = ((value & 0x0F) == 0x0F);
	++value;
	m_registers.F.Z = (value == 0);
	m_registers.F.N = 0;
}

void Cpu::dec(u8& value)
{
	m_registers.F.H = ((value & 0x0F) == 0);
	--value;
	m_registers.F.Z = (value == 0);
	m_registers.F.N = 1;
}

void Cpu::swap(u8& value)
{
	value = (value >> 4) | (value << 4);
	m_registers.F.Z = (value == 0);
	m_registers.F.N = 0;
	m_registers.F.H = 0;
	m_registers.F.C = 0;
}

void Cpu::rlc(u8& value)
{
	m_registers.F.C = value >> 7;
	value = (value << 1) | m_registers.F.C;
	m_registers.F.Z = (value == 0);
	m_registers.F.N = 0;
	m_registers.F.H = 0;
}

void Cpu::rl(u8& value)
{
	u8 oldCarry = m_registers.F.C;
	m_registers.F.C = value >> 7;
	value = (value << 1) | oldCarry;
	m_registers.F.Z = (value == 0);
	m_registers.F.N = 0;
	m_registers.F.H = 0;
}

void Cpu::rrc(u8& value)
{
	m_registers.F.C = value & 1;
	value = (value >> 1) | (m_registers.F.C << 7);
	m_registers.F.Z = (value == 0);
	m_registers.F.N = 0;
	m_registers.F.H = 0;
}

void Cpu::rr(u8& value)
{
	u8 oldCarry = m_registers.F.C;
	m_registers.F.C = value & 1;
	value = (value >> 1) | (oldCarry << 7);
	m_registers.F.Z = (value == 0);
	m_registers.F.N = 0;
	m_registers.F.H = 0;
}

void Cpu::sla(u8& value)
{
	m_registers.F.C = value >> 7;
	value <<= 1;
	m_registers.F.Z = (value == 0);
	m_registers.F.N = 0;
	m_registers.F.H = 0;
}

void Cpu::sra(u8& value)
{
	m_registers.F.C = value & 1;
	value = (value >> 1) | (value & 0x80);
	m_registers.F.Z = (value == 0);
	m_registers.F.N = 0;
	m_registers.F.H = 0;
}

void Cpu::srl(u8& value)
{
	m_registers.F.C = value & 1;
	value >>= 1;
	m_registers.F.Z = (value == 0);
	m_registers.F.N = 0;
	m_registers.F.H = 0;
}

void Cpu::bit(u8 value, u8 n)
{
	m_registers.F.Z = (~value >> n) & 1;
	m_registers.F.N = 0;
	m_registers.F.H = 1;
}

void Cpu::set(u8& value, u8 n)
{
	value |= 1 << n;
}

void Cpu::res(u8& value, u8 n)
{
	value &= ~(1 << n);
}

void Cpu::call(bool cc)
{
	if (cc)
	{
		push(m_registers.PC + 2);
		m_registers.PC = readMemory_u16(m_registers.PC);
	}
	else
	{
		m_registers.PC += 2;
		doCycle(2);
	}
}

void Cpu::rst(u16 address)
{
	push(m_registers.PC);
	m_registers.PC = address;
}

void Cpu::daa()
{
	u16 A = m_registers.A;

	if (m_registers.F.N)
	{
		if (m_registers.F.C)
			A -= 0x60;

		if (m_registers.F.H)
			A -= 0x06;
	}
	else
	{
		if (m_registers.F.H || (A & 0x0F) > 0x09)
			A += 0x06;

		if (m_registers.F.C || A > 0x9F)
			A += 0x60;

		if (A > 0xFF)
			m_registers.F.C = 1;
	}

	m_registers.A = (u8)A;
	m_registers.F.Z = (m_registers.A == 0);
	m_registers.F.H = 0;
}

u16 Cpu::add_SP_e()
{
	u8 value = fetch_u8();
	m_registers.F.H = ((m_registers.SP & 0x000F) + (value & 0x0F) > 0x0F);
	m_registers.F.C = ((m_registers.SP & 0x00FF) + value > 0xFF);
	m_registers.F.Z = 0;
	m_registers.F.N = 0;
	doCycle();

	s32 result = m_registers.SP + (s8)value;
	return (u16)result;
}

void Cpu::jr(bool cc)
{
	if (cc)
	{
		s8 value = fetch_u8();
		m_registers.PC += value;
		doCycle();
	}
	else
	{
		++m_registers.PC;
		doCycle();
	}
}

void Cpu::ret(bool cc)
{
	if (cc)
	{
		m_registers.PC = pop();
		doCycle(2);
	}
	else
		doCycle();
}

void Cpu::jp(bool cc)
{
	if (cc)
	{
		m_registers.PC = readMemory_u16(m_registers.PC);
		doCycle();
	}
	else
	{
		m_registers.PC += 2;
		doCycle(2);
	}
}
