#pragma once

#include "file.h"
#include "options.h"

#include <string>
#include <string_view>
#include <vector>

namespace html_file
{
	inline std::string
	read_file(const std::string &filename,
	          std::vector<std::pair<std::string_view, std::string_view>> additional_replacements = {})
	{
		auto file             = file::read_file(filename);
		auto find_and_replace = [&](std::string_view to_find, std::string_view replacement)
		{
			while (true)
			{
				auto index = file.find(to_find);
				if (index == file.npos)
					return;

				file = file.replace(index, to_find.size(), replacement);
			}
		};

		find_and_replace("$$domain$$", global_options.domain);

		for (const auto &replacement : additional_replacements)
			find_and_replace(replacement.first, replacement.second);

		return file;
	}
}