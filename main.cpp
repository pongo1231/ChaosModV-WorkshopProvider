#include "endpoints/endpoint_common.h"
#include "json_response.h"
#include "options.h"

#include <httpserver.hpp>

#include <csignal>
#include <memory>

using namespace httpserver;

class chaosworkshop : public http_resource
{
  public:
	chaosworkshop()
	{
	}

	const std::shared_ptr<http_response> render_GET(const http_request &request) override
	{
		const auto &endpoint = request.get_path();

		if (endpoint::get_endpoint_handlers.contains(endpoint))
			return endpoint::get_endpoint_handlers.at(endpoint)(request);

		return make_response<http_response>(404, http::http_utils::text_plain);
	}

	const std::shared_ptr<http_response> render_POST(const http_request &request) override
	{
		const auto &endpoint = request.get_path();

		if (endpoint::post_endpoint_handlers.contains(endpoint))
			return endpoint::post_endpoint_handlers.at(endpoint)(request);

		return make_response<http_response>(404, http::http_utils::text_plain);
	}
};

int main()
{
	// reap children automatically
	signal(SIGCHLD, SIG_IGN);

	global_options.read();

	webserver server = create_webserver(global_options.port)
	                       .use_ssl()
	                       .https_mem_cert("cert.pem")
	                       .https_mem_key("key.pem")
	                       .content_size_limit(global_options.submission_max_total_size * 2.f)
	                       .connection_timeout(global_options.connection_timeout);

	chaosworkshop workshop;
	server.register_resource("/", &workshop, true);
	server.start(true);
}