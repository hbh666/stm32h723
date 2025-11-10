find_package(rpl QUIET)

if (NOT rpl_FOUND)
    FetchContent_Declare(
            rpl
            GIT_REPOSITORY https://gitee.com/dlmu-cone/rpl
            GIT_TAG main
    )
    FetchContent_MakeAvailable(rpl)
endif ()