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

#include <iostream>
#include <sstream>

template<typename Argument>
void fillStringStream(std::ostringstream& stringStream, const Argument& argument)
{
	stringStream << argument;
}

template<typename Argument, typename ... Arguments>
void fillStringStream(std::ostringstream& stringStream, const Argument& argument, const Arguments& ... arguments)
{
	stringStream << argument;
	fillStringStream(stringStream, arguments...);
}

template<typename ... Arguments>
void throwError(const Arguments& ... arguments)
{
	std::ostringstream stringStream;
	fillStringStream(stringStream, arguments...);
	std::cerr << "ERROR: " << stringStream.str() << "\n";
	exit(1);
}
