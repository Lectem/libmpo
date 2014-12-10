set(EXECUTABLE_OUTPUT_PATH ./)

file(
        GLOB_RECURSE
        example_source_files
        *.c
)

add_executable(libmpo_example ${example_source_files})
target_link_libraries(libmpo_example mpo_static jpeg)
file(GLOB MPOFILES
  "*.mpo"
)
file(COPY ${MPOFILES} DESTINATION ${EXECUTABLE_OUTPUT_PATH})