#pragma once

#include "database.h"
#include "endpoint.h"
#include "file.h"
#include "html_file.h"
#include "json_response.h"
#include "options.h"
#include "submission.h"
#include "user.h"
#include "util.h"

#include <json.h>

using namespace nlohmann;

#define COMMON_PROLOGUE auto args = request.get_args();
#define ARG(x) util::string_sanitize(util::string_trim(args[x]))

#define USER_TOKEN_CHECK(submission_id)                                                                              \
	do                                                                                                               \
	{                                                                                                                \
		auto token = ARG("token");                                                                                   \
		if (token.empty())                                                                                           \
			return make_response<string_response>(                                                                   \
			    json_formulate().set("success", false).set("reason", "Missing token").to_string(), 400);             \
                                                                                                                     \
		if (!user::does_token_exist(token))                                                                          \
			return make_response<string_response>(                                                                   \
			    json_formulate().set("success", false).set("reason", "Invalid token").to_string(), 400);             \
                                                                                                                     \
		if (!submission_id.empty() && !user::can_user_modify_submission(user::get_token_user(token), submission_id)) \
			return make_response<string_response>(                                                                   \
			    json_formulate().set("success", false).set("reason", "Not allowed").to_string(), 400);               \
	} while (0)

template <typename t> static std::shared_ptr<t> make_response(auto... args)
{
	auto response = std::make_shared<t>(args...);
	response->with_header("Access-Control-Allow-Origin", "*");
	return response;
}