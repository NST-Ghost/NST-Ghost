include(FetchContent)

FetchContent_Declare(
    unrpyc_rs
    GIT_REPOSITORY https://github.com/your-username/unrpyc-rs.git
    GIT_TAG        v0.1.0 # Or use a branch name like 'main'
)

FetchContent_MakeAvailable(unrpyc_rs)

# This would allow NST to link against unrpyc_rs if needed,
# and it will build from source using the local environment's Cargo.
