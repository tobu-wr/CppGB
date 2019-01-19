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

#include <string>
#include <array>
#include <vector>

#include "types.h"

class Memory
{
public:
	enum InterruptAddress : u16
	{
		VBLANK_INTERRUPT_ADDRESS = 0x40,
		LCDSTAT_INTERRUPT_ADDRESS = 0x48,
		TIMER_INTERRUPT_ADDRESS = 0x50,
		SERIALTRANSFER_INTERRUPT_ADDRESS = 0x58,
		JOYPAD_INTERRUPT_ADDRESS = 0x60,
	};

	enum : u16
	{
		P1_ADDRESS = 0xFF00,
		SB_ADDRESS = 0xFF01,
		SC_ADDRESS = 0xFF02,
		DIV_ADDRESS = 0xFF04,
		TIMA_ADDRESS = 0xFF05,
		TMA_ADDRESS = 0xFF06,
		TAC_ADDRESS = 0xFF07,
		IF_ADDRESS = 0xFF0F,
		NR10_ADDRESS = 0xFF10,
		NR11_ADDRESS = 0xFF11,
		NR12_ADDRESS = 0xFF12,
		NR13_ADDRESS = 0xFF13,
		NR14_ADDRESS = 0xFF14,
		NR21_ADDRESS = 0xFF16,
		NR22_ADDRESS = 0xFF17,
		NR23_ADDRESS = 0xFF18,
		NR24_ADDRESS = 0xFF19,
		NR30_ADDRESS = 0xFF1A,
		NR31_ADDRESS = 0xFF1B,
		NR32_ADDRESS = 0xFF1C,
		NR33_ADDRESS = 0xFF1D,
		NR34_ADDRESS = 0xFF1E,
		NR41_ADDRESS = 0xFF20,
		NR42_ADDRESS = 0xFF21,
		NR43_ADDRESS = 0xFF22,
		NR44_ADDRESS = 0xFF23,
		NR50_ADDRESS = 0xFF24,
		NR51_ADDRESS = 0xFF25,
		NR52_ADDRESS = 0xFF26,
		LCDC_ADDRESS = 0xFF40,
		STAT_ADDRESS = 0xFF41,
		SCY_ADDRESS = 0xFF42,
		SCX_ADDRESS = 0xFF43,
		LY_ADDRESS = 0xFF44,
		LYC_ADDRESS = 0xFF45,
		DMA_ADDRESS = 0xFF46,
		BGP_ADDRESS = 0xFF47,
		OBP0_ADDRESS = 0xFF48,
		OBP1_ADDRESS = 0xFF49,
		WY_ADDRESS = 0xFF4A,
		WX_ADDRESS = 0xFF4B,
		KEY1_ADDRESS = 0xFF4D,
		VBK_ADDRESS = 0xFF4F,
		HDMA1_ADDRESS = 0xFF51,
		HDMA2_ADDRESS = 0xFF52,
		HDMA3_ADDRESS = 0xFF53,
		HDMA4_ADDRESS = 0xFF54,
		HDMA5_ADDRESS = 0xFF55,
		BCPS_ADDRESS = 0xFF68,
		BCPD_ADDRESS = 0xFF69,
		OCPS_ADDRESS = 0xFF6A,
		OCPD_ADDRESS = 0xFF6B,
		SVBK_ADDRESS = 0xFF70,
		IE_ADDRESS = 0xFFFF
	};

	Memory(const std::string& romFilename);
	~Memory();
	
	void performDmaTransfer();
	void performHdmaTransfer(u8 n);

	u8 read(u16 address);
	u8 readDisplayRam(u16 address, u8 bankNumber);
	void write(u16 address, u8 value);

	static constexpr u16 OAM_ADDRESS = 0xFE00;

	u8 P1{};
	u8 SB{}, SC{};
	u8 DIV{}, TIMA{}, TMA{}, TAC{};
	u8 IF{}, IE{};
	u8 NR10{}, NR11{}, NR12{}, NR13{}, NR14{};
	u8 NR21{}, NR22{}, NR23{}, NR24{};
	u8 NR30{}, NR31{}, NR32{}, NR33{}, NR34{};
	u8 NR41{}, NR42{}, NR43{}, NR44{};
	u8 NR50{}, NR51{}, NR52{};
	u8 LCDC{0x91};
	u8 STAT{};
	u8 SCY{}, SCX{};
	u8 LY{}, LYC{};
	u8 DMA{};
	u8 HDMA1{}, HDMA2{}, HDMA3{}, HDMA4{}, HDMA5{0x80};
	u8 BGP{};
	u8 OBP0{}, OBP1{};
	u8 WY{}, WX{};
	u8 KEY1{};
	u8 VBK{}, SVBK{};
	u8 BCPS{}, BCPD{};
	u8 OCPS{}, OCPD{};

private:
	void initRom(const std::string& filename);
	void initExternalRam();

	void getCartridgeType();
	void save();

	void writeToRom(u16 address, u8 value);
	void writeToRom_mbc1(u16 address, u8 value);
	void writeToRom_mbc2(u16 address, u8 value);
	void writeToRom_mbc3(u16 address, u8 value);
	void writeToRom_mbc5(u16 address, u8 value);

	enum Mbc
	{
		NONE, MBC1, MBC2, MBC3, MBC5
	} m_mbc = NONE;

	bool m_saveEnabled = false;
	std::string m_saveFilename;

	u8 m_romBankNumber = 1;
	u8 m_externalRamBankNumber = 0;

	std::vector<u8> m_rom;
	std::vector<u8> m_externalRam;
	std::array<u8, 0x4000> m_displayRam{};
	std::array<u8, 0x8000> m_workRam{};
	std::array<u8, 160> m_oam{};
	std::array<u8, 127> m_stackRam{};
	std::array<u8, 32> m_waveformRam{};
};
