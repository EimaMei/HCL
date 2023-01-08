/*
* Copyright (C) 2022-2023 EimaMei/Sacode
*
* This software is provided 'as-is', without any express or implied
* warranty.  In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not
*    claim that you wrote the original software. If you use this software
*    in a product, an acknowledgment in the product documentation would be
*    appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
*    misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*
*
*/

#pragma once

#include <interpreter.hpp>

namespace HSM { // HOI4 Scripting+ Mode
	// Interpreter runtime information.
	extern std::string line;
	extern int equalBrackets;

	// Interpretes a single line.
	std::string interpreteLine(std::string line);

	// Checks for any conditions.
	int checkConditions(std::string& buffer);
	// Checks for any functions.
	int checkFunctions(std::string& buffer);

	// Resets the interpreter's runtime information.
	void resetRuntimeInfo();
}