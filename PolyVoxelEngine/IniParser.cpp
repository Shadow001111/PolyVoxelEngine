#include "IniParser.h"
#include <iostream>
#include <fstream>

template<typename KeyType, typename ValueType>
std::ostream& operator<<(std::ostream& os, const std::unordered_map<KeyType ,ValueType>& map_) {
	os << "{";

	int i = 0;
	int size_ = map_.size();
	for (const auto& pair : map_)
	{
		os << pair.first << " : " << pair.second;
		i++;
		if (i != size_)
		{
			os << ", ";
		}
	}

	os << "}";
	return os;
}

std::string removeWhiteSpaces(const std::string str)
{
	int length = str.length();
	if (length == 0)
	{
		return "";
	}

	int left_space = -1;
	int right_space = length;

	for (int i = 0; i < length; i++)
	{
		if (str[i] != ' ')
		{
			break;
		}
		left_space = i;
	}

	for (int i = length - 1; i >= 0; i--)
	{
		if (str[i] != ' ')
		{
			break;
		}
		right_space = i;
	}

	return str.substr(left_space + 1, right_space - 1 - left_space);
}

std::string IniParser::ParseSectionName(const std::string& line)
{
	std::string result;

	for (int i = 1; i < line.length(); i++)
	{
		char letter = line[i];
		if (letter == ']')
		{
			return result;
		}
		result += letter;
	}
	return "";
}

Variable IniParser::ParseVariable(const std::string& line)
{
	Variable var;

	std::istringstream iss(line);
	std::vector<std::string> tokens;
	std::string token;

	int length = 0;
	while (std::getline(iss, token, ' '))
	{
		tokens.push_back(token);
		length++;
		if (length == 3)
		{
			break;
		}
	}

	if (length != 3)
	{
		return var;
	}
	if (tokens[1] != "=")
	{
		return var;
	}

	var.name = tokens[0];
	var.value = tokens[2];

	return var;
}

IniParser::IniParser(const char* filename)
{
	// open file
	std::ifstream file(filename);
	if (!file.is_open())
	{
		std::cout << "File: '" + std::string(filename) + "' not found\n";
	}

	// parse file
	std::string line;
	std::string current_section_name = "";

	while (std::getline(file, line))
	{
		std::string clean_line = removeWhiteSpaces(line);
		if (clean_line.length() == 0)
		{
			continue;
		}

		std::string section_name;
		Variable variable;


		switch (clean_line[0])
		{
		case ';':
			break;
		case '[':
			section_name = ParseSectionName(clean_line);
			if (section_name != "")
			{
				current_section_name = std::move(section_name);
			}
			break;
		default:
			variable = ParseVariable(clean_line);
			if (variable.name == "")
			{
				break;
			}
			data[current_section_name][variable.name] = variable.value;
			break;
		}
	}
	file.close();
}