include(FetchContent)

FetchContent_Declare(googletest
   URL "https://github.com/google/googletest/archive/refs/tags/release-1.11.0.zip"
   URL_HASH SHA384=93d8ce0bc1ab360c6ed1ef424cf8d2828b4911e9816f0b2eb7adabae0065204bc5e4115e4b5e361898ec8f052bfeced5
)

FetchContent_MakeAvailable(googletest)
