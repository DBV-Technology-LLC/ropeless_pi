# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/colin/ropeless_pi

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/colin/ropeless_pi/build

# Utility rule file for android-finish.

# Include any custom commands dependencies for this target.
include CMakeFiles/android-finish.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/android-finish.dir/progress.make

android-finish: CMakeFiles/android-finish.dir/build.make
	cmake -P /home/colin/ropeless_pi/build/finish_tarball.cmake
.PHONY : android-finish

# Rule to build all files generated by this target.
CMakeFiles/android-finish.dir/build: android-finish
.PHONY : CMakeFiles/android-finish.dir/build

CMakeFiles/android-finish.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/android-finish.dir/cmake_clean.cmake
.PHONY : CMakeFiles/android-finish.dir/clean

CMakeFiles/android-finish.dir/depend:
	cd /home/colin/ropeless_pi/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/colin/ropeless_pi /home/colin/ropeless_pi /home/colin/ropeless_pi/build /home/colin/ropeless_pi/build /home/colin/ropeless_pi/build/CMakeFiles/android-finish.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/android-finish.dir/depend

