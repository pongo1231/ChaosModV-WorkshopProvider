#pragma once

#include "cache.h"
#include "database.h"
#include "endpoint.h"
#include "file.h"
#include "html_file.h"
#include "json_response.h"
#include "options.h"
#include "submission.h"
#include "token.h"
#include "user.h"
#include "util.h"

#include <json.h>

#include <array>
#include <string>
#include <string_view>
#include <vector>

using namespace nlohmann;

#define COMMON_PROLOGUE auto args = request.get_args_flat();
#define ARG(x) util::string_sanitize(util::string_trim(std::string(args[x])))

#define TOKEN                                         \
	[&]()                                             \
	{                                                 \
		auto token = ARG("token");                    \
		if (token.empty())                            \
			token = request.get_cookie("user_token"); \
		return token;                                 \
	}()

#define USER_TOKEN_CHECK(submission_id)                                                                               \
	do                                                                                                                \
	{                                                                                                                 \
		auto token = TOKEN;                                                                                           \
		if (token.empty())                                                                                            \
			return make_response<string_response>(json_formulate_failure("Missing token"), 400);                      \
                                                                                                                      \
		if (!token::does_token_exist(token))                                                                          \
			return make_response<string_response>(json_formulate_failure("Invalid token"), 400);                      \
                                                                                                                      \
		if (!submission_id.empty() && !user::can_user_modify_submission(token::get_token_user(token), submission_id)) \
			return make_response<string_response>(json_formulate_failure("Not allowed"), 400);                        \
	} while (0)

#define COMMON_ADMIN                                                                           \
	do                                                                                         \
	{                                                                                          \
		USER_TOKEN_CHECK(std::string());                                                       \
		auto token = TOKEN;                                                                    \
		if (!user::is_user_admin(token::get_token_user(token)))                                \
			return make_response<string_response>(json_formulate_failure("Not allowed"), 400); \
	} while (0);

template <typename t> static std::shared_ptr<t> make_response(auto... args)
{
	auto response = std::make_shared<t>(args...);
	response->with_header("Access-Control-Allow-Origin", "*");
	return response;
}