option(ENABLE_SANITIZER_ADDRESS "Enable AddressSanitizer" OFF)
option(ENABLE_SANITIZER_UNDEFINED "Enable UndefinedBehaviorSanitizer" OFF)
option(ENABLE_SANITIZER_LEAK "Enable LeakSanitizer" OFF)
option(ENABLE_SANITIZER_THREAD "Enable ThreadSanitizer" OFF)
option(ENABLE_SANITIZER_MEMORY "Enable MemorySanitizer" OFF)

set(SANITIZER_COMPILE_FLAGS "")
set(SANITIZER_LINK_FLAGS "")

if (ENABLE_SANITIZER_ADDRESS)
	list(APPEND SANITIZER_COMPILE_FLAGS "-fsanitize=address")
	list(APPEND SANITIZER_LINK_FLAGS "-fsanitize=address")
endif()

if (ENABLE_SANITIZER_UNDEFINED)
	list(APPEND SANITIZER_COMPILE_FLAGS "-fsanitize=undefined")
	list(APPEND SANITIZER_LINK_FLAGS "-fsanitize=undefined")
endif()

if (ENABLE_SANITIZER_LEAK)
	list(APPEND SANITIZER_COMPILE_FLAGS "-fsanitize=leak")
	list(APPEND SANITIZER_LINK_FLAGS "-fsanitize=leak")
endif()

if (ENABLE_SANITIZER_THREAD)
	list(APPEND SANITIZER_COMPILE_FLAGS "-fsanitize=thread")
	list(APPEND SANITIZER_LINK_FLAGS "-fsanitize=thread")
endif()

if (ENABLE_SANITIZER_MEMORY)
	list(APPEND SANITIZER_COMPILE_FLAGS "-fsanitize=memory" "-fno-omit-frame-pointer")
	list(APPEND SANITIZER_LINK_FLAGS "-fsanitize=memory")
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	if (CMAKE_BUILD_TYPE STREQUAL "Debug")
		add_compile_options(
			"-Wall"
			"-Wextra"
			"-g3")
		if (SANITIZER_COMPILE_FLAGS)
			add_compile_options(${SANITIZER_COMPILE_FLAGS})
		endif()
		if (SANITIZER_LINK_FLAGS)
			add_link_options(${SANITIZER_LINK_FLAGS})
		endif()
	elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
		add_compile_options(
			"-Wall"
			"-Wextra"
			"-Werror"
			"-O3"
			"-flto"
			"-DNDEBUG")
		add_link_options(-flto)
    endif()
endif()
