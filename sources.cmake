
dplx_target_sources(deeppack
    TEST_TARGET deeppack-tests
    MODE SMART_SOURCE MERGED_LAYOUT
    BASE_DIR dplx
    
    PUBLIC
        dp/core
        dp/disappointment
)

dplx_target_sources(deeppack
    TEST_TARGET deeppack-tests
    MODE SMART_HEADER_ONLY MERGED_LAYOUT
    BASE_DIR dplx
    
    PUBLIC
        dp/items/emit
        dp/items/emit_context
        dp/streams/memory_output_stream2
        dp/streams/output_buffer
)

dplx_target_sources(deeppack
    TEST_TARGET deeppack-legacy-tests
    MODE SMART_SOURCE MERGED_LAYOUT
    BASE_DIR dplx
    
    PUBLIC
        dp
)

dplx_target_sources(deeppack
    TEST_TARGET deeppack-legacy-tests
    MODE SMART_HEADER_ONLY MERGED_LAYOUT
    BASE_DIR dplx

    PUBLIC
        dp/config

        dp/concepts
        dp/fwd
        dp/layout_descriptor
        dp/memory_buffer
        dp/object_def
        dp/stream
        dp/tuple_def
        dp/type_code

        dp/detail/item_size
        dp/encoder/api
        dp/encoder/arg_list
        dp/encoder/chrono
        dp/encoder/core
        dp/encoder/narrow_strings
        dp/encoder/object_utils
        dp/encoder/std_path
        dp/encoder/tuple_utils
        dp/item_emitter

        dp/decoder/api
        dp/decoder/chrono
        dp/decoder/core
        dp/decoder/object_utils
        dp/decoder/std_container
        dp/decoder/std_path
        dp/decoder/std_string
        dp/decoder/tuple_utils
        dp/decoder/utils
        dp/detail/parse_item
        dp/item_parser
        dp/skip_item

        dp/customization
        dp/customization.std
        dp/indefinite_range
        dp/map_pair

        dp/streams/chunked_input_stream
        dp/streams/chunked_output_stream
        dp/streams/memory_input_stream
        dp/streams/memory_output_stream

        dp/detail/bit
        dp/detail/hash
        dp/detail/mp_for_dots
        dp/detail/perfect_hash
        dp/detail/type_utils
        dp/detail/utils
        dp/detail/workaround
)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/generated/src/dplx/dp/detail)
configure_file(tools/config.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/generated/src/dplx/dp/detail/config.hpp @ONLY)
target_sources(deeppack PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated/src/dplx/dp/detail/config.hpp>)

if (BUILD_TESTING)
    dplx_target_sources(deeppack-tests PRIVATE
        MODE VERBATIM
        BASE_DIR dp_tests-ng

        PRIVATE
            item_sample.hpp
            range_generator.hpp
            test_utils.hpp
    )

    dplx_target_sources(deeppack-legacy-tests PRIVATE
        MODE VERBATIM
        BASE_DIR dp_tests

        PRIVATE
            test_utils.hpp
    )

    target_sources(deeppack-legacy-tests PRIVATE
        tests/encoder.blob.test.cpp
        tests/encoder.map.test.cpp
        tests/encoder.range.test.cpp
        tests/encoder.string.test.cpp
        tests/encoder.tuple.test.cpp

        tests/enum_codec.test.cpp

        tests/item_emitter.integer.test.cpp
        tests/item_parser.array.test.cpp
        tests/item_parser.binary.test.cpp
        tests/item_parser.expect.test.cpp
        tests/item_parser.integer.test.cpp
    )
endif ()
