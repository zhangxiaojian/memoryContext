set(
	MEMORYCONTEXT_HEADERS
	mctx.h
	MemoryContextData.h
	MemoryContextTool.h
	AllocSetContext.h
	AllocSetType.h
	)

set(
	MEMORYCONTEXT_SOURCE
	AllocSetContext.cpp
	MemoryContextData.cpp
	)

set(
	MEMORYCONTEXT_TARGET
	memctx
	)

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
