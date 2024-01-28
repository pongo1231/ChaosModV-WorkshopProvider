#pragma once

#include "database.h"
#include "file.h"

#include <string>

#define TOKENS_DATABASE "data/tokens.db"

namespace token
{
	inline SQLite::Database &get_database()
	{
		static auto &database = []() -> SQLite::Database &
		{
			static SQLite::Database database(file::get_data_root() + TOKENS_DATABASE,
			                                 SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE | SQLite::OPEN_FULLMUTEX);
			database.exec("CREATE TABLE IF NOT EXISTS tokens("
			              "token TEXT PRIMARY KEY NOT NULL,"
			              "id TEXT NOT NULL,"
			              "creationtime INT NOT NULL)");
			return database;
		}();

		return database;
	}

	inline std::string get_token_user(const std::string &token)
	{
		std::string result;
		if (!database::exec_steps<std::string>(
		        get_database(), "SELECT id FROM tokens WHERE token=@token", { "@token", token },
		        [&](const SQLite::Statement &statement) { result = statement.getColumn(0).getString(); }))
			return {};

		return result;
	}

	inline bool add_user_token(const std::string &user_id, const std::string &token)
	{
		auto time = std::time(nullptr);
		return database::exec<std::string, std::string, time_t>(
		    get_database(), "INSERT INTO tokens (token, id, creationtime) VALUES (@token, @id, @creationtime)",
		    { "@token", token }, { "@id", user_id }, { "@creationtime", time });
	}

	inline bool does_token_exist(const std::string &token)
	{
		return database::exec_steps<std::string>(get_database(), "SELECT 1 FROM tokens WHERE token=@token",
		                                         { "@token", token });
	}

	inline std::vector<std::string> get_user_tokens(const std::string &user_id)
	{
		std::vector<std::string> tokens;
		if (!database::exec_steps<std::string>(
		        get_database(), "SELECT token FROM tokens WHERE id=@id ORDER BY creationtime", { "@id", user_id },
		        [&](const SQLite::Statement &statement) { tokens.push_back(statement.getColumn(0)); }))
			return {};

		return tokens;
	}

	inline bool erase_token(const std::string &token)
	{
		return database::exec<std::string>(get_database(), "DELETE FROM tokens WHERE token=@token",
		                                   { "@token", token });
	}

	inline bool erase_all_user_tokens(const std::string &user_id)
	{
		return database::exec<std::string>(get_database(), "DELETE FROM tokens WHERE id=@user_id",
		                                   { "@user_id", user_id });
	}
}