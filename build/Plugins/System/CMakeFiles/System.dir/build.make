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
CMAKE_SOURCE_DIR = /home/tzupan/gekengine

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/tzupan/gekengine/build

# Include any dependencies generated for this target.
include Plugins/System/CMakeFiles/System.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include Plugins/System/CMakeFiles/System.dir/compiler_depend.make

# Include the progress variables for this target.
include Plugins/System/CMakeFiles/System.dir/progress.make

# Include the compile flags for this target's objects.
include Plugins/System/CMakeFiles/System.dir/flags.make

Plugins/System/CMakeFiles/System.dir/VideoDevice.cpp.o: Plugins/System/CMakeFiles/System.dir/flags.make
Plugins/System/CMakeFiles/System.dir/VideoDevice.cpp.o: ../Plugins/System/VideoDevice.cpp
Plugins/System/CMakeFiles/System.dir/VideoDevice.cpp.o: Plugins/System/CMakeFiles/System.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/tzupan/gekengine/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object Plugins/System/CMakeFiles/System.dir/VideoDevice.cpp.o"
	cd /home/tzupan/gekengine/build/Plugins/System && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT Plugins/System/CMakeFiles/System.dir/VideoDevice.cpp.o -MF CMakeFiles/System.dir/VideoDevice.cpp.o.d -o CMakeFiles/System.dir/VideoDevice.cpp.o -c /home/tzupan/gekengine/Plugins/System/VideoDevice.cpp

Plugins/System/CMakeFiles/System.dir/VideoDevice.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/System.dir/VideoDevice.cpp.i"
	cd /home/tzupan/gekengine/build/Plugins/System && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/tzupan/gekengine/Plugins/System/VideoDevice.cpp > CMakeFiles/System.dir/VideoDevice.cpp.i

Plugins/System/CMakeFiles/System.dir/VideoDevice.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/System.dir/VideoDevice.cpp.s"
	cd /home/tzupan/gekengine/build/Plugins/System && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/tzupan/gekengine/Plugins/System/VideoDevice.cpp -o CMakeFiles/System.dir/VideoDevice.cpp.s

Plugins/System/CMakeFiles/System.dir/entrypoints.cpp.o: Plugins/System/CMakeFiles/System.dir/flags.make
Plugins/System/CMakeFiles/System.dir/entrypoints.cpp.o: ../Plugins/System/entrypoints.cpp
Plugins/System/CMakeFiles/System.dir/entrypoints.cpp.o: Plugins/System/CMakeFiles/System.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/tzupan/gekengine/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object Plugins/System/CMakeFiles/System.dir/entrypoints.cpp.o"
	cd /home/tzupan/gekengine/build/Plugins/System && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT Plugins/System/CMakeFiles/System.dir/entrypoints.cpp.o -MF CMakeFiles/System.dir/entrypoints.cpp.o.d -o CMakeFiles/System.dir/entrypoints.cpp.o -c /home/tzupan/gekengine/Plugins/System/entrypoints.cpp

Plugins/System/CMakeFiles/System.dir/entrypoints.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/System.dir/entrypoints.cpp.i"
	cd /home/tzupan/gekengine/build/Plugins/System && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/tzupan/gekengine/Plugins/System/entrypoints.cpp > CMakeFiles/System.dir/entrypoints.cpp.i

Plugins/System/CMakeFiles/System.dir/entrypoints.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/System.dir/entrypoints.cpp.s"
	cd /home/tzupan/gekengine/build/Plugins/System && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/tzupan/gekengine/Plugins/System/entrypoints.cpp -o CMakeFiles/System.dir/entrypoints.cpp.s

# Object files for target System
System_OBJECTS = \
"CMakeFiles/System.dir/VideoDevice.cpp.o" \
"CMakeFiles/System.dir/entrypoints.cpp.o"

# External object files for target System
System_EXTERNAL_OBJECTS =

../bin/libSystem.so: Plugins/System/CMakeFiles/System.dir/VideoDevice.cpp.o
../bin/libSystem.so: Plugins/System/CMakeFiles/System.dir/entrypoints.cpp.o
../bin/libSystem.so: Plugins/System/CMakeFiles/System.dir/build.make
../bin/libSystem.so: ../bin/libMath.a
../bin/libSystem.so: ../bin/libUtility.a
../bin/libSystem.so: ../bin/libMath.a
../bin/libSystem.so: gnu_11.3_cxx20_64_release/libtbb.so.12.10
../bin/libSystem.so: ../bin/libfmt.so.9.1.1
../bin/libSystem.so: Plugins/System/CMakeFiles/System.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/tzupan/gekengine/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking CXX shared library ../../../bin/libSystem.so"
	cd /home/tzupan/gekengine/build/Plugins/System && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/System.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
Plugins/System/CMakeFiles/System.dir/build: ../bin/libSystem.so
.PHONY : Plugins/System/CMakeFiles/System.dir/build

Plugins/System/CMakeFiles/System.dir/clean:
	cd /home/tzupan/gekengine/build/Plugins/System && $(CMAKE_COMMAND) -P CMakeFiles/System.dir/cmake_clean.cmake
.PHONY : Plugins/System/CMakeFiles/System.dir/clean

Plugins/System/CMakeFiles/System.dir/depend:
	cd /home/tzupan/gekengine/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/tzupan/gekengine /home/tzupan/gekengine/Plugins/System /home/tzupan/gekengine/build /home/tzupan/gekengine/build/Plugins/System /home/tzupan/gekengine/build/Plugins/System/CMakeFiles/System.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : Plugins/System/CMakeFiles/System.dir/depend

