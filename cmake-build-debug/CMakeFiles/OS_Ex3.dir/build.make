# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.8

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/local/jetbrains/clion-2017.2.2/bin/cmake/bin/cmake

# The command to remove a file.
RM = /usr/local/jetbrains/clion-2017.2.2/bin/cmake/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /cs/usr/jherskow/safe/OS_Ex3

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /cs/usr/jherskow/safe/OS_Ex3/cmake-build-debug

# Include any dependencies generated for this target.
include CMakeFiles/OS_Ex3.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/OS_Ex3.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/OS_Ex3.dir/flags.make

CMakeFiles/OS_Ex3.dir/main.cpp.o: CMakeFiles/OS_Ex3.dir/flags.make
CMakeFiles/OS_Ex3.dir/main.cpp.o: ../main.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/cs/usr/jherskow/safe/OS_Ex3/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/OS_Ex3.dir/main.cpp.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/OS_Ex3.dir/main.cpp.o -c /cs/usr/jherskow/safe/OS_Ex3/main.cpp

CMakeFiles/OS_Ex3.dir/main.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/OS_Ex3.dir/main.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /cs/usr/jherskow/safe/OS_Ex3/main.cpp > CMakeFiles/OS_Ex3.dir/main.cpp.i

CMakeFiles/OS_Ex3.dir/main.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/OS_Ex3.dir/main.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /cs/usr/jherskow/safe/OS_Ex3/main.cpp -o CMakeFiles/OS_Ex3.dir/main.cpp.s

CMakeFiles/OS_Ex3.dir/main.cpp.o.requires:

.PHONY : CMakeFiles/OS_Ex3.dir/main.cpp.o.requires

CMakeFiles/OS_Ex3.dir/main.cpp.o.provides: CMakeFiles/OS_Ex3.dir/main.cpp.o.requires
	$(MAKE) -f CMakeFiles/OS_Ex3.dir/build.make CMakeFiles/OS_Ex3.dir/main.cpp.o.provides.build
.PHONY : CMakeFiles/OS_Ex3.dir/main.cpp.o.provides

CMakeFiles/OS_Ex3.dir/main.cpp.o.provides.build: CMakeFiles/OS_Ex3.dir/main.cpp.o


CMakeFiles/OS_Ex3.dir/MapReduceFramework.cpp.o: CMakeFiles/OS_Ex3.dir/flags.make
CMakeFiles/OS_Ex3.dir/MapReduceFramework.cpp.o: ../MapReduceFramework.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/cs/usr/jherskow/safe/OS_Ex3/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/OS_Ex3.dir/MapReduceFramework.cpp.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/OS_Ex3.dir/MapReduceFramework.cpp.o -c /cs/usr/jherskow/safe/OS_Ex3/MapReduceFramework.cpp

CMakeFiles/OS_Ex3.dir/MapReduceFramework.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/OS_Ex3.dir/MapReduceFramework.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /cs/usr/jherskow/safe/OS_Ex3/MapReduceFramework.cpp > CMakeFiles/OS_Ex3.dir/MapReduceFramework.cpp.i

CMakeFiles/OS_Ex3.dir/MapReduceFramework.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/OS_Ex3.dir/MapReduceFramework.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /cs/usr/jherskow/safe/OS_Ex3/MapReduceFramework.cpp -o CMakeFiles/OS_Ex3.dir/MapReduceFramework.cpp.s

CMakeFiles/OS_Ex3.dir/MapReduceFramework.cpp.o.requires:

.PHONY : CMakeFiles/OS_Ex3.dir/MapReduceFramework.cpp.o.requires

CMakeFiles/OS_Ex3.dir/MapReduceFramework.cpp.o.provides: CMakeFiles/OS_Ex3.dir/MapReduceFramework.cpp.o.requires
	$(MAKE) -f CMakeFiles/OS_Ex3.dir/build.make CMakeFiles/OS_Ex3.dir/MapReduceFramework.cpp.o.provides.build
.PHONY : CMakeFiles/OS_Ex3.dir/MapReduceFramework.cpp.o.provides

CMakeFiles/OS_Ex3.dir/MapReduceFramework.cpp.o.provides.build: CMakeFiles/OS_Ex3.dir/MapReduceFramework.cpp.o


# Object files for target OS_Ex3
OS_Ex3_OBJECTS = \
"CMakeFiles/OS_Ex3.dir/main.cpp.o" \
"CMakeFiles/OS_Ex3.dir/MapReduceFramework.cpp.o"

# External object files for target OS_Ex3
OS_Ex3_EXTERNAL_OBJECTS =

OS_Ex3: CMakeFiles/OS_Ex3.dir/main.cpp.o
OS_Ex3: CMakeFiles/OS_Ex3.dir/MapReduceFramework.cpp.o
OS_Ex3: CMakeFiles/OS_Ex3.dir/build.make
OS_Ex3: CMakeFiles/OS_Ex3.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/cs/usr/jherskow/safe/OS_Ex3/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking CXX executable OS_Ex3"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/OS_Ex3.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/OS_Ex3.dir/build: OS_Ex3

.PHONY : CMakeFiles/OS_Ex3.dir/build

CMakeFiles/OS_Ex3.dir/requires: CMakeFiles/OS_Ex3.dir/main.cpp.o.requires
CMakeFiles/OS_Ex3.dir/requires: CMakeFiles/OS_Ex3.dir/MapReduceFramework.cpp.o.requires

.PHONY : CMakeFiles/OS_Ex3.dir/requires

CMakeFiles/OS_Ex3.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/OS_Ex3.dir/cmake_clean.cmake
.PHONY : CMakeFiles/OS_Ex3.dir/clean

CMakeFiles/OS_Ex3.dir/depend:
	cd /cs/usr/jherskow/safe/OS_Ex3/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /cs/usr/jherskow/safe/OS_Ex3 /cs/usr/jherskow/safe/OS_Ex3 /cs/usr/jherskow/safe/OS_Ex3/cmake-build-debug /cs/usr/jherskow/safe/OS_Ex3/cmake-build-debug /cs/usr/jherskow/safe/OS_Ex3/cmake-build-debug/CMakeFiles/OS_Ex3.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/OS_Ex3.dir/depend
