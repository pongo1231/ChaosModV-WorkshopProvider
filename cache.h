#pragma once

#include "database.h"
#include "file.h"
#include "submission.h"

#include <json.h>
#include <zstd.h>

#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

namespace cache
{
	inline std::unordered_map<std::string, std::string> file_caches;
	inline std::unordered_map<std::string, std::mutex> file_mutexes;

	inline std::string fetch_compressed_submissions()
	{
		std::lock_guard lock(file_mutexes[SUBMISSIONS_DATABASE]);

		auto &submission_cache = file_caches[SUBMISSIONS_DATABASE];

		if (!submission_cache.empty())
			return submission_cache;

		// lazy update cache

		nlohmann::json cached_submissions_json;
		cached_submissions_json["submissions"] = nlohmann::json::object();

		database::exec_steps(submission::get_database(), "SELECT * FROM submissions",
		                     [&](const SQLite::Statement &statement)
		                     {
			                     std::string submission_id = statement.getColumn(0);
			                     for (int i = 1; i < statement.getColumnCount(); i++)
				                     cached_submissions_json["submissions"][submission_id][statement.getColumnName(i)] =
				                         statement.getColumn(i);
		                     });

		auto cached_submissions = cached_submissions_json.dump();

		auto file_size          = cached_submissions.size();
		auto bound_size         = ZSTD_compressBound(file_size);

		submission_cache.resize(bound_size);

		auto compressed_size =
		    ZSTD_compress(submission_cache.data(), bound_size, cached_submissions.data(), file_size, 10);

		submission_cache.resize(compressed_size);

		return submission_cache;
	}

	inline void invalidate_submissions_cache()
	{
		std::lock_guard lock(file_mutexes[SUBMISSIONS_DATABASE]);
		file_caches[SUBMISSIONS_DATABASE].clear();
	}
}