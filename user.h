#pragma once

#include "cache.h"
#include "database.h"
#include "file.h"

#include <json.h>

#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#define USERS_DATABASE "data/users.db"
#define USER_DIR_FRAGMENT "data/users/"
#define USER_FILE_FRAGMENT "/user.json"

namespace user
{
	inline SQLite::Database &get_database()
	{
		static auto &database = []() -> SQLite::Database &
		{
			static SQLite::Database database(file::get_data_root() + USERS_DATABASE,
			                                 SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE | SQLite::OPEN_FULLMUTEX);
			database.exec("CREATE TABLE IF NOT EXISTS users("
			              "name     TEXT PRIMARY KEY NOT NULL,"
			              "id       TEXT             NOT NULL,"
			              "password TEXT             NOT NULL)");
			return database;
		}();

		return database;
	}

	inline std::unordered_map<std::string, std::string> &get_user_tokens()
	{
		static std::unordered_map<std::string, std::string> user_tokens;
		return user_tokens;
	}

	inline std::string get_token_user(const std::string &token)
	{
		const auto &user_tokens = get_user_tokens();
		if (!user_tokens.contains(token))
			return {};

		return user_tokens.at(token);
	}

	inline void set_token_user(const std::string &token, const std::string &user_id)
	{
		get_user_tokens()[token] = user_id;
	}

	inline bool does_token_exist(const std::string &token)
	{
		return get_user_tokens().contains(token);
	}

	inline std::vector<std::string> get_user_tokens(const std::string &user_id)
	{
		const auto &user_tokens = get_user_tokens();

		std::vector<std::string> tokens;
		std::for_each(user_tokens.begin(), user_tokens.end(),
		              [&](const auto &pair)
		              {
			              if (pair.second == user_id)
				              tokens.push_back(pair.first);
		              });

		return tokens;
	}

	inline void erase_token(const std::string &token)
	{
		get_user_tokens().erase(token);
	}

	inline bool does_user_id_exist(const std::string &user_id)
	{
		return database::exec_steps<std::string>(get_database(), "SELECT 1 FROM users WHERE id=@id",
		                                         { "@id", user_id });
	}

	inline std::string get_user_name(const std::string &user_id)
	{
		std::string result;
		database::exec_steps<std::string>(get_database(), "SELECT name FROM users WHERE id=@id", { "@id", user_id },
		                                  [&](const SQLite::Statement &statement)
		                                  {
			                                  if (result.empty())
				                                  result = statement.getColumn(0).getString();
		                                  });
		return result;
	}

	inline bool does_user_name_exist(const std::string &user_name)
	{
		return database::exec_steps<std::string>(get_database(), "SELECT 1 FROM users WHERE name=@user_name",
		                                         { "@user_name", user_name });
	}

	inline std::string get_user_id(const std::string &user_name)
	{
		std::string result;
		database::exec_steps<std::string>(get_database(), "SELECT id FROM users WHERE name=@user_name",
		                                  { "@user_name", user_name },
		                                  [&](const SQLite::Statement &statement)
		                                  {
			                                  if (result.empty())
				                                  result = statement.getColumn(0).getString();
		                                  });
		return result;
	}

	inline nlohmann::json get_user_json(const std::string &user_id)
	{
		return file::read_json_file(USER_DIR_FRAGMENT + user_id + USER_FILE_FRAGMENT);
	}

	template <typename t = nlohmann::json>
	inline t get_user_attribute(const std::string &user_id, const std::string &attribute)
	{
		return get_user_json(user_id)[attribute];
	}

	inline bool contains_user_attribute(const std::string &user_id, const std::string &attribute)
	{
		return get_user_json(user_id).contains(attribute);
	}

	inline void write_user_json(const std::string &user_id, const nlohmann::json &json)
	{
		file::write_file(USER_DIR_FRAGMENT + user_id + USER_FILE_FRAGMENT, json.dump(4));
	}

	inline void set_user_attribute(const std::string &user_id, const std::string &attribute, auto value)
	{
		auto user_json       = get_user_json(user_id);
		user_json[attribute] = value;
		write_user_json(user_id, user_json);
	}

	template <typename t> struct user_attribute
	{
		std::string attribute;
		t value;

		user_attribute(std::string attribute, t value) : attribute(attribute), value(value)
		{
		}
	};
	template <typename... t>
	inline void set_user_attributes(const std::string &user_id, const user_attribute<t> &...attributes)
	{
		auto user_json = get_user_json(user_id);

		(
		    [&]
		    {
			    user_json[attributes.attribute] = attributes.value;
		    }(),
		    ...);

		write_user_json(user_id, user_json);
	}

	inline void erase_user_attribute(const std::string &user_id, const std::string &attribute)
	{
		auto user_json = get_user_json(user_id);
		user_json.erase(attribute);
		write_user_json(user_id, user_json);
	}

	template <typename... t> inline void erase_user_attributes(const std::string &user_id, const t &...attributes)
	{
		auto user_json = get_user_json(user_id);
		for (const auto &attribute : { attributes... })
			user_json.erase(attribute);
		write_user_json(user_id, user_json);
	}

	inline bool is_user_admin(const std::string &user_id)
	{
		return contains_user_attribute(user_id, "is_admin");
	}

	inline bool can_user_modify_submission(const std::string &user_id, const std::string &submission_id)
	{
		if (is_user_admin(user_id))
			return true;

		return get_user_attribute(user_id, "submissions").contains(submission_id);
	}
}