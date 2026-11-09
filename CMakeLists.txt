cmake_minimum_required(VERSION 3.21)

project(vkeybd-qt LANGUAGES CXX)

# C++20: required by libremidi
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Qt's moc and rcc replace qmake's implicit Qt code-generation rules.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

include(CTest)

find_package(Qt6 6.2 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Svg
    SvgWidgets
    Network
    DBus
)

# Optional MIDI backend "libremidi" (-o libremidi). An installed libremidi is
# used if CMake finds one, otherwise the pinned release is downloaded.
option(VKEYBD_WITH_LIBREMIDI "Build the libremidi MIDI backend" ON)
if(VKEYBD_WITH_LIBREMIDI)
    find_package(libremidi CONFIG QUIET)
    if(NOT libremidi_FOUND)
        include(FetchContent)
        set(LIBREMIDI_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(LIBREMIDI_TESTS OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(libremidi
            GIT_REPOSITORY https://github.com/celtera/libremidi
            GIT_TAG        v5.3.1
            GIT_SHALLOW    TRUE
        )
        FetchContent_MakeAvailable(libremidi)
        
        # Fix a libremidi bug: check_client_released() has an inverted condition
        # and releases a std::binary_semaphore on every JACK cycle (aborts with
        # libstdc++ assertions, undefined behaviour without). The fix is applied
        # to the downloaded source before it is compiled.
        set(VKEYBD_LIBREMIDI_SEMAPHORE "${libremidi_SOURCE_DIR}/include/libremidi/detail/semaphore.hpp")
        file(READ "${VKEYBD_LIBREMIDI_SEMAPHORE}" VKEYBD_SEMAPHORE_TEXT)
        string(FIND "${VKEYBD_SEMAPHORE_TEXT}" "if (!this->sem_needpost.try_acquire())" VKEYBD_SEMAPHORE_BUG)
        if(NOT VKEYBD_SEMAPHORE_BUG EQUAL -1)
            string(REPLACE "if (!this->sem_needpost.try_acquire())" "if (this->sem_needpost.try_acquire())" VKEYBD_SEMAPHORE_TEXT "${VKEYBD_SEMAPHORE_TEXT}")
            file(WRITE "${VKEYBD_LIBREMIDI_SEMAPHORE}" "${VKEYBD_SEMAPHORE_TEXT}")
            message(STATUS "libremidi: patched semaphore_pair_lock::check_client_released()")
        endif()
    endif()
endif()

add_subdirectory(src)

if(BUILD_TESTING)
    find_package(Qt6 6.2 REQUIRED COMPONENTS Test)
    add_subdirectory(tests)
endif()
