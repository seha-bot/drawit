cmake_minimum_required(VERSION 3.19)
project(tetrisand
	VERSION 0.0.1
	LANGUAGES C CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SDL2 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2_TTF REQUIRED IMPORTED_TARGET SDL2_ttf)
pkg_check_modules(SDL2_IMAGE REQUIRED IMPORTED_TARGET SDL2_image)

include(FetchContent)
FetchContent_Declare(kiss_sdl
	GIT_REPOSITORY https://github.com/actsl/kiss_sdl.git
	GIT_TAG dd292a20728f98f6594cb68dfc62f91c6cd6947b
	SOURCE_DIR kiss_sdl
)
FetchContent_MakeAvailable(kiss_sdl)

add_executable(tetrisand
	${CMAKE_BINARY_DIR}/kiss_sdl/kiss_draw.c
	${CMAKE_BINARY_DIR}/kiss_sdl/kiss_general.c
	${CMAKE_BINARY_DIR}/kiss_sdl/kiss_posix.c
	${CMAKE_BINARY_DIR}/kiss_sdl/kiss_widgets.c

	src/main.cpp
	src/game.cpp
	src/kiss/kiss.cpp
)

target_include_directories(tetrisand PRIVATE
	${SDL2_INCLUDE_DIRS}
	${CMAKE_BINARY_DIR}/kiss_sdl

	src/include
	src/kiss/include
)

set(BASE_FLAGS, -Wall -Wextra -Wshadow -Wunused)

if(debug)
	target_compile_options(tetrisand PRIVATE ${BASE_FLAGS} -g)
else()
	target_compile_options(tetrisand PRIVATE ${BASE_FLAGS} -Werror -O3)
endif()

target_link_libraries(tetrisand PRIVATE
	SDL2::SDL2
	PkgConfig::SDL2_TTF
	PkgConfig::SDL2_IMAGE
)
