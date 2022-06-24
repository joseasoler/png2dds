/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef PNG2DDS_ARGUMENTS_HPP
#define PNG2DDS_ARGUMENTS_HPP

#include <string_view>
#include <string>

namespace png2dds::args {

struct data {
	std::string error;
	std::string path;
	std::string only;
	int threads;
};

/**
 * Obtain argument data from command-line data
 * @param argc Number of arguments.
 * @param argv Array of arguments
 * @return Structure containing parsed arguments.
 */
data get(int argc, char** argv);
} // namespace png2dds::args

#endif // PNG2DDS_ARGUMENTS_HPP