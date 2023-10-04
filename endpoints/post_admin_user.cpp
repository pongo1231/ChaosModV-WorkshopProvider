#include "endpoint_common.h"

#include "webhook.h"

#define COMMON_ADMIN                                                                                   \
	do                                                                                                 \
	{                                                                                                  \
		USER_TOKEN_CHECK(std::string());                                                               \
		auto token = ARG("token");                                                                     \
		if (!user::is_user_admin(user::get_token_user(token)))                                         \
			return make_response<string_response>(                                                     \
			    json_formulate().set("success", false).set("reason", "Not allowed").to_string(), 400); \
	} while (0);

static std::shared_ptr<http_response> handle_endpoint_admincreateuser(const http_request &request)
{
	COMMON_PROLOGUE
	COMMON_ADMIN

	auto name = ARG("name");
	if (name.empty())
		return make_response<string_response>(
		    json_formulate().set("success", false).set("reason", "Missing name").to_string(), 400);

	if (user::does_user_name_exist(name))
		return make_response<string_response>(
		    json_formulate().set("success", false).set("reason", "Name already in use").to_string(), 400);

	auto password = ARG("password");
	if (password.empty())
		return make_response<string_response>(
		    json_formulate().set("success", false).set("reason", "Missing password").to_string(), 400);

	password = util::sha512(password);

	std::string user_id;
	while (user_id.empty())
	{
		user_id = util::generate_random_string();
		if (user::does_user_id_exist(user_id))
			user_id.clear();
	}

	if (!database::exec<std::string, std::string, std::string>(
	        user::get_database(), "INSERT INTO users (name, id, password) VALUES (@name, @user_id, @password)",
	        { "@name", name }, { "@user_id", user_id }, { "@password", password }))
		return make_response<string_response>(
		    json_formulate().set("success", false).set("reason", "Statement failed").to_string(), 400);

	std::filesystem::create_directories(file::get_data_root() + USER_DIR_FRAGMENT + user_id);

	return make_response<string_response>(json_formulate().set("success", true).set("user_id", user_id).to_string(),
	                                      400);
}

REGISTER_POST_ENDPOINT("/admin/user/create", handle_endpoint_admincreateuser);

static std::shared_ptr<http_response> handle_endpoint_admingetuserid(const http_request &request)
{
	COMMON_PROLOGUE
	COMMON_ADMIN

	auto user_name = ARG("name");
	if (user_name.empty())
		return make_response<string_response>(
		    json_formulate().set("success", false).set("reason", "Missing name").to_string(), 400);

	if (!user::does_user_name_exist(user_name))
		return make_response<string_response>(
		    json_formulate().set("success", false).set("reason", "User doesn't exist").to_string(), 400);

	return make_response<string_response>(
	    json_formulate().set("success", true).set("user_id", user::get_user_id(user_name)).to_string(), 400);
}

REGISTER_POST_ENDPOINT("/admin/user/get_id", handle_endpoint_admingetuserid);

static std::shared_ptr<http_response> handle_endpoint_adminsetuserpassword(const http_request &request)
{
	COMMON_PROLOGUE
	COMMON_ADMIN

	auto user_id = ARG("user_id");
	if (user_id.empty())
		return make_response<string_response>(
		    json_formulate().set("success", false).set("reason", "Missing user_id").to_string(), 400);

	if (!user::does_user_id_exist(user_id))
		return make_response<string_response>(
		    json_formulate().set("success", false).set("reason", "User doesn't exist").to_string(), 400);

	auto password = ARG("password");
	if (password.empty())
		return make_response<string_response>(
		    json_formulate().set("success", false).set("reason", "Missing password").to_string(), 400);

	password = util::sha512(password);

	if (!database::exec<std::string, std::string>(user::get_database(),
	                                              "UPDATE users SET password = @password WHERE id = @user_id",
	                                              { "@password", password }, { "@user_id", user_id }))
		return make_response<string_response>(
		    json_formulate().set("success", false).set("reason", "Statement failed").to_string(), 400);

	return make_response<string_response>(json_formulate().set("success", true).to_string(), 400);
}

REGISTER_POST_ENDPOINT("/admin/user/set_password", handle_endpoint_adminsetuserpassword);

static std::shared_ptr<http_response> handle_endpoint_adminsetuserattribute(const http_request &request)
{
	COMMON_PROLOGUE
	COMMON_ADMIN

	auto user_id = ARG("user_id");
	if (user_id.empty())
		return make_response<string_response>(
		    json_formulate().set("success", false).set("reason", "Missing user_id").to_string(), 400);

	if (!user::does_user_id_exist(user_id))
		return make_response<string_response>(
		    json_formulate().set("success", false).set("reason", "User doesn't exist").to_string(), 400);

	auto attribute_name = ARG("attribute");
	if (attribute_name.empty())
		return make_response<string_response>(
		    json_formulate().set("success", false).set("reason", "Missing attribute").to_string(), 400);

	auto attribute_value = ARG("value");
	if (attribute_value.empty())
		return make_response<string_response>(
		    json_formulate().set("success", false).set("reason", "Missing value").to_string(), 400);

	std::filesystem::create_directories(file::get_data_root() + USER_DIR_FRAGMENT + user_id);

	user::set_user_attribute(user_id, attribute_name, attribute_value);

	if (attribute_name == "suspended")
	{
		for (const auto &token : user::get_user_tokens(user_id))
			user::erase_token(token);

		webhook::send({
		    .author      = "Admin",
		    .fields      = { { "Username", user::get_user_name(user_id) } },
		    .description = "Account has been suspended",
		});
	}

	return make_response<string_response>(json_formulate().set("success", true).to_string(), 400);
}

REGISTER_POST_ENDPOINT("/admin/user/set_attribute", handle_endpoint_adminsetuserattribute);

static std::shared_ptr<http_response> handle_endpoint_adminclearuserattribute(const http_request &request)
{
	COMMON_PROLOGUE
	COMMON_ADMIN

	auto user_id = ARG("user_id");
	if (user_id.empty())
		return make_response<string_response>(
		    json_formulate().set("success", false).set("reason", "Missing user_id").to_string(), 400);

	if (!user::does_user_id_exist(user_id))
		return make_response<string_response>(
		    json_formulate().set("success", false).set("reason", "User doesn't exist").to_string(), 400);

	auto attribute_name = ARG("attribute");
	if (attribute_name.empty())
		return make_response<string_response>(
		    json_formulate().set("success", false).set("reason", "Missing attribute").to_string(), 400);

	std::filesystem::create_directories(file::get_data_root() + USER_DIR_FRAGMENT + user_id);

	user::erase_user_attribute(user_id, attribute_name);

	if (attribute_name == "suspended")
		webhook::send({
		    .author      = "Admin",
		    .fields      = { { "Username", user::get_user_name(user_id) } },
		    .description = "Account has been unsuspended",
		});

	return make_response<string_response>(json_formulate().set("success", true).to_string(), 400);
}

REGISTER_POST_ENDPOINT("/admin/user/clear_attribute", handle_endpoint_adminclearuserattribute);

static std::shared_ptr<http_response> handle_endpoint_admingetuserattributes(const http_request &request)
{
	COMMON_PROLOGUE
	COMMON_ADMIN

	auto user_id = ARG("user_id");
	if (user_id.empty())
		return make_response<string_response>(
		    json_formulate().set("success", false).set("reason", "Missing user_id").to_string(), 400);

	if (!user::does_user_id_exist(user_id))
		return make_response<string_response>(
		    json_formulate().set("success", false).set("reason", "User doesn't exist").to_string(), 400);

	auto response = json_formulate().set("success", true);

	return make_response<string_response>(
	    json_formulate().set("success", true).set("attributes", user::get_user_json(user_id)).to_string(), 400);
}

REGISTER_POST_ENDPOINT("/admin/user/get_attributes", handle_endpoint_admingetuserattributes);