/*
* Copyright (C) 2021-2022 Eima
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

#define _CRT_SECURE_NO_WARNINGS

#include <interpreter.hpp>
#include <core.hpp>
#include <helper.hpp>
#include <functions.hpp>

#include <iostream>
#include <string.h>
#include <math.h>

std::vector<std::string> alreadyReadFiles;

std::string HPL::colorText(std::string txt, RETURN_OUTPUT type, bool light/* = false*/) {
	#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
	int clr;

	switch (type) {
		case OUTPUT_NOTHING: clr = 7; break;
		case OUTPUT_BLACK: clr = 16; break;
		case OUTPUT_RED: clr = FOREGROUND_RED; break;
		case OUTPUT_GREEN: clr = FOREGROUND_GREEN; break;
		case OUTPUT_YELLOW: clr = FOREGROUND_RED | FOREGROUND_GREEN; break;
		case OUTPUT_BLUE: clr = FOREGROUND_BLUE; break;
		case OUTPUT_PURPLE: clr = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE; break;
		case OUTPUT_CYAN: clr = FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
		case OUTPUT_GRAY: clr = 8; break;
		default: clr = type; break;
	}
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, clr);
	std::cout << txt;
	SetConsoleTextAttribute(hConsole, 7);

	#else
	std::string text;

	switch (type) {
		case OUTPUT_NOTHING:  text = "\x1B[0m";  break;
		case OUTPUT_BLACK:    text = "\x1B[30m"; break;
		case OUTPUT_RED:      text = "\x1B[31m"; break;
		case OUTPUT_GREEN:    text = "\x1B[32m"; break;
		case OUTPUT_YELLOW:   text = "\x1B[33m"; break;
		case OUTPUT_BLUE:     text = "\x1B[34m"; break;
		case OUTPUT_PURPLE:   text = "\x1B[35m"; break;
		case OUTPUT_CYAN:     text = "\x1B[36m"; break;
		case OUTPUT_GRAY:     text = "\x1B[37m"; break;
	}
	if (light) {
		std::replace(text.begin(), text.begin() + 2, '3', '9');
	}
	text += txt + "\x1B[0m";

	return text;
	#endif

	return "";
}


void HPL::interpreteFile(std::string file) {
	curFile = file;
	FILE* fp = fopen(curFile.c_str(), "r");
	bool alreadyRead = false;
	for (auto readFile : alreadyReadFiles) {
		if (readFile == file) {
			alreadyRead = true;
			break;
		}
	}

	if (fp != NULL) {
		fseek(fp, 0, SEEK_END);
		long size = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		char* buf = new char[size];
		while (fgets(buf, size + 1, fp)) {
			if (!arg.interprete)
				break;

			buf[strcspn(buf, "\n")] = 0;
			line = buf;
			lineCount++;

			if (find(line, "#read once") && alreadyRead)
				break; // Since we already read the file, just don't do it.
			else if (find(line, "#read once"))
				continue; // Ignore this line, otherwise an error will appear.

			interpreteLine(line);
		}
		if (!alreadyRead) std::cout << colorText("Finished interpreting " +std::to_string(lineCount)+ " lines from ", OUTPUT_GREEN) << "'" << colorText(curFile, OUTPUT_YELLOW) << "'" << colorText(" successfully", OUTPUT_GREEN) << std::endl;
	}
	else
		std::cout << HPL::colorText("Error: ", HPL::OUTPUT_RED) << HPL::colorText(curFile, HPL::OUTPUT_RED) << ": No such file or directory" << std::endl;

	fclose(fp);
	resetRuntimeInfo();
	alreadyReadFiles.push_back(file);
}


int HPL::interpreteLine(std::string str) {
	if (!arg.interprete)
		return FOUND_NOTHING;

	if (find(str, "//")) // Ignore comments
		str = split(str, "//", "\"\"")[0];

	line = str;

	if (arg.breakpoint) { // A breakpoint was set.
		if (curFile == arg.breakpointValues.first && lineCount == arg.breakpointValues.second) {
			std::cout << "Breakpoint reached at " << curFile << ":" << lineCount << std::endl;
			arg.interprete = false;
		}
	}

	if (checkIncludes() == FOUND_SOMETHING) return FOUND_SOMETHING;
	if (checkModes() == FOUND_SOMETHING) return FOUND_SOMETHING;
	if (checkConditions() == FOUND_SOMETHING) return FOUND_SOMETHING;
	if (checkFunctions() == FOUND_SOMETHING) return FOUND_SOMETHING;
	if (checkStruct() == FOUND_SOMETHING) return FOUND_SOMETHING;
	if (checkVariables() == FOUND_SOMETHING) return FOUND_SOMETHING;

	return FOUND_NOTHING;
}


int HPL::checkIncludes() {
	if (useRegex(line, R"(#include\s+([<|\"].*[>|\"]))")) {
		std::string match = unstringify(matches.str(1));

		// Save the old data, since when we're gonna interprete a new file, it's gonna overwrite the old data but not reinstate it when it's finished.
		// Which means we have to reinstate the old data ourselves.
		std::string oldFile = curFile;
		int oldLineCount = lineCount;
		lineCount = 0; // We reset it so that the lineCount won't be innaccurate when the lines gets interpreted correctly.


		if (find(line, "<") && find(line, ">")) // Core library '#include <libpdx.hpl>'
			match = "core/" + unstringify(match, true);
		else if (find(line, "\"")) // Some library '#include "../somelib.hpl"'
			match = getPathFromFilename(oldFile) + "/" + match;

		if (HPL::arg.debugAll || HPL::arg.debugLog) {
			std::cout << arg.curIndent << "LOG: [INCLUDE][FILE]: " << curFile << ":" << lineCount << ": #include <file>: #include \"" << match << "\"" << std::endl;
		}
		interpreteFile(match);

		curFile = oldFile;
		lineCount = oldLineCount;

	}
	return FOUND_NOTHING;
}


int HPL::checkModes() {
	bool leftBracket = useRegex(line, R"(^\s*(\})\s*$)");
	bool rightBracket = useRegex(line, R"(^.*\s*\{\s*$)");

	if (find(line, "{") && (mode >= 0x100 && mode <= 0x1000)) {
		switch (mode) {
			case MODE_CHECK_STRUCT: mode = MODE_SAVE_STRUCT; break;
			case MODE_CHECK_FUNC: mode = MODE_SAVE_FUNC; return FOUND_SOMETHING;
			case MODE_CHECK_IF_STATEMENT: mode = MODE_SCOPE_IF_STATEMENT; return FOUND_SOMETHING; // NEEDS FIXING TOO.
			case MODE_CHECK_IGNORE_ALL: mode = MODE_SCOPE_IGNORE_ALL; return FOUND_SOMETHING;
			default: return FOUND_SOMETHING;
		}
	}

	if (leftBracket)
		equalBrackets--;
	if (useRegex(line, R"(^.*\s*\{\s*$)"))
		equalBrackets++;


	if (leftBracket && mode == MODE_SAVE_STRUCT) {
		mode = MODE_DEFAULT;

		if (HPL::arg.debugAll || HPL::arg.debugLog) {
			arg.curIndent.pop_back();
			std::cout << arg.curIndent << "LOG: [DONE][STRUCT]: " << curFile << ":" << lineCount << ": struct <name> {...}: struct " << structures.front().name << " {...}" << std::endl;
		}

		return FOUND_SOMETHING;
	}
	else if (leftBracket && mode == MODE_SAVE_FUNC && equalBrackets == 0) { // Last line, do not save anymore after this.
		mode = MODE_DEFAULT;

		if (HPL::arg.debugAll || HPL::arg.debugLog) {
			arg.curIndent.pop_back();
			std::cout << arg.curIndent << "LOG: [DONE][FUNCTION]: " << curFile << ":" << lineCount << ": <type> <name>(<params>): " << printFunction(functions.back()) << std::endl;
		}

		return FOUND_SOMETHING;
	}
	else if (mode == MODE_SAVE_FUNC) {
		functions.back().code.push_back(line);

		if (HPL::arg.debugAll || HPL::arg.debugLog) {
			//std::cout << arg.curIndent << "LOG: [ADDED][LINE]: " << curFile << ":" << lineCount << ": <line>: " << line << std::endl;
		}

		return FOUND_SOMETHING;
	}
	else if (mode == MODE_SCOPE_IGNORE_ALL) {
		if (leftBracket && equalBrackets + 1 == ifStatements.back().startingLine) {
			if (ifStatements.size() > 1)
				mode = MODE_SCOPE_IF_STATEMENT;
			else
				mode = MODE_DEFAULT;
		}
		return FOUND_SOMETHING;
	}
	else if ((leftBracket || rightBracket) && mode == MODE_SCOPE_IF_STATEMENT && !ifStatements.empty()) {
		std::vector<HPL::variable> oldVars = HPL::variables;
		mode = MODE_DEFAULT;

		if (ifStatements.back().type == "invalid") {
			if (equalBrackets != 0)
				mode = MODE_SCOPE_IF_STATEMENT;
			else
				mode = MODE_DEFAULT;
			ifStatements.pop_back();

			if (rightBracket)
				return FOUND_NOTHING;
			else
				return FOUND_SOMETHING;
		}

		HPL::lineCount -= ifStatements.front().code.size();

		for (auto line : ifStatements.front().code) {
			HPL::lineCount++;
			HPL::interpreteLine(line);
		}

		// If a global variable was edited in the function, save the changes.
		for (auto& oldV : oldVars) {
			for (auto newV : HPL::variables) {
				if (oldV.name == newV.name)
					oldV.value = newV.value;
			}
		}
		HPL::variables = oldVars;
		ifStatements.erase(ifStatements.begin());
		arg.curIndent.pop_back();

		if (!ifStatements.empty())
			mode = MODE_SCOPE_IF_STATEMENT;

		if (leftBracket || (rightBracket && ifStatements.empty()))
			return FOUND_SOMETHING;
	}
	else if (mode == MODE_SCOPE_IF_STATEMENT) {
		if (ifStatements.empty())
			return FOUND_SOMETHING;

		if (ifStatements.back().type == "invalid") {
			return FOUND_SOMETHING;
		}
		if (useRegex(line, R"(^\s*if\s*(.*)\s*\{$)")) {
			return FOUND_NOTHING;
		}
		ifStatements.back().code.push_back(line);

		return FOUND_SOMETHING;
	}

	return FOUND_NOTHING;
}


int HPL::checkConditions() {
	if (useRegex(line, R"(^\s*if\s*(.*)\s*$)")) {
		std::string oldValue = removeFrontAndBackSpaces(matches.str(1));
		bool rightBracket = false;

		if (oldValue.back() == '{') {
			rightBracket = true;
			oldValue.pop_back();
			mode = MODE_SCOPE_IF_STATEMENT;
		}
		else
			mode = MODE_CHECK_IF_STATEMENT;


		auto params = split(oldValue, " ", "\"\"{}"); // NOTE: Need to a fix a bug when there's no space between an operator and two values (eg. 33=="@34")

		ifStatements.push_back({.startingLine = equalBrackets});

		if (HPL::arg.debugAll || HPL::arg.debugLog) {
			arg.curIndent += "\t\t";
			std::cout << arg.curIndent << "LOG: [FOUND][IF-STATEMENT]: " << curFile << ":" << lineCount << ": if (<condition>): if (" << oldValue << ")" << std::endl;
		}

		std::string _operator;
		bool failed = true;

		for (int i = 0; i < params.size(); i++) {
			auto& p = params[i];
			p = removeFrontAndBackSpaces(p);

			HPL::variable var = {.type = "NO_TYPE"};
			setCorrectValue(var, p);
			p = xToStr(var.value);

			if (HPL::arg.debugAll || HPL::arg.debugLog) {
				std::cout << arg.curIndent << "LOG: [CHECKING][IF-STATEMENT-VALUE]: " << curFile << ":" << lineCount << ": <value> ([type]): " << p << " (" << var.type << ")" << std::endl;
			}

			if (p == "false" || p == "0") {
				for (int x = i; x < params.size(); x++) {
					if (params[x] == "||") {
						failed = false;
						break;
					}
				}

				if (!failed) {
					failed = true;
					continue;
				}

				ifStatements.back().type = "invalid";
				mode = rightBracket ? MODE_SCOPE_IGNORE_ALL : MODE_CHECK_IGNORE_ALL;

				if (HPL::arg.debugAll || HPL::arg.debugLog) {
					arg.curIndent.pop_back();
					arg.curIndent.pop_back();
					std::cout << arg.curIndent << "LOG: [FAILED][IF-STATEMENT]: " << curFile << ":" << lineCount << ": Condition failed, output returned false." << std::endl;
				}
				return FOUND_SOMETHING;
			}
			if (var.type == "relational-operator") {
				_operator = p;
				continue;
			}

			if (!_operator.empty()) {
				bool res = false;

				if (_operator == "==")
					res = (params[i - 2] == p);
				else if (_operator == "!=")
					res = (params[i - 2] != p);
				else if (_operator == ">=")
					res = (params[i - 2] >= p);
				else if (_operator == "<=")
					res = (params[i - 2] <= p);
				else if (_operator == ">")
					res = (params[i - 2] >  p);
				else if (_operator == "<")
					res = (params[i - 2] <  p);

				if (HPL::arg.debugAll || HPL::arg.debugLog) {
					std::cout << arg.curIndent << "LOG: [OPERATOR][RELATION]: " << curFile << ":" << lineCount << ": <value 1> <operator> <value 2>: " << params[i - 2] << " " << _operator << " " <<  p << " (" << (res == true ? "true" : "false") << ")" << std::endl;
				}

				_operator = "";

				if (!res) {
					for (int x = i; x < params.size(); x++) {
						if (params[x] == "||") {
							failed = false;
							break;
						}
					}

					if (!failed) {
						failed = true;
						continue;
					}

					ifStatements.back().type = "invalid";
					mode = rightBracket ? MODE_SCOPE_IGNORE_ALL : MODE_CHECK_IGNORE_ALL;

					if (HPL::arg.debugAll || HPL::arg.debugLog) {
						arg.curIndent.pop_back();
						std::cout << arg.curIndent << "LOG: [FAILED][IF-STATEMENT]: " << curFile << ":" << lineCount << ": Condition failed, output returned false." << std::endl;
					}

					return FOUND_SOMETHING;
				}
			}
		}
		arg.curIndent.pop_back();

		return FOUND_SOMETHING;
	}

	return FOUND_NOTHING;
}


int HPL::checkStruct() {
	// 'struct <name> {' Only matches the <name>
	if (useRegex(line, R"(struct\s+([a-zA-Z]+))")) {
		std::string name = removeSpaces(matches.str(1));

		structures.insert(structures.begin(), {name});
		mode = MODE_CHECK_STRUCT;

		if (HPL::arg.debugAll || HPL::arg.debugLog) {
			std::cout << arg.curIndent << "LOG: [CREATE][STRUCT]: " << curFile << ":" << lineCount << ": struct <name>: struct " << name << std::endl;
			arg.curIndent += "\t";
		}

		if (find(line, "{")) {
			mode = MODE_SAVE_STRUCT;
			return FOUND_SOMETHING;
		}
	}

	return FOUND_NOTHING;
}



int HPL::checkFunctions() {
	// Match <return type> <name>(<params>) {
	if (useRegex(line, R"(^\s*([^\s]+)\s+([^\s]+)\s*\((.*)\)\s*\{?$)") && matches.size() >= 3) {
		function func = {matches.str(1), matches.str(2)};
		std::vector<std::string> params = split(matches.str(3), ",", "\"\"");

		for (const auto& v : params) {
			if (useRegex(v, R"(\s*([^\s]+)\s+([^\s]+)?\s*=?\s*(f?\".*\"|\{.*\}|[^\s*]*)\s*)")) {
				variable var = {matches.str(1), matches.str(2)};

				if (matches.str(3).empty())
					func.minParamCount++;
				else
					var.value = unstringify(matches.str(3));

				func.params.push_back(var);
			}
		}
		func.file = HPL::curFile;
		func.startingLine = HPL::lineCount;

		functions.push_back(func);
		mode = MODE_CHECK_FUNC;

		if (HPL::arg.debugAll || HPL::arg.debugLog) {
			std::cout << arg.curIndent << "LOG: [CREATE][FUNCTION]: " << curFile << ":" << lineCount << ": <type> <name>(<params>): " << printFunction(func) << std::endl;
			arg.curIndent += "\t";
		}

		if (find(line, "{"))
			mode = MODE_SAVE_FUNC;

		return FOUND_SOMETHING;
	}
	else if (useRegex(line, R"(^\s*([^\s\(]+)\((.*)\)\s*$)") && matches.size() == 2) {
		function f; HPL::variable res;

		if (HPL::arg.debugAll || HPL::arg.debugLog) {
			std::cout << arg.curIndent << "LOG: [USE][FUNCTION]: " << curFile << ":" << lineCount << ": <name>(<params>): " << matches.str(1) << "(" << matches.str(2) << ")" << std::endl;
		}

		return executeFunction(matches.str(1), matches.str(2), f, res);
	}
	else if (useRegex(line, R"(^\s*return\s+(f?\".*\"|\{.*\}|[^\s]+\(.*\)|[^\s]*)\s*$)")) {
		setCorrectValue(functionOutput, matches.str(1));

		if (HPL::arg.debugAll || HPL::arg.debugLog) {
			std::cout << arg.curIndent << "LOG: [FOUND][RETURN]: " << curFile << ":" << lineCount << ": return <value> (<type>): return " << xToStr(functionOutput.value) << " (" << functionOutput.type << ")" << std::endl;
		}

		return FOUND_SOMETHING;
	}

	return FOUND_NOTHING;
}


int HPL::checkVariables() {
	// <name> = <value>
	if (useRegex(line, R"(^\s*([^\s]+)\s*[^\-\+\/\*]=\s*(f?\".*\"|\{.*\}|\w*\(.*\)|[\d\s\+\-\*\/\.]+[^\w]*|[^\s]*)\s*.*$)")) { // Edit a pre-existing variable
		variable info = {"", matches.str(1), matches.str(2)};
		auto& value = getStr(info.value);

		variable* existingVar = getVarFromName(info.name);


		if (existingVar != nullptr)
			setCorrectValue(*existingVar, value);
		else
			HPL::throwError(true, "Variable '%s' doesn't exist (Can't edit a variable that doesn't exist)", info.name.c_str());

		if (HPL::arg.debugAll || HPL::arg.debugLog) {
			std::cout << arg.curIndent << "LOG: [EDIT][VARIABLE]: " << curFile << ":" << lineCount << ": <variable> = <value>: " << existingVar->name << " = ";
			print(*existingVar);
		}
	}
	//<int> <operator> [value]
	else if (useRegex(line, R"(^\s*([^\s\-\+]+)\s*([\+\-\*\/\=]+)\s*(f?\".*\"|\{.*\}|[^\s]+\(.*\)|[^\s]*)\s*.*$)")) { // A math operator.
		variable* existingVar = getVarFromName(matches.str(1));
		double res;

		if (existingVar != nullptr && existingVar->type == "string" && matches.str(2) == "+=") {
			HPL::variable var;
			setCorrectValue(var, matches.str(3));
			if (var.type == "struct" || var.type == "scope") {
				HPL::throwError(true, "Cannot append a %s type to a string (Value '%s' is a %s-type).", var.type.c_str(), xToStr(var.value).c_str(), var.type.c_str());
			}
			existingVar->value = xToStr(existingVar->value) + xToStr(var.value);
		}

		else if (existingVar != nullptr && !(existingVar->type == "int" || existingVar->type == "float"))
			HPL::throwError(true, "Cannot perform any math operations to a non-int variable (Variable '%s' isn't int/float typed, can't operate to a '%s' type).", existingVar->name.c_str(), existingVar->type.c_str());

		else if (existingVar != nullptr) {
			double dec1 = xToType<float>(existingVar->value);
			double dec2 = std::stod(matches.str(3));

			if (matches.str(2) == "++")
				res = dec1 + 1;
			else if (matches.str(2) == "--")
				res = dec1 - 1;
			else if (matches.str(2) == "+=")
				res = dec1 + dec2;
			else if (matches.str(2) == "-=")
				res = dec1 - dec2;
			else if (matches.str(2) == "*=")
				res = dec1 * dec2;
			else if (matches.str(2) == "/=")
				res = dec1 / dec2;
			else if (matches.str(2) == "%=")
				res = fmod(dec1, dec2);

			if (existingVar->type == "int") {
				existingVar->value = (int)res;
			}
			else if (existingVar->type == "float") {
				existingVar->value = (float)res;
			}
		}
		else {
			throwError(true, "Cannot perform any math operations to this variable (Variable '%s' does not exist).", matches.str(1).c_str());
		}

		if (HPL::arg.debugAll || HPL::arg.debugLog) {
			std::cout << arg.curIndent << "LOG: [MATH][VARIABLE]: " << curFile << ":" << lineCount << ": <variable> <operator> [value]: " << existingVar->name << " " << matches.str(2);
			if (matches.str(2) != "--" || matches.str(2) != "++") std::cout << " " << res;
		}
	}
	// Matches the type, name and value
	else if (useRegex(line, R"(^\s*([^\s]+)\s+(\b[^=]+\b)\s*=?\s*(f?\".*\"|\{.*\}|[^\s]+\(.*\)|.*)\s*$)")) { // Declaring (a) new variable(s).
		auto listOfVarNames = split(matches.str(2), ",");
		auto listOfVarValues = split(matches.str(3), ",", "(){}\"\"");
		std::string ogType = matches.str(1);


		for (int listOfVarIndex = 0; listOfVarIndex < listOfVarNames.size(); listOfVarIndex++) {
			variable var = {.type = ogType, .name = removeSpaces(listOfVarNames[listOfVarIndex]), .value = std::string{}};

			if (listOfVarIndex < listOfVarValues.size())
				var.value = removeFrontAndBackSpaces(listOfVarValues[listOfVarIndex]);

			auto& value = getStr(var.value);
			structure s;

			if (value.empty())
				var.reset_value();

			if (!typeIsValid(var.type, &s)) // Type isn't cored or a structure.
				throwError(true, "Type '%s' doesn't exist (Cannot init a variable without valid type).", var.type.c_str());

			if (!s.name.empty()) { // The returned type from `typeIsValid` returned a struct
				if (useRegex(value, R"(\s*([^\s]+)\((.*)\))")) { // Value is a function.
					if (HPL::arg.debugAll || HPL::arg.debugLog) {
						std::cout << arg.curIndent << "LOG: [USE][FUNCTION]: " << curFile << ":" << lineCount << ": <name>(<params>): " << matches.str(1) << "(" << matches.str(2) << ")" << std::endl;
					}

					assignFuncReturnToVar(&var, HPL::matches.str(1), HPL::matches.str(2));
				}
				else if (!var.has_value()) { // Nothing is set, meaning it's just the struct's default arguments.
					std::vector<variable> res = s.value;
					var.value = res;
				}
				else { // Oh god oh fuck it's a custom list.
					// We don't split the string by the comma if the comma is inside double quotes
					// Meaning "This, yes this, exact test string" won't be split. We also remove the curly brackets before splitting.
					std::vector<std::string> valueList = split(unstringify(value, true), ",", "\"\"{}");
					std::vector<HPL::variable> result;

					if (valueList.size() > s.value.size()) {
						throwError(true, "Too many values are provided when declaring the variable '%s' (you provided '%i' arguments when struct type '%s' has only '%i' members).", var.name.c_str(), valueList.size(), var.type.c_str(), s.value.size());
					}

					// The first removes the spaces, then the double quotes.
					for (int i = 0; i < s.value.size(); i++) {
						if (i < s.value.size() && i < valueList.size()) {
							result.push_back({s.value[i].type, s.value[i].name, unstringify(removeFrontAndBackSpaces(valueList[i]))});
						}
						else { // Looks like the user didn't provide the entire argument list. That's fine, though we must check for any default options.
							if (s.value[i].has_value()) {
								result.push_back(s.value[i]);
							}
							else if (arg.strict)
								HPL::throwError(true, "Too few values are provided to fully initialize a struct (you provided '%i' arguments when struct type '%s' has '%i' members).", valueList.size(), var.type.c_str(), s.value.size());
							else
								HPL::throwError(true, "Shouldn't happen?");
						}
					}

					var.value = result;
				}
			}
			else if (!value.empty())
				setCorrectValue(var, value);

			if (mode == MODE_SAVE_STRUCT) // Save variables inside a struct.
				structures.begin()->value.push_back(var);
			else
				variables.push_back(var);


			if (HPL::arg.debugAll || HPL::arg.debugLog) {
				std::cout << arg.curIndent << "LOG: [CREATE][VARIABLE]: " << curFile << ":" << lineCount << ": <type> <variable> = [value]: " << var.type << " " << var.name;
				if (!value.empty()) {
					std::cout << " = ";
					print(var, "");
				}
				std::cout << std::endl;
			}
		}
		return FOUND_SOMETHING;
	}


	return FOUND_NOTHING;
}


void HPL::debugMode() {
	std::cout << colorText("============ DEBUG INFORMATION ============\n", HPL::OUTPUT_PURPLE);

	std::cout << colorText("Variables:\n", OUTPUT_CYAN, true);
	debugPrintVar(variables);
	std::cout << colorText("\nStructures:\n", OUTPUT_CYAN, true);
	debugPrintStruct(structures);
	std::cout << colorText("Functions:\n", OUTPUT_CYAN, true);
	debugPrintFunc(functions);
}


void HPL::debugPrintVar(std::vector<variable> vars, std::string tabs/* = "	"*/, std::string end/* = "\n"*/) {
	for (int index = 0; index < vars.size(); index++) {  // Regular variables
		auto& var = vars[index];

		RETURN_OUTPUT clr = OUTPUT_PURPLE;

		if (!coreTyped(var.type))
			clr = OUTPUT_NOTHING;

		std::cout << tabs << colorText(var.type, clr) << " " << var.name;
		if (var.has_value()) std::cout << " = ";
		print(var, "");

		if (index + 1 < vars.size()) std::cout << end;
	}
}


void HPL::debugPrintStruct(std::vector<structure> structList, std::string indent/* = "\t"*/) {
	for (auto s : structures) {
		std::cout << indent << colorText("struct", HPL::OUTPUT_PURPLE) << " " << colorText(s.name, HPL::OUTPUT_YELLOW) << colorText(" {", HPL::OUTPUT_GREEN) << std::endl;
		debugPrintVar(s.value, indent + indent);
		std::cout << colorText("\n" + indent + "}\n", HPL::OUTPUT_GREEN);
	}
}


void HPL::debugPrintFunc(std::vector<function> func, std::string indent/* = "\t"*/) {
	for (auto f : functions) {
		RETURN_OUTPUT clr = OUTPUT_PURPLE;

		if (f.type == "scope")
			clr = OUTPUT_BLUE;
		else if (!coreTyped(f.type))
			clr = OUTPUT_RED;

		std::cout << indent << colorText(f.type, clr) << " " << f.name << "("; debugPrintVar(f.params, "", ", "); std::cout << ") {\n";

		for (auto c : f.code)
			std::cout << indent << c << std::endl;

		std::cout << indent << "}" << std::endl;
	}
}


void HPL::resetRuntimeInfo() {
	curFile.clear();
	line.clear();
	functionOutput.reset_value();

	lineCount = 0;
	mode = MODE_DEFAULT;
	matches = {};
}


void HPL::throwError(bool sendRuntimeError, std::string text, ...) {
	va_list valist;
	va_start(valist, text);
	bool colorMode = false;
	
	#if defined(WINDOWS)
	std::string msg;
	std::cout << colorText("Error at ", OUTPUT_RED) << "'" << colorText(curFile + ":" + std::to_string(lineCount), OUTPUT_YELLOW) << "'" << colorText(": ", OUTPUT_RED);
	#else
	std::string msg = colorText("Error at ", OUTPUT_RED) + "'" + colorText(curFile + ":" + std::to_string(lineCount), OUTPUT_YELLOW) + "'" + colorText(": ", OUTPUT_RED);
	#endif

	for (int i = 0; i < text.size(); i++) {
		auto x = text[i];
		int num = 0;
		colorMode = false;

		if (x == '%' && (i + 1) < text.size()) {
			if ((i + 1) < text.size()) {
				if (text[i - 1] == '\'' && text[i + 2] == '\'')
					colorMode = true;
			}

			switch (text[i + 1]) {
				case 's':
					if (colorMode) {
						#if defined(WINDOWS)
						std::cout << msg << colorText(va_arg(valist, const char*), OUTPUT_YELLOW);
						#else
						msg += colorText(va_arg(valist, const char*), OUTPUT_YELLOW);
						#endif
						msg.clear();
					}
					else {
						#if defined(WINDOWS)
						std::cout << msg << va_arg(valist, const char*);
						msg.clear();
						#else
						msg += va_arg(valist, const char*), OUTPUT_YELLOW;
						#endif
					}

					break;
				case 'd':
				case 'i':
					num = va_arg(valist, int);

					if (colorMode) {
						#if defined(WINDOWS)
						std::cout << msg << colorText(std::to_string(num), OUTPUT_YELLOW);
						msg.clear();
						#else
						msg += colorText(std::to_string(num), OUTPUT_YELLOW);
						#endif
					}
					else {
						#if defined(WINDOWS)
						std::cout << msg << num;
						msg.clear();
						#else
						msg += std::to_string(num);
						#endif
					}

					break;

				default:
					break;
			}
			i++;
		}
		else msg += x;
	}
	va_end(valist);

	if (sendRuntimeError) {
		#if defined(WINDOWS) // Windows is fucking stupid with colored terminals.
		std::cout << msg << std::endl;

		if ((HPL::arg.debugAll || HPL::arg.debugPrint) && sendRuntimeError)
			debugMode();

		throw std::runtime_error("");
		#else
		throw std::runtime_error("\x1B[0m" + msg);
		#endif
	}
	else {
		#if !defined(WINDOWS) // Windows is fucking stupid with colored terminals.
		if ((HPL::arg.debugAll || HPL::arg.debugPrint) && sendRuntimeError)
			debugMode();
		#endif

		std::printf("\x1B[0m%s\n", msg.c_str());

		#if defined(WINDOWS) // Windows is fucking stupid with colored terminals.
		if ((HPL::arg.debugAll || HPL::arg.debugPrint) && sendRuntimeError)
			debugMode();
		#endif
	}
}


namespace HPL {
	// Inteperter configs.
	HPL::configArgs arg;

	// Interpreter rules runtime.
	std::string curFile;
	std::string line;
	int lineCount = 0;
	int mode = 0;
	HPL::vector matches;
	int equalBrackets = 0;

	// Defnitions that are saved in memory.
	std::vector<variable> variables;
	std::vector<structure> structures;
	std::vector<function> functions;
	std::vector<function> ifStatements;

	HPL::variable functionOutput;
}