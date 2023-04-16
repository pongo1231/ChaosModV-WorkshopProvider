#include "endpoint_common.h"

#include "webhook.h"

static std::shared_ptr<http_response> handle_endpoint(const http_request &request)
{
	COMMON_PROLOGUE

	auto submission_id = ARG("submission_id");
	if (submission_id.empty())
		return make_response<string_response>(
		    json_formulate().set("success", false).set("reason", "Missing submission_id").to_string(), 400);

	if (!submission::submission_id_sanitycheck(submission_id))
		return make_response<string_response>(
		    json_formulate().set("success", false).set("reason", "Invalid submission_id").to_string(), 400);

	USER_TOKEN_CHECK(submission_id);
	auto token           = ARG("token");

	const auto &database = submission::get_database();

	std::string name;
	std::string author_user_id;
	if (!database::exec_steps<std::string>(submission::get_database(),
	                                       "SELECT name, author FROM submissions WHERE id=@submission_id",
	                                       { "@submission_id", submission_id },
	                                       [&](const SQLite::Statement &statement)
	                                       {
		                                       name           = statement.getColumn(0).getString();
		                                       author_user_id = user::get_user_id(statement.getColumn(1).getString());
	                                       }))
		return make_response<string_response>(
		    json_formulate().set("success", false).set("reason", "submission_id not found").to_string());

	if (!database::exec<std::string>(database, "DELETE FROM submissions WHERE id=@submission_id",
	                                 { "@submission_id", submission_id }))
		return make_response<string_response>(
		    json_formulate().set("success", false).set("reason", "Statement failed").to_string());

	std::filesystem::remove_all(SUBMISSION_DIR_FRAGMENT + submission_id);

	if (!author_user_id.empty())
		user::erase_user_attribute(author_user_id, submission_id);

	auto user_id = user::get_token_user(token);
	webhook::send({
	    .author = !user::is_user_admin(user_id) ? user::get_user_name(user_id) : "Admin",
	    .title  = "Submission removed",
	    .fields = { { "Name", !name.empty() ? name : "Unknown" } },
	});

	cache::invalidate_submissions_cache();

	return make_response<string_response>(json_formulate().set("success", true).to_string());
}

REGISTER_POST_ENDPOINT("/workshop/remove_submission", handle_endpoint);