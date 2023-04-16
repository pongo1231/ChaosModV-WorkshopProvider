#pragma once

#include <json.h>

struct json_formulation
{
	nlohmann::basic_json<> json_object;

	json_formulation &set(const std::string &key, auto value)
	{
		json_object[key] = value;
		return *this;
	}

	std::string to_string()
	{
		return json_object.dump();
	}
};

inline json_formulation json_formulate()
{
	return json_formulation();
}