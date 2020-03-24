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

#include "Memory.h"
#include "Cpu.h"
#include "error.h"

int main(int argumentCount, char* arguments[])
{
	if (argumentCount < 2)
		throwError("Usage: CppGB.exe <rom>");
	
	Memory memory(arguments[1]);
	Cpu cpu(memory);
	cpu.run();

	return 0;
}
