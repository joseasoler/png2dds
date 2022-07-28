/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef PNG2DDS_FORMAT_HPP
#define PNG2DDS_FORMAT_HPP

#include <string_view>

namespace png2dds::format {

enum class type : std::uint8_t { bc1, bc7 };

[[nodiscard]] constexpr std::string_view name(type fmt) noexcept {
	std::string_view name_str{};
	switch (fmt) {
	case type::bc1: name_str = "BC1"; break;
	case type::bc7: name_str = "BC7"; break;
	}
	return name_str;
}

[[nodiscard]] constexpr unsigned int max_level(type fmt) noexcept {
	unsigned int maximum{};
	switch (fmt) {
	case type::bc1: maximum = 18U; break;
	case type::bc7: maximum = 6U; break;
	}
	return maximum;
}

} // namespace png2dds::format

#endif // PNG2DDS_FORMAT_HPP
