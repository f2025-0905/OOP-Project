cmake_minimum_required(VERSION 3.16)
project(ShiftWiseBackend VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED libpqxx)

# Crow (header-only, fetched via FetchContent)
include(FetchContent)

FetchContent_Declare(
  Crow
  GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
  GIT_TAG        v1.0+5
)
FetchContent_MakeAvailable(Crow)

FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

FetchContent_Declare(
  jwt-cpp
  GIT_REPOSITORY https://github.com/Thalhammer/jwt-cpp.git
  GIT_TAG        v0.7.0
)
FetchContent_MakeAvailable(jwt-cpp)

FetchContent_Declare(
  bcrypt
  GIT_REPOSITORY https://github.com/hilch/Bcrypt.cpp.git
  GIT_TAG        main
)
FetchContent_MakeAvailable(bcrypt)

# Source files
set(SOURCES
    src/main.cpp
    src/controllers/AuthController.cpp
    src/controllers/StaffController.cpp
    src/controllers/ScheduleController.cpp
    src/controllers/LeaveController.cpp
    src/controllers/DashboardController.cpp
    src/middleware/AuthMiddleware.cpp
)

add_executable(shiftwise ${SOURCES})

target_include_directories(shiftwise PRIVATE
    include
    ${PQXX_INCLUDE_DIRS}
)

target_link_libraries(shiftwise PRIVATE
    Crow::Crow
    nlohmann_json::nlohmann_json
    jwt-cpp::jwt-cpp
    ${PQXX_LIBRARIES}
)

target_compile_options(shiftwise PRIVATE ${PQXX_CFLAGS_OTHER})

# Optimization for production
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(shiftwise PRIVATE -O2)
endif()
