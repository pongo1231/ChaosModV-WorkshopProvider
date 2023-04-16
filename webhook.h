#pragma once

#include "options.h"

#include <dpp/dpp.h>

#include <string>
#include <vector>

namespace webhook
{
	inline dpp::cluster &get_bot()
	{
		static dpp::cluster bot("");
		return bot;
	}

	inline dpp::webhook &get_webhook()
	{
		static dpp::webhook webhook(global_options.webhook_url);
		return webhook;
	}

	struct webhook_options
	{
		std::string author;
		std::string title;
		std::vector<std::pair<std::string, std::string>> fields;
		std::string description;
		std::string footer;
	};
	inline void send(webhook_options options)
	{
		if (global_options.webhook_url.empty())
			return;

		try
		{
			dpp::embed embed;
			embed.set_title(options.title);
			for (const auto &[key, value] : options.fields)
				embed.add_field(key, value);
			embed.set_description(options.description);
			embed.set_footer(dpp::embed_footer { .text = options.footer });
			embed.set_author(dpp::embed_author { .name = options.author });
			embed.set_thumbnail("https://gopong.dev/chaos/chaos.png");

			dpp::message msg;
			msg.add_embed(embed);

			get_bot().execute_webhook(get_webhook(), msg);
		}
		catch (dpp::exception)
		{
		}
	}
}