set (daemon_src main.c)

# Make build flags compiler specific
if (CMAKE_COMPILER_IS_GNUCC)
	if (CMAKE_BUILD_TYPE STREQUAL "Debug")
		set (CMAKE_C_FLAGS "-D_REETRANT -ggdb -fPIC -Wall -lwiringPi -Wextra -pedantic -O0")
	elseif( CMAKE_BUILD_TYPE STREQUAL "Release" )
		set (CMAKE_C_FLAGS "-D_REETRANT -DNDEBUG -fPIC -Wall -lwiringPi -Wextra -pedantic -O0")
	endif ()
endif (CMAKE_COMPILER_IS_GNUCC)

add_executable (pithond ${daemon_src})

# The rule to install daemon binary
install (TARGETS pithond
	RUNTIME
	DESTINATION bin)
