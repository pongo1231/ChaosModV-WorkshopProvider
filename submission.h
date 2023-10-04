#pragma once

#include "file.h"

#include <SQLiteCpp/SQLiteCpp.h>
#include <json.h>

#include <algorithm>
#include <cctype>
#include <string_view>

#define SUBMISSIONS_DATABASE "data/submissions.db"
#define SUBMISSION_DIR_FRAGMENT "data/submissions/"
#define SUBMISSION_DATA_FILE_FRAGMENT "/data.zip"
#define SUBMISSION_DATA_FILE_COMPRESSED_FRAGMENT "/data.zip.zst"
#define SUBMISSION_CHANGELOG_FILE_FRAGMENT "/changelog.json"

namespace submission
{
	inline SQLite::Database &get_database()
	{
		static auto &database = []() -> SQLite::Database &
		{
			static SQLite::Database database(file::get_data_root() + SUBMISSIONS_DATABASE,
			                                 SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE | SQLite::OPEN_FULLMUTEX);
			database.exec("CREATE TABLE IF NOT EXISTS submissions("
			              "id          TEXT  PRIMARY KEY NOT NULL,"
			              "author      TEXT              NOT NULL,"
			              "description TEXT,"
			              "lastupdated INT          NOT NULL,"
			              "name        TEXT              NOT NULL,"
			              "sha256      TEXT,"
			              "version     TEXT              NOT NULL)");
			return database;
		}();

		return database;
	}

	inline bool submission_id_sanitycheck(std::string_view submission_id)
	{
		if (submission_id.length() != 16
		    || std::find_if(submission_id.begin(), submission_id.end(),
		                    [](auto c) { return !std::isalnum(c) || std::isupper(c); })
		           != submission_id.end())
			return false;

		return true;
	}

	inline nlohmann::json get_changelog_json(const std::string &submission_id)
	{
		return file::read_json_file(SUBMISSION_DIR_FRAGMENT + submission_id + SUBMISSION_CHANGELOG_FILE_FRAGMENT);
	}

	inline void write_changelog_json(const std::string &submission_id, const nlohmann::json &json)
	{
		file::write_file(SUBMISSION_DIR_FRAGMENT + submission_id + SUBMISSION_CHANGELOG_FILE_FRAGMENT, json.dump(4));
	}
}