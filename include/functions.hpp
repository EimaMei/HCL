#include <interpreter.hpp>
#include <string.h>

/* ======================== CORE FUNCTIONS OF HCL ======================== */
// Each function define here must follow this format:
//      <Description of the function>
//      <Function implementation in HCL>
//      <The actual function defnition in C++>
//
// It's also pretty good to space out functions depending
// on which group they're categorized as for readability
// (for example, instead of having no space between 'print', 
// 'createFolder' and 'removeFolder', it would be best to
// put a space between 'print' and 'createFolder').

// Prints something out in the terminal.
// void print(var msg, string end = "\n")
void print(HCL::variable msg, std::string end = "\n");

// Creates a folder.
// int createFolder(string path, int mode = 0777)
int createFolder(std::string path, int mode = 0777);
// Removes a folder.
// int removeFolder(string path)
int removeFolder(std::string path);

// Creates a file.
// int createFile(string path, string content = "", bool useUtf8BOM = false)
int createFile(std::string, std::string content = "", bool useUtf8BOM = false);
// Reads a file.
// string readFile(string path)
std::string readFile(std::string path);
// Writes into a file.
// int writeFile(string path, string content, string mode = "w")
int writeFile(std::string path, std::string content, std::string mode = "w");
// Removes a file.
// int delete(string path)
int removeFile(std::string path);
// Copies a file to a new path.
// int copyFile(string source, string output)
int copyFile(std::string source, std::string output);
// Moves a file to a new path.
// int moveFile(string source, string output)
int moveFile(std::string source, std::string output);
// Writes to a pre-existing localisation file.
// int writeLocalisation(string file, string name, string description)
int writeLocalisation(std::string file, std::string name, std::string description);

// Converts an image to .dds and copies said converted image to "output"
// int convertToDds(string input, string output)
int convertToDds(std::string input, std::string output);

// Checks if a path already exists.
// int pathExists(string path)
bool pathExists(std::string path);

// Checks if a string is in 'line'
// bool find(string line, string str)
// (Check helper.hpp for the C++ definition)

// Replaces all instances of 'oldString' with 'newString' in 'str'
// string replaceAll(string str, string oldString, string newString);
// (Check helper.hpp for the C++ definition)

// Gets the filename from the path (eg. /usr/bin/somefile.img would turn to somefile.img).
// string getFilenameFromPath(string path)
std::string getFilenameFromPath(std::string path);