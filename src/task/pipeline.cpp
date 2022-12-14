/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "png2dds/pipeline.hpp"

#include "png2dds/dds.hpp"
#include "png2dds/dds_image.hpp"
#include "png2dds/image.hpp"
#include "png2dds/pixel_block_image.hpp"
#include "png2dds/png.hpp"
#include "png2dds/vector.hpp"

#include <boost/nowide/fstream.hpp>
#include <boost/nowide/iostream.hpp>
#include <boost/predef.h>
#include <dds_defs.h>
#include <fmt/format.h>
#include <oneapi/tbb/concurrent_queue.h>
#include <oneapi/tbb/parallel_pipeline.h>

#include <atomic>
#include <chrono>
#include <future>

namespace otbb = oneapi::tbb;

using png2dds::dds_image;
using png2dds::image;
using png2dds::pixel_block_image;
using png2dds::pipeline::paths_vector;

namespace {

constexpr std::size_t error_file_index = std::numeric_limits<std::size_t>::max();

constexpr DDS_HEADER_DXT10 header_extension{DXGI_FORMAT_BC7_UNORM, D3D10_RESOURCE_DIMENSION_TEXTURE2D, 0U, 1U, 0U};

struct png_file {
	png2dds::vector<std::uint8_t> buffer;
	std::size_t file_index;
};

class load_png_file final {
public:
	explicit load_png_file(const paths_vector& paths, std::atomic<std::size_t>& counter,
		otbb::concurrent_queue<std::string>& error_log) noexcept
		: _paths{paths}
		, _counter{counter}
		, _error_log{error_log} {}

	png_file operator()(otbb::flow_control& flow) const {
		std::size_t index = _counter++;
		if (index >= _paths.size()) {
			flow.stop();
			return {};
		}

#if BOOST_OS_WINDOWS
		const boost::filesystem::path input{R"(\\?\)" + _paths[index].first.string()};
#else
		const boost::filesystem::path& input{_paths[index].first};
#endif
		boost::nowide::ifstream ifs{input, std::ios::in | std::ios::binary};

		if (!ifs.is_open()) [[unlikely]] {
			_error_log.push(fmt::format("Load PNG file error in {:s} ", _paths[index].first.string()));
		}

		return {{std::istreambuf_iterator<char>{ifs}, {}}, index};
	}

private:
	const paths_vector& _paths;
	std::atomic<std::size_t>& _counter;
	otbb::concurrent_queue<std::string>& _error_log;
};

class decode_png_image final {
public:
	explicit decode_png_image(const paths_vector& paths, bool vflip, otbb::concurrent_queue<std::string>& error_log,
		bool calculate_alpha) noexcept
		: _paths{paths}
		, _vflip{vflip}
		, _error_log{error_log}
		, _calculate_alpha{calculate_alpha} {}

	image operator()(const png_file& file) const {
		image result{error_file_index, 0U, 0U};

		// If the buffer is empty, assume that load_png_file already reported an error.
		if (!file.buffer.empty()) {
			const std::string& path = _paths[file.file_index].first.string();
			try {
				result = png2dds::png::decode(file.file_index, path, file.buffer, _vflip);
			} catch (const std::runtime_error& exc) {
				_error_log.push(fmt::format("PNG Decoding error {:s} -> {:s}", path, exc.what()));
			}
		}

		if (result.file_index() != error_file_index && _calculate_alpha) [[unlikely]] {
			bool alpha = false;

			for (std::size_t pixel_y = 0UL; !alpha && pixel_y < result.height(); ++pixel_y) {
				for (std::size_t pixel_x = 0UL; pixel_x < result.width(); ++pixel_x) {
					const auto alpha_value = result.get_pixel(pixel_x, pixel_y).back();
					if (alpha_value < 255) {
						alpha = true;
						break;
					}
				}
			}

			if (alpha) { result.set_encode_as_alpha(); }
		}

		return result;
	}

private:
	const paths_vector& _paths;
	bool _vflip;
	otbb::concurrent_queue<std::string>& _error_log;
	bool _calculate_alpha;
};

class get_pixel_blocks final {
public:
	pixel_block_image operator()(const image& image) const { return pixel_block_image{image}; }
};

class encode_bc1_image final {
public:
	explicit encode_bc1_image(unsigned int level) noexcept
		: _level{level} {}

	dds_image operator()(const pixel_block_image& pixel_image) const {
		if (pixel_image.file_index() != error_file_index) [[likely]] {
			return png2dds::dds::bc1_encode(_level, pixel_image);
		}
		return dds_image{};
	}

private:
	unsigned int _level;
};

class encode_bc7_image final {
public:
	explicit encode_bc7_image(unsigned int level) noexcept
		: _params{png2dds::dds::bc7_encode_params(level)} {}

	dds_image operator()(const pixel_block_image& pixel_image) const {
		if (pixel_image.file_index() != error_file_index) [[likely]] {
			return png2dds::dds::bc7_encode(_params, pixel_image);
		}
		return dds_image{};
	}

private:
	png2dds::dds::bc7_params _params;
};

class encode_bc1_alpha_bc7_image final {
public:
	explicit encode_bc1_alpha_bc7_image(unsigned int level) noexcept
		: _params{png2dds::dds::bc7_encode_params(level)} {}

	dds_image operator()(const pixel_block_image& pixel_image) const {
		if (pixel_image.file_index() != error_file_index) [[likely]] {
			return pixel_image.encode_as_alpha() ?
							 png2dds::dds::bc7_encode(_params, pixel_image) :
							 png2dds::dds::bc1_encode(png2dds::format::max_level(png2dds::format::type::bc1), pixel_image);
		}
		return dds_image{};
	}

private:
	png2dds::dds::bc7_params _params;
};

class save_dds_file final {
public:
	explicit save_dds_file(const paths_vector& paths) noexcept
		: _paths{paths} {}

	void operator()(const dds_image& dds_img) const {
		if (dds_img.file_index() == error_file_index) [[unlikely]] { return; }

#if BOOST_OS_WINDOWS
		const boost::filesystem::path output{R"(\\?\)" + _paths[dds_img.file_index()].second.string()};
#else
		const boost::filesystem::path& output{_paths[dds_img.file_index()].second.string()};
#endif

		boost::nowide::ofstream ofs{output, std::ios::out | std::ios::binary};

		ofs << "DDS ";
		const std::size_t block_size_bytes = dds_img.blocks().size() * sizeof(std::uint64_t);
		const auto header = dds_img.header();
		ofs.write(header.data(), header.size());
		if (dds_img.format() == png2dds::format::type::bc7) {
			ofs.write(reinterpret_cast<const char*>(&header_extension), sizeof(header_extension));
		}

		ofs.write(reinterpret_cast<const char*>(dds_img.blocks().data()), static_cast<std::ptrdiff_t>(block_size_bytes));
		ofs.close();
	}

private:
	const paths_vector& _paths;
};

void error_reporting(
	std::atomic<std::size_t>& progress, std::size_t total, otbb::concurrent_queue<std::string>& error_log) {
	using namespace std::chrono_literals;
	std::size_t last_progress{};
	std::string error_str;
	bool requires_newline = false;

	while (progress < total) {
		while (error_log.try_pop(error_str)) {
			if (requires_newline) {
				boost::nowide::cerr << '\n';
				requires_newline = false;
			}
			boost::nowide::cerr << error_str << '\n';
		}
		const std::size_t current_progress = progress;

		if (current_progress > last_progress && current_progress < total) {
			last_progress = current_progress;
			boost::nowide::cout << fmt::format("\rProgress: {:d}/{:d}", current_progress, total);
			boost::nowide::cout.flush();
			requires_newline = true;
			std::this_thread::sleep_for(50ms);
		}
	}
}

otbb::filter<pixel_block_image, dds_image> encoding_filter(png2dds::format::type format_type, unsigned int level) {
	switch (format_type) {
	case png2dds::format::type::bc1:
		return otbb::make_filter<pixel_block_image, dds_image>(otbb::filter_mode::parallel, encode_bc1_image{level});
	case png2dds::format::type::bc7:
		return otbb::make_filter<pixel_block_image, dds_image>(otbb::filter_mode::parallel, encode_bc7_image{level});
	case png2dds::format::type::bc1_alpha_bc7:
		return otbb::make_filter<pixel_block_image, dds_image>(
			otbb::filter_mode::parallel, encode_bc1_alpha_bc7_image{level});
	}
	assert(false);
	return {};
}

} // Anonymous namespace

namespace png2dds::pipeline {

void encode_as_dds(std::size_t tokens, const args::data& arguments, const paths_vector& paths) {
	// Variables referenced by the filters.
	std::atomic<std::size_t> counter;
	otbb::concurrent_queue<std::string> error_log;

	std::future<void> error_report{};
	if (arguments.verbose) {
		error_report =
			std::async(std::launch::async, error_reporting, std::ref(counter), paths.size(), std::ref(error_log));
	}

	const otbb::filter<void, void> filters =
		otbb::make_filter<void, png_file>(otbb::filter_mode::serial_in_order, load_png_file(paths, counter, error_log)) &
		otbb::make_filter<png_file, image>(otbb::filter_mode::parallel,
			decode_png_image(paths, arguments.vflip, error_log, arguments.format == format::type::bc1_alpha_bc7)) &
		otbb::make_filter<image, pixel_block_image>(otbb::filter_mode::parallel, get_pixel_blocks{}) &
		encoding_filter(arguments.format, arguments.level) &
		otbb::make_filter<dds_image, void>(otbb::filter_mode::parallel, save_dds_file{paths});

	otbb::parallel_pipeline(tokens, filters);

	if (error_report.valid()) {
		// Wait until the error report task is done before finishing the error log report.
		error_report.get();
		std::string error_str;
		while (error_log.try_pop(error_str)) { boost::nowide::cerr << error_str << '\n'; }
		boost::nowide::cerr.flush();
		boost::nowide::cout << fmt::format("\rProgress: {:d}/{:d}\n", paths.size(), paths.size());
		boost::nowide::cout.flush();
	}
}

} // namespace png2dds::pipeline
