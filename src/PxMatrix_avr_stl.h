#ifndef __PxMATRIX_AVR_STL_H
#define __PxMATRIX_AVR_STL_H

/*
 * This header provide minimal requirements for c++11 and gnu compilers
 * to build source code with usage of initializer_list and move semantics.
 */

#pragma GCC system_header

#if __cplusplus < 201103L
#error This file requires compiler and library support \
for the ISO C++ 2011 standard. This support must be enabled \
with the -std=c++11 or -std=gnu++11 compiler options.
#else

#pragma GCC visibility push(default)

// <initializer_list>
namespace std {
    template<typename _T>
    class initializer_list {
    private:
        const _T* __arr;
        size_t __len;
        constexpr initializer_list(const _T* arr, const size_t len)
            : __arr{arr}, __len{len} { }
    public:
        constexpr size_t size() const noexcept { return __len; }
        constexpr const _T* begin() const noexcept { return __arr; }
        constexpr const _T* end() const noexcept { return __arr + __len; }
    };
}

// <type_traits>
namespace std {
    template<typename _T>
    struct remove_reference
    { typedef _T type; };

    template<typename _T>
    struct remove_reference<_T&>
    { typedef _T type; };

    template<typename _T>
    struct remove_reference<_T&&>
    { typedef _T type; };
}

// <move>
namespace std {
    template<typename _T>
    constexpr typename remove_reference<_T>::type&& move(_T&& __ref) noexcept {
        return static_cast<typename remove_reference<_T>::type&&>(__ref);
    }
}

#pragma GCC visibility pop

#endif // __cplusplus
#endif // __PxMATRIX_AVR_STL_H
