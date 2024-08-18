#pragma once
#include <unordered_map>
#include <string>
#include <sstream>

struct Variable
{
	std::string name = "";
	std::string value = "";
};

class IniParser
{
	std::unordered_map<std::string, std::unordered_map<std::string, std::string>> data;

	std::string ParseSectionName(const std::string& line);
	Variable ParseVariable(const std::string& line);
public:
	IniParser(const char* filename);

	template<typename T>
	T Get(std::string section, std::string name);
};

template<typename T>
T IniParser::Get(std::string section, std::string name)
{
	auto it1 = data.find(section);
	if (it1 == data.end())
	{
		throw std::invalid_argument("Section: " + section + " doesnt exists");
	}
	auto it2 = (it1->second).find(name);
	if (it2 == (it1->second).end())
	{
		throw std::invalid_argument("Variable: " + name + " doesnt exists");
	}

	std::istringstream iss(it2->second);
	T result;
	if (!(iss >> result))
	{
		throw std::invalid_argument("Cannot convert string to specified type");
	}
	return result;
}
