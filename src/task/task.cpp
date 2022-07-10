/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "png2dds/task.hpp"

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/filesystem.hpp>
#include <boost/nowide/filesystem.hpp>
#include <boost/nowide/fstream.hpp>
#include <oneapi/tbb/global_control.h>

#include <algorithm>
#include <string>
#include <string_view>

#include "pipeline.hpp"

namespace fs = boost::filesystem;
namespace otbb = oneapi::tbb;
using png2dds::pipeline::paths_vector;

namespace {

bool try_add_file(const fs::path& png_path, const fs::file_status& status, paths_vector& paths, bool overwrite) {
	if (!fs::is_regular_file(status)) { return false; }
	const std::string extension = boost::to_lower_copy(png_path.extension().string());
	constexpr std::string_view png_extension{".png"};
	if (boost::to_lower_copy(png_path.extension().string()) != png_extension) { return false; }
	constexpr std::string_view dds_extension{".dds"};
	fs::path dds_path{png_path};
	dds_path.replace_extension(dds_extension.data());
	if (overwrite || !fs::exists(dds_path)) { paths.emplace_back(png_path, dds_path); }
	return true;
}

void process_directory(const fs::path& path, paths_vector& paths, bool overwrite, unsigned int depth) {
	const fs::directory_entry dir{path};
	if (!fs::exists(dir) || !fs::is_directory(dir)) { return; }

	for (fs::recursive_directory_iterator itr{dir}; itr != fs::recursive_directory_iterator{}; ++itr) {
		try_add_file(itr->path(), itr->status(), paths, overwrite);
		if (static_cast<unsigned int>(itr.depth()) >= depth) { itr.disable_recursion_pending(); }
	}
}

} // anonymous namespace

namespace png2dds {

task::task(const args::data& arguments) {
	// Use UTF-8 as the default encoding for Boost.Filesystem.
	boost::nowide::nowide_filesystem();

	paths_vector paths;
	if (!try_add_file(arguments.path, fs::status(arguments.path), paths, arguments.overwrite)) {
		process_directory(arguments.path, paths, arguments.overwrite, arguments.depth);
	}

	if (fs::exists(arguments.list) && fs::is_regular_file(arguments.list)) {
		boost::nowide::fstream stream{arguments.list};
		std::string buffer;
		while (std::getline(stream, buffer)) {
			fs::path path{buffer};

			if (!try_add_file(path, fs::status(path), paths, arguments.overwrite)) {
				process_directory(path, paths, arguments.overwrite, arguments.depth);
			}
		}
	}

	// Process the list in order ignoring duplicates.
	std::sort(paths.begin(), paths.end());
	paths.erase(std::unique(paths.begin(), paths.end()), paths.end());

	if (paths.empty()) { return; }

	// Configure the maximum parallelism allowed for tbb.
	otbb::global_control control(otbb::global_control::max_allowed_parallelism, arguments.threads);
	const std::size_t num_tokens = arguments.threads * 4UL;

	pipeline::encode_as_dds(num_tokens, arguments.level, arguments.flip, paths);
}

} // namespace png2dds
