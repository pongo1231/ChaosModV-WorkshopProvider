#pragma once

#include <httpserver.hpp>

#include <assert.h>
#include <functional>
#include <memory>
#include <string_view>
#include <unordered_map>

using namespace httpserver;

#define _ENDPOINT_CONCAT(a, b) a##b
#define ENDPOINT_CONCAT(a, b) _ENDPOINT_CONCAT(a, b)
#define REGISTER_GET_ENDPOINT(endpoint_name, func)                                                \
	namespace                                                                                     \
	{                                                                                             \
		endpoint::get_endpoint_handler ENDPOINT_CONCAT(endpoint_, __LINE__)(endpoint_name, func); \
	}
#define REGISTER_POST_ENDPOINT(endpoint_name, func)                                                \
	namespace                                                                                      \
	{                                                                                              \
		endpoint::post_endpoint_handler ENDPOINT_CONCAT(endpoint_, __LINE__)(endpoint_name, func); \
	}

namespace endpoint
{
	struct endpoint_data;

	inline std::unordered_map<std::string_view, std::function<std::shared_ptr<http_response>(const http_request &)>>
	    get_endpoint_handlers;
	inline std::unordered_map<std::string_view, std::function<std::shared_ptr<http_response>(const http_request &)>>
	    post_endpoint_handlers;

	struct get_endpoint_handler
	{
		get_endpoint_handler(std::string_view endpoint,
		                     std::function<std::shared_ptr<http_response>(const http_request &)> func)
		{
			assert(!get_endpoint_handlers.contains(endpoint));
			get_endpoint_handlers[endpoint] = func;
		}
	};

	struct post_endpoint_handler
	{
		post_endpoint_handler(std::string_view endpoint,
		                      std::function<std::shared_ptr<http_response>(const http_request &)> func)
		{
			assert(!post_endpoint_handlers.contains(endpoint));
			post_endpoint_handlers[endpoint] = func;
		}
	};
}