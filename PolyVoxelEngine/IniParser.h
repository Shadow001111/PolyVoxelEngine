#pragma once
#include <unordered_map>
#include <string>
#include <sstream>
#include <iostream>

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
	T Get(std::string section, std::string name, T defaultValue);
};

template<typename T>
T IniParser::Get(std::string section, std::string name, T defaultValue)
{
	auto it1 = data.find(section);
	if (it1 == data.end())
	{
		std::cerr << "IniParser: Section " + section + " doesnt exists" << std::endl;
		return defaultValue;
	}
	auto it2 = (it1->second).find(name);
	if (it2 == (it1->second).end())
	{
		std::cerr << "IniParser: Variable " + name + " doesnt exists" << std::endl;
		return defaultValue;
	}

	std::istringstream iss(it2->second);
	T result;
	if (!(iss >> result))
	{
		std::cerr << "IniParser: Can not convert string to specified type" << std::endl;
		return defaultValue;
	}
	return result;
}
