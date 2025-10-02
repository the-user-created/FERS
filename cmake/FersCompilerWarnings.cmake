# This module defines a function to apply consistent compiler warnings and flags
# across all project targets.

function(apply_fers_warnings TARGET_NAME)
	# Find the Threads package to enable the Threads::Threads target.
	find_package(Threads REQUIRED)

	# --- Common Warnings for GCC and Clang ---
	set(FERS_GCC_CLANG_WARNINGS
		-Wall
		-Wextra
		-Wshadow
		-Wnon-virtual-dtor
		-Wold-style-cast
		-Wcast-align
		-Wunused
		-Woverloaded-virtual
		-Wpedantic
		-Wconversion
		-Wsign-conversion
		-Wnull-dereference
		-Wdouble-promotion
		-Wformat=2
		-Wmisleading-indentation
		-Wduplicated-cond
		-Wduplicated-branches
		-Wlogical-op
	)

	# --- Apply Flags Based on Compiler ---
	if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
		target_compile_options(${TARGET_NAME} PRIVATE ${FERS_GCC_CLANG_WARNINGS})
	elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
		# Add equivalent MSVC warnings if needed, e.g., /W4
		target_compile_options(${TARGET_NAME} PRIVATE /W4)
	endif()

	# --- Common Flags ---
	# Use generator expressions to apply flags conditionally.
	target_compile_options(${TARGET_NAME} PRIVATE
		# Enable threading support
		$<$<CXX_COMPILER_ID:GNU>:-pthread>
		# Performance-related math flags
		-ffast-math
		-fno-finite-math-only
	)
	target_link_libraries(${TARGET_NAME} PRIVATE
		$<$<CXX_COMPILER_ID:GNU>:Threads::Threads>
	)
	target_compile_definitions(${TARGET_NAME} PRIVATE _REENTRANT)

	# --- Build-Type Specific Flags ---
	# Debug flags
	target_compile_options(${TARGET_NAME} PRIVATE
		$<$<CONFIG:Debug>:-g>
		$<$<CONFIG:Debug>:-O0>
		$<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:GNU>>:-fprofile-arcs;-ftest-coverage>
	)

	# Release flags
	target_compile_options(${TARGET_NAME} PRIVATE $<$<CONFIG:Release>:-O2>)
	target_link_options(${TARGET_NAME} PRIVATE $<$<CONFIG:Release>:-s>)

endfunction()
