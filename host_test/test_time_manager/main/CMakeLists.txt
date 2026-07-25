idf_component_register(
    SRCS 
        "main.cpp"
        "test_time_manager.cpp"
    
    INCLUDE_DIRS
        "."
        "../../../../idf_hals/mocks"
        
    REQUIRES 
        time_manager
        idf_hals
        gtest
        WHOLE_ARCHIVE
)

if(IDF_TARGET STREQUAL "linux")
    target_compile_options(${COMPONENT_LIB} PUBLIC --coverage)
    target_link_options(${COMPONENT_LIB} PUBLIC --coverage)
endif()
