
dplx_target_sources(deeppack
    TEST_TARGET deeppack-tests
    MODE SMART_SOURCE MERGED_LAYOUT
    BASE_DIR dplx

    PUBLIC
        dp/disappointment

        dp/codecs/core
        dp/codecs/fixed_u8string
        dp/codecs/std-chrono
        dp/codecs/std-filesystem
        dp/codecs/std-string

        dp/items/skip_item
)

dplx_target_sources(deeppack
    TEST_TARGET deeppack-tests
    MODE SMART_HEADER_ONLY MERGED_LAYOUT
    BASE_DIR dplx

    PUBLIC
        dp/api
        dp/indefinite_range
        dp/map_pair

        dp/codecs/auto_enum
        dp/codecs/auto_object
        dp/codecs/auto_tuple
        dp/codecs/std-container
        dp/codecs/std-tuple

        dp/cpos/property_id_hash

        dp/detail/item_size
        dp/detail/perfect_hash
        dp/detail/type_utils
        dp/detail/utils

        dp/items
        dp/items/emit_context
        dp/items/emit_core
        dp/items/emit_ranges
        dp/items/encoded_item_head_size
        dp/items/item_size_of_core
        dp/items/item_size_of_ranges
        dp/items/parse_context
        dp/items/parse_core
        dp/items/parse_ranges
        dp/items/type_code

        dp/streams/input_buffer
        dp/streams/memory_output_stream2
        dp/streams/output_buffer
        dp/streams/void_stream
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
        dp/tuple_def

        dp/customization
        dp/customization.std

        dp/legacy/chunked_input_stream
        dp/legacy/chunked_output_stream
        dp/legacy/memory_input_stream
        dp/legacy/stream

        dp/detail/bit
        dp/detail/hash
        dp/detail/mp_for_dots
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
            blob_matcher.hpp
            core_samples.hpp
            item_sample_ct.hpp
            item_sample_rt.hpp
            range_generator.hpp
            simple_encodable.hpp
            test_input_stream.hpp
            test_output_stream.hpp
            test_utils.hpp
            yaml_sample_generator.hpp
    )

    dplx_target_data(deeppack-tests
        SOURCE_DIR test-samples

        FILES
            arrays.yaml
            blobs.yaml
            maps.yaml
            text.yaml
    )

    dplx_target_sources(deeppack-legacy-tests PRIVATE
        MODE VERBATIM
        BASE_DIR dp_tests

        PRIVATE
            test_utils.hpp
    )
endif ()
