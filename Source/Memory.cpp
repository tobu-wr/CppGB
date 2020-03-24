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

#include <iostream>
#include <fstream>
#include <algorithm>
#include <iterator>

#include "error.h"
#include "Memory.h"

enum : u16
{
	ROM_START_ADDRESS = 0x0000,
	ROM_END_ADDRESS = 0x7FFF,
	DISPLAYRAM_START_ADDRESS = 0x8000,
	DISPLAYRAM_END_ADDRESS = 0x9FFF,
	EXTERNALRAM_START_ADDRESS = 0xA000,
	EXTERNALRAM_END_ADDRESS = 0xBFFF,
	WORKRAM_START_ADDRESS = 0xC000,
	WORKRAM_END_ADDRESS = 0xDFFF,
	ECHORAM_START_ADDRESS = 0xE000,
	ECHORAM_END_ADDRESS = 0xFDFF,
	OAM_START_ADDRESS = 0xFE00,
	OAM_END_ADDRESS = 0xFE9F,
	WAVEFORMRAM_START_ADDRESS = 0xFF30,
	WAVEFORMRAM_END_ADDRESS = 0xFF3F,
	STACKRAM_START_ADDRESS = 0xFF80,
	STACKRAM_END_ADDRESS = 0xFFFE
};

enum : u16
{
	ROM_BANK_SIZE = 0x4000,
	DISPLAYRAM_BANK_SIZE = 0x2000,
	EXTERNALRAM_BANK_SIZE = 0x2000,
	WORKRAM_BANK_SIZE = 0x1000
};

std::string removeExtension(const std::string& filename)
{
	size_t lastDotPosition = filename.find_last_of(".");
	return filename.substr(0, lastDotPosition);
}

Memory::Memory(const std::string& romFilename)
{
	m_saveFilename = removeExtension(romFilename) + ".save";

	initRom(romFilename);
	getCartridgeType();
	initExternalRam();
}

Memory::~Memory()
{
	if (m_saveEnabled)
		save();
}

void Memory::performDmaTransfer()
{
	u16 dmaAddress = DMA * 0x100;

	for (u8& destination : m_oam)
	{
		destination = read(dmaAddress);
		++dmaAddress;
	}
}

void Memory::performHdmaTransfer(u8 n)
{
	u16 transferSize = 16 * (n + 1);

	u16 sourceAddress = (HDMA1 << 8) | (HDMA2 & 0xF0);
	u16 destinationAddress = ((HDMA3 & 0x1F) << 8) | (HDMA4 & 0xF0);

	for (u16 byteCounter = 0; byteCounter < transferSize; ++byteCounter)
	{
		m_displayRam[destinationAddress + VBK * DISPLAYRAM_BANK_SIZE] = read(sourceAddress);
		++sourceAddress;
		++destinationAddress;
	}

	HDMA1 = sourceAddress >> 8;
	HDMA2 = sourceAddress & 0x00FF;
	HDMA3 = destinationAddress >> 8;
	HDMA4 = destinationAddress & 0x00FF;
	HDMA5 -= (n + 1);
}

u8 Memory::read(u16 address)
{
	if ((ROM_START_ADDRESS <= address) && (address <= ROM_END_ADDRESS))
	{
		if (address < ROM_BANK_SIZE)
			return m_rom[address];
		else
			return m_rom[address + (m_romBankNumber - 1) * ROM_BANK_SIZE];
	}

	else if ((DISPLAYRAM_START_ADDRESS <= address) && (address <= DISPLAYRAM_END_ADDRESS))
		return readDisplayRam(address, VBK);

	else if ((EXTERNALRAM_START_ADDRESS <= address) && (address <= EXTERNALRAM_END_ADDRESS))
	{
		u16 position = address - EXTERNALRAM_START_ADDRESS + m_externalRamBankNumber * EXTERNALRAM_BANK_SIZE;

		if (position < m_externalRam.size())
			return m_externalRam[position];
		else
			return 0xFF;
	}

	else if ((WORKRAM_START_ADDRESS <= address) && (address <= WORKRAM_END_ADDRESS))
	{
		u16 position = address - WORKRAM_START_ADDRESS;

		if (position < WORKRAM_BANK_SIZE)
			return m_workRam[position];
		else
		{
			u8 bankNumber = (SVBK == 0) ? 1 : SVBK;
			return m_workRam[position + (bankNumber - 1) * WORKRAM_BANK_SIZE];
		}
	}

	else if ((ECHORAM_START_ADDRESS <= address) && (address <= ECHORAM_END_ADDRESS))
		return read(address - 0x2000);

	else if ((OAM_START_ADDRESS <= address) && (address <= OAM_END_ADDRESS))
		return m_oam[address - OAM_START_ADDRESS];

	else if ((WAVEFORMRAM_START_ADDRESS <= address) && (address <= WAVEFORMRAM_END_ADDRESS))
		return m_waveformRam[address - WAVEFORMRAM_START_ADDRESS];

	else if ((STACKRAM_START_ADDRESS <= address) && (address <= STACKRAM_END_ADDRESS))
		return m_stackRam[address - STACKRAM_START_ADDRESS];

	else
	{
		switch (address)
		{
		case P1_ADDRESS: return P1;
		case SB_ADDRESS: return SB;
		case SC_ADDRESS: return SC;
		case DIV_ADDRESS: return DIV;
		case TIMA_ADDRESS: return TIMA;
		case TMA_ADDRESS: return TMA;
		case TAC_ADDRESS: return TAC;
		case IF_ADDRESS: return IF;
		case NR10_ADDRESS: return NR10;
		case NR11_ADDRESS: return NR11;
		case NR12_ADDRESS: return NR12;
		case NR13_ADDRESS: return NR13;
		case NR14_ADDRESS: return NR14;
		case NR21_ADDRESS: return NR21;
		case NR22_ADDRESS: return NR22;
		case NR23_ADDRESS: return NR23;
		case NR24_ADDRESS: return NR24;
		case NR30_ADDRESS: return NR30;
		case NR31_ADDRESS: return NR31;
		case NR32_ADDRESS: return NR32;
		case NR33_ADDRESS: return NR33;
		case NR34_ADDRESS: return NR34;
		case NR41_ADDRESS: return NR41;
		case NR42_ADDRESS: return NR42;
		case NR43_ADDRESS: return NR43;
		case NR44_ADDRESS: return NR44;
		case NR50_ADDRESS: return NR50;
		case NR51_ADDRESS: return NR51;
		case NR52_ADDRESS: return NR52;
		case LCDC_ADDRESS: return LCDC;
		case STAT_ADDRESS: return STAT;
		case SCY_ADDRESS: return SCY;
		case SCX_ADDRESS: return SCX;
		case LY_ADDRESS: return LY;
		case LYC_ADDRESS: return LYC;
		case DMA_ADDRESS: return DMA;
		case BGP_ADDRESS: return BGP;
		case OBP0_ADDRESS: return OBP0;
		case OBP1_ADDRESS: return OBP1;
		case WY_ADDRESS: return WY;
		case WX_ADDRESS: return WX;
		case KEY1_ADDRESS: return KEY1;
		case VBK_ADDRESS: return VBK;
		case HDMA1_ADDRESS: return HDMA1;
		case HDMA2_ADDRESS: return HDMA2;
		case HDMA3_ADDRESS: return HDMA3;
		case HDMA4_ADDRESS: return HDMA4;
		case HDMA5_ADDRESS: return HDMA5;
		case BCPS_ADDRESS: return BCPS;
		case BCPD_ADDRESS: return BCPD;
		case OCPS_ADDRESS: return OCPS;
		case OCPD_ADDRESS: return OCPD;
		case SVBK_ADDRESS: return SVBK;
		case IE_ADDRESS: return IE;
		default: return 0xFF;
		}
	}
}

u8 Memory::readDisplayRam(u16 address, u8 bankNumber)
{
	return m_displayRam[address - DISPLAYRAM_START_ADDRESS + bankNumber * DISPLAYRAM_BANK_SIZE];
}

void Memory::write(u16 address, u8 value)
{
	if ((ROM_START_ADDRESS <= address) && (address <= ROM_END_ADDRESS))
		writeToRom(address, value);

	else if ((DISPLAYRAM_START_ADDRESS <= address) && (address <= DISPLAYRAM_END_ADDRESS))
		m_displayRam[address - DISPLAYRAM_START_ADDRESS + VBK * DISPLAYRAM_BANK_SIZE] = value;

	else if ((EXTERNALRAM_START_ADDRESS <= address) && (address <= EXTERNALRAM_END_ADDRESS))
	{
		u16 position = address - EXTERNALRAM_START_ADDRESS + m_externalRamBankNumber * EXTERNALRAM_BANK_SIZE;
		
		if (position < m_externalRam.size())
			m_externalRam[position] = value;
	}

	else if ((WORKRAM_START_ADDRESS <= address) && (address <= WORKRAM_END_ADDRESS))
	{
		u16 position = address - WORKRAM_START_ADDRESS;
		
		if (position < WORKRAM_BANK_SIZE)
			m_workRam[position] = value;
		else
		{
			u8 bankNumber = (SVBK == 0) ? 1 : SVBK;
			m_workRam[position + (bankNumber - 1) * WORKRAM_BANK_SIZE] = value;
		}
	}

	else if ((ECHORAM_START_ADDRESS <= address) && (address <= ECHORAM_END_ADDRESS))
		write(address - 0x2000, value);

	else if ((OAM_START_ADDRESS <= address) && (address <= OAM_END_ADDRESS))
		m_oam[address - OAM_START_ADDRESS] = value;

	else if ((WAVEFORMRAM_START_ADDRESS <= address) && (address <= WAVEFORMRAM_END_ADDRESS))
		m_waveformRam[address - WAVEFORMRAM_START_ADDRESS] = value;

	else if ((STACKRAM_START_ADDRESS <= address) && (address <= STACKRAM_END_ADDRESS))
		m_stackRam[address - STACKRAM_START_ADDRESS] = value;

	else
	{
		switch (address)
		{
		case P1_ADDRESS: P1 = value; break;
		case SB_ADDRESS: SB = value; break;
		case SC_ADDRESS: SC = value; break;
		case DIV_ADDRESS: DIV = value; break;
		case TIMA_ADDRESS: TIMA = value; break;
		case TMA_ADDRESS: TMA = value; break;
		case TAC_ADDRESS: TAC = value; break;
		case IF_ADDRESS: IF = value; break;
		case NR10_ADDRESS: NR10 = value; break;
		case NR11_ADDRESS: NR11 = value; break;
		case NR12_ADDRESS: NR12 = value; break;
		case NR13_ADDRESS: NR13 = value; break;
		case NR14_ADDRESS: NR14 = value; break;
		case NR21_ADDRESS: NR21 = value; break;
		case NR22_ADDRESS: NR22 = value; break;
		case NR23_ADDRESS: NR23 = value; break;
		case NR24_ADDRESS: NR24 = value; break;
		case NR30_ADDRESS: NR30 = value; break;
		case NR31_ADDRESS: NR31 = value; break;
		case NR32_ADDRESS: NR32 = value; break;
		case NR33_ADDRESS: NR33 = value; break;
		case NR34_ADDRESS: NR34 = value; break;
		case NR41_ADDRESS: NR41 = value; break;
		case NR42_ADDRESS: NR42 = value; break;
		case NR43_ADDRESS: NR43 = value; break;
		case NR44_ADDRESS: NR44 = value; break;
		case NR50_ADDRESS: NR50 = value; break;
		case NR51_ADDRESS: NR51 = value; break;
		case NR52_ADDRESS: NR52 = value; break;
		case LCDC_ADDRESS: LCDC = value; break;
		case STAT_ADDRESS: STAT = value; break;
		case SCY_ADDRESS: SCY = value; break;
		case SCX_ADDRESS: SCX = value; break;
		case LY_ADDRESS: LY = value; break;
		case LYC_ADDRESS: LYC = value; break;
		case DMA_ADDRESS: DMA = value; break;
		case BGP_ADDRESS: BGP = value; break;
		case OBP0_ADDRESS: OBP0 = value; break;
		case OBP1_ADDRESS: OBP1 = value; break;
		case WY_ADDRESS: WY = value; break;
		case WX_ADDRESS: WX = value; break;
		case KEY1_ADDRESS: KEY1 = value; break;
		case VBK_ADDRESS: VBK = value; break;
		case HDMA1_ADDRESS: HDMA1 = value; break;
		case HDMA2_ADDRESS: HDMA2 = value; break;
		case HDMA3_ADDRESS: HDMA3 = value; break;
		case HDMA4_ADDRESS: HDMA4 = value; break;
		case HDMA5_ADDRESS: HDMA5 = value; break;
		case BCPS_ADDRESS: BCPS = value; break;
		case BCPD_ADDRESS: BCPD = value; break;
		case OCPS_ADDRESS: OCPS = value; break;
		case OCPD_ADDRESS: OCPD = value; break;
		case SVBK_ADDRESS: SVBK = value; break;
		case IE_ADDRESS: IE = value; break;
		}
	}
}

void Memory::initRom(const std::string& filename)
{
	std::ifstream file(filename, std::ios_base::binary);

	if (!file)
		throwError("Cannot open file ", filename);

	std::istreambuf_iterator<char> begin(file);
	std::istreambuf_iterator<char> end;
	auto to = std::back_inserter(m_rom);
	std::copy(begin, end, to);
}

void Memory::initExternalRam()
{
	// set size
	u8 size = m_rom[0x149];

	switch (size)
	{
	case 0:
		if (m_mbc == MBC2)
			m_externalRam.resize(0x200);
		break;

	case 2:
		m_externalRam.resize(0x2000);
		break;

	case 3:
		m_externalRam.resize(0x8000);
		break;

	case 4:
		m_externalRam.resize(0x20000);
		break;

	default:
		throwError("Unknown external ram size (0x", std::hex, (u16)size, ")");
	}

	// load save
	std::ifstream file(m_saveFilename, std::ios_base::binary);

	if (!file)
		return;

	std::istreambuf_iterator<char> begin(file);
	std::istreambuf_iterator<char> end;
	auto to = m_externalRam.begin();
	std::copy(begin, end, to);
}

void Memory::getCartridgeType()
{
	u8 cartridgeType = m_rom[0x147];

	switch (cartridgeType)
	{
	case 0x00:
		std::cout << "Cartridge type: ROM only" << std::endl;
		break;

	case 0x01:
		std::cout << "Cartridge type: MBC1" << std::endl;
		m_mbc = MBC1;
		break;

	case 0x02:
		std::cout << "Cartridge type: MBC1 + RAM" << std::endl;
		m_mbc = MBC1;
		break;

	case 0x03:
		std::cout << "Cartridge type: MBC1 + RAM + battery" << std::endl;
		m_mbc = MBC1;
		m_saveEnabled = true;
		break;

	case 0x05:
		std::cout << "Cartridge type: MBC2" << std::endl;
		m_mbc = MBC2;
		break;

	case 0x06:
		std::cout << "Cartridge type: MBC2 + battery" << std::endl;
		m_mbc = MBC2;
		m_saveEnabled = true;
		break;

	case 0x08:
		std::cout << "Cartridge type: ROM + RAM" << std::endl;
		break;

	case 0x09:
		std::cout << "Cartridge type: ROM + RAM + battery" << std::endl;
		m_saveEnabled = true;
		break;

	case 0x0F:
		std::cout << "Cartridge type: MBC3 + RTC + battery" << std::endl;
		m_mbc = MBC3;
		break;

	case 0x10:
		std::cout << "Cartridge type: MBC3 + RTC + RAM + battery" << std::endl;
		m_mbc = MBC3;
		m_saveEnabled = true;
		break;

	case 0x11:
		std::cout << "Cartridge type: MBC3" << std::endl;
		m_mbc = MBC3;
		break;

	case 0x12:
		std::cout << "Cartridge type: MBC3 + RAM" << std::endl;
		m_mbc = MBC3;
		break;

	case 0x13:
		std::cout << "Cartridge type: MBC3 + RAM + battery" << std::endl;
		m_mbc = MBC3;
		m_saveEnabled = true;
		break;

	case 0x19:
		std::cout << "Cartridge type: MBC5" << std::endl;
		m_mbc = MBC5;
		break;

	case 0x1A:
		std::cout << "Cartridge type: MBC5 + RAM" << std::endl;
		m_mbc = MBC5;
		break;

	case 0x1B:
		std::cout << "Cartridge type: MBC5 + RAM + battery" << std::endl;
		m_mbc = MBC5;
		m_saveEnabled = true;
		break; 
	
	case 0x1C:
		std::cout << "Cartridge type: MBC5 + rumble" << std::endl;
		m_mbc = MBC5;
		break;

	case 0x1D:
		std::cout << "Cartridge type: MBC5 + rumble + RAM" << std::endl;
		m_mbc = MBC5;
		break;

	case 0x1E:
		std::cout << "Cartridge type: MBC5 + rumble + RAM + battery" << std::endl;
		m_mbc = MBC5;
		m_saveEnabled = true;
		break;

	default:
		throwError("Unknown cartridge type (0x", std::hex, (u16)cartridgeType, ")");
	}
}

void Memory::save()
{
	std::ofstream file(m_saveFilename, std::ios_base::binary);
	auto begin = m_externalRam.begin();
	auto end = m_externalRam.end();
	std::ostream_iterator<char> to(file);
	std::copy(begin, end, to);
}

void Memory::writeToRom(u16 address, u8 value)
{
	switch (m_mbc)
	{
	case MBC1:
		writeToRom_mbc1(address, value);
		break;

	case MBC2:
		writeToRom_mbc2(address, value);
		break;

	case MBC3:
		writeToRom_mbc3(address, value);
		break;

	case MBC5:
		writeToRom_mbc5(address, value);
		break;
	}
}

void Memory::writeToRom_mbc1(u16 address, u8 value)
{
	if (address < 0x2000)
	{
		// not implemented
	}
	else if (address < 0x4000)
	{
		if (value == 0)
			m_romBankNumber = 1;
		else
			m_romBankNumber = value;
	}
	else if (address < 0x6000)
		m_externalRamBankNumber = value;
	else
	{
		// not implemented
	}
}

void Memory::writeToRom_mbc2(u16 address, u8 value)
{
	if (address < 0x1000)
	{
		// not implemented
	}
	else if ((0x2100 <= address) && (address < 0x2200))
		m_romBankNumber = value;
}

void Memory::writeToRom_mbc3(u16 address, u8 value)
{
	if (address < 0x2000)
	{
		// not implemented
	}
	else if (address < 0x4000)
		m_romBankNumber = value;
	else if (address < 0x6000)
		m_externalRamBankNumber = value;
	else
	{
		// not implemented
	}
}

void Memory::writeToRom_mbc5(u16 address, u8 value)
{
	if (address < 0x2000)
	{
		// not implemented
	}
	else if (address < 0x3000)
		m_romBankNumber = value;
	else if (address < 0x4000)
	{
 		// not implemented
	}
	else if (address < 0x6000)
		m_externalRamBankNumber = value;
}
