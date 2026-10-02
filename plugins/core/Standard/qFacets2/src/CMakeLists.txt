
target_sources( ${PROJECT_NAME}
	PRIVATE
		${CMAKE_CURRENT_LIST_DIR}/qFacets2.cpp
		${CMAKE_CURRENT_LIST_DIR}/qFacets2Dialog.cpp
		${CMAKE_CURRENT_LIST_DIR}/qFacets2Process.cpp
)

target_include_directories( ${PROJECT_NAME}
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}
)