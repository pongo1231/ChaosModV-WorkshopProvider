#include "endpoint_common.h"

static std::shared_ptr<http_response> handle_endpoint_fetchsubmissions(const http_request &request)
{
	return make_response<string_response>(cache::fetch_compressed_submissions());
}

REGISTER_GET_ENDPOINT("/workshop/fetch_submissions", handle_endpoint_fetchsubmissions);

static std::shared_ptr<http_response> handle_endpoint_fetchsubmissiondata(const http_request &request)
{
	COMMON_PROLOGUE

	auto submission_id = ARG("submission_id");
	if (submission_id.empty())
		return make_response<string_response>(
		    json_formulate().set("success", false).set("reason", "Missing submission_id").to_string(), 400);

	if (!submission::submission_id_sanitycheck(submission_id))
		return make_response<string_response>(
		    json_formulate().set("success", false).set("reason", "Invalid submission_id").to_string(), 400);

	auto path = SUBMISSION_DIR_FRAGMENT + submission_id + SUBMISSION_DATA_FILE_COMPRESSED_FRAGMENT;

	if (!file::does_file_exist(path))
	{
		path = SUBMISSION_DIR_FRAGMENT + submission_id + SUBMISSION_DATA_FILE_FRAGMENT;

		if (!file::does_file_exist(path))
			return make_response<string_response>(
			    json_formulate().set("success", false).set("reason", "Teapot?").to_string(), 418);

		auto response = make_response<file_response>(path);
		response->with_header("compressed", "no");
		return response;
	}

	if (ARG("uncompressed") == "yes")
	{
		auto compressed = file::read_file(path);

		std::string uncompressed_buffer;
		// just to safeguard in case file size is above submission_max_total_size
		uncompressed_buffer.resize(compressed.size() < global_options.submission_max_total_size
		                               ? global_options.submission_max_total_size
		                               : compressed.size() * 2);
		auto uncompressed_size = ZSTD_decompress(uncompressed_buffer.data(), uncompressed_buffer.size(),
		                                         compressed.data(), compressed.size());
		uncompressed_buffer.resize(uncompressed_size);

		auto response = make_response<string_response>(uncompressed_buffer);
		response->with_header("compressed", "no");
		return response;
	}

	auto response = make_response<file_response>(path);
	response->with_header("compressed", "yes");
	return response;
}

REGISTER_GET_ENDPOINT("/workshop/fetch_submission_data", handle_endpoint_fetchsubmissiondata);

static std::shared_ptr<http_response> handle_endpoint_fetchsubmission(const http_request &request)
{
	COMMON_PROLOGUE

	auto submission_id = ARG("submission_id");
	if (submission_id.empty())
		return make_response<string_response>(
		    json_formulate().set("success", false).set("reason", "Missing submission_id").to_string(), 400);

	if (!submission::submission_id_sanitycheck(submission_id))
		return make_response<string_response>(
		    json_formulate().set("success", false).set("reason", "Invalid submission_id").to_string(), 400);

	json submission_json;
	if (!database::exec_steps<std::string>(submission::get_database(),
	                                       "SELECT * FROM submissions WHERE id=@submission_id",
	                                       { "@submission_id", submission_id },
	                                       [&](const SQLite::Statement &statement)
	                                       {
		                                       for (int i = 1; i < statement.getColumnCount(); i++)
			                                       submission_json[statement.getColumnName(i)] = statement.getColumn(i);
	                                       }))
		return make_response<string_response>(
		    json_formulate().set("success", false).set("reason", "Submission not found").to_string(), 400);

	return make_response<string_response>(
	    json_formulate().set("success", true).set("submission", submission_json).to_string());
}

REGISTER_GET_ENDPOINT("/workshop/fetch_submission", handle_endpoint_fetchsubmission);