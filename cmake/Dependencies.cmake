function(steambridge_prepare_dependencies)
    if(NOT STEAMSDK_SOURCE_PATH)
        include(FetchContent)
        FetchContent_Declare(steambridge_sdk
            GIT_REPOSITORY https://github.com/MetaHookSv/SteamSDK
            GIT_TAG ba2bfcc008b8aa197aba05e6122a15d6d236b80b
            GIT_SUBMODULES "" SOURCE_SUBDIR _source_only)
        FetchContent_MakeAvailable(steambridge_sdk)
        set(STEAMSDK_SOURCE_PATH "${steambridge_sdk_SOURCE_DIR}")
    endif()
    if(NOT EXISTS "${STEAMSDK_SOURCE_PATH}/steam/steam_api.h")
        message(FATAL_ERROR "STEAMSDK_SOURCE_PATH must contain steam/steam_api.h")
    endif()
    set(STEAMSDK_SOURCE_PATH "${STEAMSDK_SOURCE_PATH}" PARENT_SCOPE)
endfunction()
