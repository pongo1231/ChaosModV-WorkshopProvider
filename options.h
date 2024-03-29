#pragma once

#include "file.h"
#include "logging.h"

#include <json.h>

#include <iostream>

#define OPTIONS_FILE "data/options.json"

inline struct global_options
{
	std::string domain;
	unsigned short port;
	bool use_tls;
	size_t connection_timeout;
	std::string webhook_url;
	std::string requestor_substitute_header;

	size_t user_min_name_length;
	size_t user_max_name_length;
	size_t user_max_password_length;
	size_t user_max_submissions;
	size_t user_max_active_tokens;
	size_t user_time_between_registrations;

	size_t submission_max_name_length;
	size_t submission_max_version_length;
	size_t submission_max_description_length;
	size_t submission_max_description_newlines;
	size_t submission_max_changelog_length;
	size_t submission_max_changelog_newlines;
	size_t submission_max_total_size;
	size_t submission_max_file_count;
	size_t submission_max_file_name_length;
	size_t submission_unpack_timeout;

	void read()
	{
		auto options_json = file::read_json_file(OPTIONS_FILE);
		if (options_json.empty())
		{
			LOG(RED << "ERROR: Missing or invalid " OPTIONS_FILE " file in DATA_ROOT path\n" << WHITE);
			exit(EXIT_FAILURE);
		}

		auto get = [&](const std::string &key, const std::string &subcategory = {})
		{
			try
			{
				if (!subcategory.empty())
					return options_json.at(subcategory).at(key);

				return options_json.at(key);
			}
			catch (std::exception)
			{
				LOG(RED << "ERROR: Missing config option " << key << "\n");
				exit(EXIT_FAILURE);
			}
		};

		domain                              = get("domain");
		port                                = get("port");
		use_tls                             = get("use_tls");
		connection_timeout                  = get("connection_timeout");
		webhook_url                         = get("webhook_url");
		requestor_substitute_header         = get("requestor_substitute_header");

		user_min_name_length                = get("min_name_length", "user");
		user_max_name_length                = get("max_name_length", "user");
		user_max_password_length            = get("max_password_length", "user");
		user_max_submissions                = get("max_submissions", "user");
		user_max_active_tokens              = get("max_active_tokens", "user");
		user_time_between_registrations     = get("time_between_registrations", "user");

		submission_max_name_length          = get("max_name_length", "submission");
		submission_max_version_length       = get("max_version_length", "submission");
		submission_max_description_length   = get("max_description_length", "submission");
		submission_max_description_newlines = get("max_description_newlines", "submission");
		submission_max_changelog_length     = get("max_changelog_length", "submission");
		submission_max_changelog_newlines   = get("max_changelog_newlines", "submission");
		submission_max_total_size           = get("max_total_size", "submission");
		submission_max_file_count           = get("max_file_count", "submission");
		submission_max_file_name_length     = get("max_file_name_length", "submission");
		submission_unpack_timeout           = get("unpack_timeout", "submission");
	}
} global_options;