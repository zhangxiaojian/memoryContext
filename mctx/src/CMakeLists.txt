set(
	MEMORYCONTEXT_HEADERS
	mctx.h
	MemoryContextData.h
	MemoryContextTool.h
	AllocSetContext.h
	AllocSetType.h
	AllocAnaly.h
	Row.h
	)

set(
	MEMORYCONTEXT_SOURCE
	AllocSetContext.cpp
	MemoryContextData.cpp
	AllocAnaly.cpp
	Row.cpp
	)

set(
	MEMORYCONTEXT_TARGET
	memctx
	)

set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

add_library(
	${MEMORYCONTEXT_TARGET}
	${MEMORYCONTEXT_SOURCE}
	)

install(
	TARGETS
		${MEMORYCONTEXT_TARGET}
	DESTINATION
		${PROJECT_BINARY_DIR}/lib
	)

install(
	FILES
		${MEMORYCONTEXT_HEADERS}
	DESTINATION
		${PROJECT_BINARY_DIR}/include
	)

add_subdirectory(tools)
