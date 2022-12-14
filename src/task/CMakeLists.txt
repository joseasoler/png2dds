# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

add_library(png2dds_task STATIC
	include/png2dds/pipeline.hpp
	include/png2dds/task.hpp
	pipeline.cpp
	task.cpp
	)

target_include_directories(png2dds_task PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>/include
	)

target_compile_options(png2dds_task PRIVATE ${PNG2DDS_CPP_WARNING_FLAGS})

target_link_libraries(png2dds_task
	PUBLIC
	png2dds_arguments
	png2dds_format
	PRIVATE
	png2dds_dds
	png2dds_png
	png2dds_regex
	png2dds_util
	bc7enc_dds_defs
	Boost::headers
	Boost::filesystem
	fmt::fmt
	TBB::tbb
	)
