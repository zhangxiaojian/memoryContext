
set(
	MEMORYCONTEXT_SOURCE
	main.cpp
	Time.cpp
	)

add_executable(
	mctx
	${MEMORYCONTEXT_SOURCE}
	)

target_link_libraries(
	mctx
	${MEMORYCONTEXT_TARGET}
	)

install(
	TARGETS
		mctx
	DESTINATION
		${PROJECT_BINARY_DIR}	
	)
