/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef PNG2DDS_UTIL_HPP
#define PNG2DDS_UTIL_HPP
#include <cstdint>

namespace png2dds::util {
/**
 * Provides the smallest value divisible by 16 larger than the input.
 * @param value Input value.
 * @return Number larger than input and divisible by 16.
 */
[[nodiscard]] constexpr std::size_t next_divisible_by_16(std::size_t value) noexcept {
	return (value + 0b1111U) & ~0b1111U;
}
} // namespace png2dds::util

#endif // PNG2DDS_UTIL_HPP