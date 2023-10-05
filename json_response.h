#pragma once

#include <json.h>

#include <string_view>

struct json_formulation
{
	nlohmann::basic_json<> json_object;

	json_formulation &set(const std::string &key, auto value)
	{
		json_object[key] = value;
		return *this;
	}

	operator std::string() const
	{
		return json_object.dump();
	}
};

inline json_formulation json_formulate()
{
	return json_formulation();
}

inline json_formulation json_formulate_success()
{
	return json_formulation().set("success", true);
}

inline json_formulation json_formulate_failure(std::string_view reason)
{
	return json_formulation().set("success", false).set("reason", reason);
}