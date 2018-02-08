#pragma once

#include <functional>
#include <memory>

template <typename T>
using auto_cleanup = std::unique_ptr<T, std::function<void(T*)>>;

static char dummy[] = "";

#define _DEFER_CAT_(a, b) a##b
#define _DEFER_NAME_(a, b) _DEFER_CAT_(a, b)

#define defer(...) auto _DEFER_NAME_(_defer_, __LINE__) = auto_cleanup<char>(dummy, [&](char*) { __VA_ARGS__; });
