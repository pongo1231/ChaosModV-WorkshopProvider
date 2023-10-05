#include "endpoint_common.h"

#include "webhook.h"

#include <elzip/elzip.hpp>
#include <sys/poll.h>
#include <zstd.h>

#include <array>
#include <string_view>

static std::array<int, 2> unpacking_process_pipe;

static size_t get_user_max_data_size(const std::string &user_id)
{
	if (user::contains_user_attribute(user_id, "custom_submission_size"))
	{
		auto custom_max_size = user::get_user_attribute(user_id, "custom_submission_size");
		try
		{
			return std::stoull(custom_max_size.get<std::string>());
		}
		catch (...)
		{
		}
	}

	return global_options.submission_max_total_size;
}

static pid_t handle_unpacking(std::string_view data, std::string_view data_content_path, const std::string &data_path,
                              size_t max_data_size)
{
	file::write_file(data_path, data);

	pipe(unpacking_process_pipe.data());

	auto pid = fork();
	switch (pid)
	{
	case -1: // error
		close(unpacking_process_pipe[0]);
		close(unpacking_process_pipe[1]);
		return -1;
	case 0:                               // new process
		close(unpacking_process_pipe[0]); // close read end
		break;
	default:                              // this process
		close(unpacking_process_pipe[1]); // close write end
		break;
	}

	if (pid) // this process
		return pid;

	auto write_pipe = [](std::string data)
	{
		data += "\n";
		write(unpacking_process_pipe[1], data.data(), data.size());
	};

	try
	{
		elz::extractZip(data_path, data_content_path);

		size_t data_file_count = 0, data_file_size = 0;
		for (const auto &entry : std::filesystem::recursive_directory_iterator(data_content_path))
		{
			data_file_count++;
			if (data_file_count > global_options.submission_max_file_count)
			{
				write_pipe(json_formulate_failure("Data contains too many files (max "
				                                  + std::to_string(global_options.submission_max_file_count) + ")"));
				exit(EXIT_FAILURE);
			}

			if (!entry.is_regular_file())
				continue;

			auto path              = entry.path();
			auto relative_path_str = std::filesystem::relative(path, data_content_path).string();
			auto extension         = path.extension();
			if (!path.has_extension() || (extension != ".lua" && extension != ".mp3" && extension != ".txt"))
			{
				write_pipe(json_formulate_failure("Data contains invalid file (" + relative_path_str + ")"));
				exit(EXIT_FAILURE);
			}
			else if (relative_path_str.length() > global_options.submission_max_file_name_length)
			{
				write_pipe(
				    json_formulate_failure("Data contains file over character limit (" + relative_path_str + ")"));
				exit(EXIT_FAILURE);
			}

			data_file_size += entry.file_size();
		}

		if (data_file_size == 0)
		{
			write_pipe(json_formulate_failure("Data contains no files"));
			exit(EXIT_FAILURE);
		}

		if (data_file_size > max_data_size)
		{
			write_pipe(json_formulate_failure("Data exceeds size limit (" + std::to_string(max_data_size / 1024 / 1024)
			                                  + "MB)"));
			exit(EXIT_FAILURE);
		}

		auto data_file_size_str = "data_file_len " + std::to_string(data_file_size);
		write_pipe(data_file_size_str);
	}
	catch (elz::zip_exception exception)
	{
		LOG(RED << exception.what() << "\n");
		write_pipe(json_formulate_failure("Data is not a valid zip file"));
		exit(EXIT_FAILURE);
	}

	write_pipe("finished");
	close(unpacking_process_pipe[1]);
	exit(EXIT_SUCCESS);

	return -1;
}

static std::shared_ptr<http_response> handle_endpoint(const http_request &request)
{
	COMMON_PROLOGUE

	struct update_state
	{
		bool new_submission = false;
	} update_state;

	std::string submission_id = ARG("submission_id");
	if (!submission_id.empty() && !submission::submission_id_sanitycheck(submission_id))
		return make_response<string_response>(json_formulate_failure("Invalid submission_id"), 400);

	USER_TOKEN_CHECK(submission_id);
	auto token           = ARG("token");
	auto user_id         = user::get_token_user(token);

	const auto &database = submission::get_database();

	std::string orig_author, orig_description, orig_name, orig_sha256, orig_version;
	int64_t orig_lastupdated = 0;
	if (!submission_id.empty()
	    && !database::exec_steps<std::string>(database, "SELECT * FROM submissions WHERE id=@submission_id",
	                                          { "@submission_id", submission_id },
	                                          [&](const SQLite::Statement &statement)
	                                          {
		                                          orig_author      = statement.getColumn("author").getString();
		                                          orig_description = statement.getColumn("description").getString();
		                                          orig_lastupdated = statement.getColumn("lastupdated").getInt64();
		                                          orig_name        = statement.getColumn("name").getString();
		                                          orig_sha256      = statement.getColumn("sha256").getString();
		                                          orig_version     = statement.getColumn("version").getString();
	                                          }))
		return make_response<string_response>(json_formulate_failure("Invalid submission_id"), 400);

	if (submission_id.empty())
	{
		// check if user has reached max submissions count
		if (user::get_user_attribute(user_id, "submissions").size() >= global_options.user_max_submissions)
			return make_response<string_response>(
			    json_formulate_failure(
			        "Submissions limit reached, delete an existing one before creating a new submission"),
			    400);

		while (submission_id.empty())
		{
			submission_id = util::generate_random_string();

			// check if id doesn't exist already for safety
			if (database::exec_steps<std::string>(database, "SELECT 1 FROM submissions WHERE id=@submission_id",
			                                      { "@submission_id", submission_id }))
				submission_id.clear();
		}

		update_state.new_submission = true;
	}

	auto name = ARG("name");
	if (name.empty() && !orig_name.empty())
		name = orig_name;

	if (name.empty())
		return make_response<string_response>(json_formulate_failure("Missing name"), 400);

	if (name.length() > global_options.submission_max_name_length || name.find('\n') != name.npos
	    || name.find('\\') != name.npos)
		return make_response<string_response>(json_formulate_failure("Invalid name"), 400);

	if (database::exec_steps<std::string, std::string>(
	        database, "SELECT 1 FROM submissions WHERE name=@name AND NOT id=@submission_id", { "@name", name },
	        { "@submission_id", submission_id }))
		return make_response<string_response>(json_formulate_failure("Name already in use"), 400);

	std::string version = ARG("version");
	if (version.empty() && !orig_version.empty())
		version = orig_version;

	if (version.empty())
		return make_response<string_response>(json_formulate_failure("Missing version"), 400);

	if (version.length() > global_options.submission_max_version_length || version.starts_with('.')
	    || version.ends_with('.')
	    || !std::all_of(version.begin(), version.end(), [](char c) { return std::isdigit(c) || c == '.'; }))
		return make_response<string_response>(json_formulate_failure("Invalid version"), 400);

	std::string description = ARG("description");
	if (description.empty() && !orig_description.empty())
		description = orig_description;

	if (description.length() > global_options.submission_max_description_length
	    || std::count(description.begin(), description.end(), '\n')
	           > global_options.submission_max_description_newlines)
		return make_response<string_response>(json_formulate_failure("Invalid description"), 400);

	auto changelog = ARG("changelog");
	if (!changelog.empty()
	    && (changelog.size() > global_options.submission_max_changelog_length
	        || std::count(changelog.begin(), changelog.end(), '\n') > global_options.submission_max_changelog_newlines))
		return make_response<string_response>(json_formulate_failure("Invalid changelog"), 400);

	auto author = user::get_user_name(user_id);
	if (user::is_user_admin(user_id) || orig_author.empty())
		author = !author.empty() ? author : (!orig_author.empty() ? orig_author : "Unknown");

	auto data_target_path                 = file::get_data_root() + SUBMISSION_DIR_FRAGMENT + submission_id;
	auto data_target_data_path            = data_target_path + SUBMISSION_DATA_FILE_FRAGMENT;
	auto data_target_compressed_data_path = data_target_path + SUBMISSION_DATA_FILE_COMPRESSED_FRAGMENT;

	std::string sha256                    = orig_sha256;
	int64_t lastupdated                   = orig_lastupdated;

	auto data                             = args["data"];
	if (data.empty() && !file::does_file_exist(data_target_compressed_data_path)
	    && !file::does_file_exist(data_target_data_path))
		return make_response<string_response>(json_formulate_failure("Missing data"), 400);

	size_t data_file_len = 0;
	if (!data.empty())
	{
		auto max_data_size = get_user_max_data_size(user_id);
		if (data.length() > max_data_size)
			return make_response<string_response>(json_formulate_failure("Data exceeds size limit ("
			                                                             + std::to_string(max_data_size / 1024 / 1024)
			                                                             + " MB)"),
			                                      400);

		std::string data_content_path = "/tmp/" + submission_id;
		std::string data_path         = data_content_path + ".zip";
		auto cleanup                  = [&]()
		{
			std::filesystem::remove(data_path);
			std::filesystem::remove_all(data_content_path);
		};

		auto unpacking_process_pid = handle_unpacking(data, data_content_path, data_path, max_data_size);
		if (unpacking_process_pid < 1)
			return make_response<string_response>(json_formulate_failure("Internal error (unpacking_process_pid < 1)"),
			                                      500);
		auto start_time = std::chrono::system_clock::now();
		while (true)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));

			auto cur_time                                = std::chrono::system_clock::now();
			std::chrono::duration<float> elapsed_seconds = cur_time - start_time;
			if (elapsed_seconds.count() > global_options.submission_unpack_timeout)
			{
				kill(unpacking_process_pid, SIGTERM);
				cleanup();
				return make_response<string_response>(
				    json_formulate_failure(
				        "Timeout while verifying data, try reducing the amount of files / compression"),
				    400);
			}

			// read is blocking so don't do it unless necessary
			struct pollfd poll_descriptor;
			poll_descriptor.fd     = unpacking_process_pipe[0];
			poll_descriptor.events = POLLIN;
			int poll_amount        = poll(&poll_descriptor, 1, 0);
			if (poll_amount < 1)
				continue;

			std::string read_buffer;
			read_buffer.resize(512);
			auto read_bytes = read(unpacking_process_pipe[0], read_buffer.data(), read_buffer.size());
			if (read_bytes > 0)
			{
				read_buffer.resize(read_bytes);
				auto messages = util::string_split(read_buffer, "\n");

				bool finished = false;
				for (const auto &message : messages)
				{
					if (message == "finished")
					{
						finished = true;
						break;
					}
					else if (message.starts_with("data_file_len"))
						data_file_len = std::stoull(util::string_split(message, " ")[1]);
					else // should be json containing error code
					{
						cleanup();
						return make_response<string_response>(message, 400);
					}
				}

				if (finished)
					break;
			}
		}

		close(unpacking_process_pipe[0]);

		std::filesystem::create_directories(data_target_path);

		elz::zipFolder(data_content_path, data_target_data_path);

		cleanup();

		auto data_file_content = file::read_file(data_target_data_path);
		auto data_file_size    = data_file_content.size();

		std::string data_file_compressed_content;

		auto bound_size = ZSTD_compressBound(data_file_size);
		data_file_compressed_content.resize(bound_size);

		auto compressed_size = ZSTD_compress(data_file_compressed_content.data(), bound_size, data_file_content.data(),
		                                     data_file_size, 10);

		data_file_compressed_content.resize(compressed_size);

		file::write_file(data_target_compressed_data_path, data_file_compressed_content);

		std::filesystem::remove(data_target_data_path);

		sha256      = util::sha256(data_file_compressed_content);
		lastupdated = std::time(0);
	}

	if (!database::exec<std::string, std::string, std::string, int64_t, std::string, std::string, std::string>(
	        database,
	        "REPLACE INTO submissions (id, author, description, lastupdated, name, sha256, version) VALUES "
	        "(@submission_id, @author, @description, @lastupdated, @name, @sha256, @version)",
	        { "@submission_id", submission_id }, { "@author", author }, { "@description", description },
	        { "@lastupdated", lastupdated }, { "@name", name }, { "@sha256", sha256 }, { "@version", version }))
		return make_response<string_response>(
		    json_formulate_failure("Timeout while verifying data, try reducing the amount of files / compression"),
		    400);

	cache::invalidate_submissions_cache();

	auto user_json                          = user::get_user_json(user_id);
	user_json["submissions"][submission_id] = json::object();
	user::write_user_json(user_id, user_json);

	if (!changelog.empty())
	{
		auto changelog_json                  = submission::get_changelog_json(submission_id);
		changelog_json["changelog"][version] = changelog;
		submission::write_changelog_json(submission_id, changelog_json);
	}

	bool send_webhook = true;
	webhook::webhook_options webhook_options {
		.author = !user::is_user_admin(user_id) ? author : "Admin",
		.title  = update_state.new_submission ? "New submission" : "Submission updated",
	};
	webhook_options.fields.emplace_back("Name", !orig_name.empty() ? orig_name : name);
	if (!orig_name.empty() && name != orig_name)
		webhook_options.fields.emplace_back("New name", name);
	if (description != orig_description || update_state.new_submission)
		webhook_options.fields.emplace_back(!orig_description.empty() ? "New description" : "Description",
		                                    !description.empty() ? description : "No Description");
	if (version != orig_version || update_state.new_submission)
	{
		if (update_state.new_submission)
			webhook_options.fields.emplace_back("Version", "v" + version);
		else
			webhook_options.fields.emplace_back("New version", "v" + orig_version + " -> v" + version);
	}
	if (!changelog.empty())
		webhook_options.fields.emplace_back("Changelog", changelog);
	if (!data.empty())
	{
		webhook_options.description = "New file uploaded";
		std::string size_text;
		size_text.resize(64);
		std::sprintf(size_text.data(), "Size: %.2fMB", data_file_len / 1000.f / 1000.f);
		webhook_options.footer = size_text;
	}
	else if (webhook_options.fields.size() < 2)
		send_webhook = false;

	if (send_webhook)
		webhook::send(webhook_options);

	return make_response<string_response>(json_formulate_success().set("submission_id", submission_id));
}

REGISTER_POST_ENDPOINT("/workshop/update_submission", handle_endpoint);