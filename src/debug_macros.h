#ifndef DEBUG_MACROS_H
#define DEBUG_MACROS_H

#include <stdexcept>

/////////////////////////////////////////////////////////
// Global macro settings
// ----------------------------------------
// Note: these options can also be set through the
// makefile using -D<option_name>
/////////////////////////////////////////////////////////

// uncomment the setting below to enable all debug macros (overrides all settings below!)
// #define DEBUG_ENA_ALL

// uncomment the setting below to enable the debug macros to print a diagnostic on
// assertion failure
// #define DEBUG_ENA_PRINT

// uncomment the setting below to enable bounds checking
// #define DEBUG_ENA_BOUNDS_CHECK

// uncomment the setting below to enable the ENSURE assert
// #define DEBUG_ENA_ENSURE

/////////////////////////////////////////////////////////
// Type implementations
/////////////////////////////////////////////////////////

namespace Debug {

class AssertionError : public std::runtime_error {
public:
    template <typename... Args>
    explicit AssertionError(Args&&... args)
        : std::runtime_error(std::forward<Args>(args)...) {}
};

} // namespace Debug

/////////////////////////////////////////////////////////
// Macro implementations
/////////////////////////////////////////////////////////
#ifdef DEBUG_ENA_ALL
#define DEBUG_ENA_BOUNDS_CHECK
#define DEBUG_ENA_ENSURE
#endif

#if defined(DEBUG_ENA_BOUNDS_CHECK) || defined(DEBUG_ENA_ENSURE)
#include <sstream>
#endif

#define UNUSED(x) ((void*)(x))

#ifdef DEBUG_ENA_PRINT
#include <iostream>
#define DEBUG_PRINT(x) (std::cerr << "DEBUG ERROR: " << (x) << std::endl)
#else
#define DEBUG_PRINT(x)
#endif // DEBUG_ENA_PRINT

// bounds checking macros for the four different kinds of boundary conditions.
#ifdef DEBUG_ENA_BOUNDS_CHECK
#define BOUND_CHK_LT(index, bound)                                                       \
    do {                                                                                 \
        if ((index) >= (bound)) {                                                        \
            std::stringstream ss;                                                        \
            ss << "bound check failed at " << __FILE__ << ":" << __LINE__ << std::endl   \
               << "Index '" << (index) << "' >= bound '" << (bound) << "'.";             \
            DEBUG_PRINT(ss.str());                                                       \
            throw Debug::AssertionError(ss.str());                                       \
        }                                                                                \
    } while (0)
#define BOUND_CHK_LTE(index, bound)                                                      \
    do {                                                                                 \
        if ((index) > (bound)) {                                                         \
            std::stringstream ss;                                                        \
            ss << "bound check failed at " << __FILE__ << ":" << __LINE__ << std::endl   \
               << "Index '" << (index) << "' > bound '" << (bound) << "'.";              \
            DEBUG_PRINT(ss.str());                                                       \
            throw Debug::AssertionError(ss.str());                                       \
        }                                                                                \
    } while (0)
#define BOUND_CHK_GT(index, bound)                                                       \
    do {                                                                                 \
        if ((index) <= (bound)) {                                                        \
            std::stringstream ss;                                                        \
            ss << "bound check failed at " << __FILE__ << ":" << __LINE__ << std::endl   \
               << "Index '" << (index) << "' <= bound '" << (bound) << "'.";             \
            DEBUG_PRINT(ss.str());                                                       \
            throw Debug::AssertionError(ss.str());                                       \
        }                                                                                \
    } while (0)
#define BOUND_CHK_GTE(index, bound)                                                      \
    do {                                                                                 \
        if ((index) < (bound)) {                                                         \
            std::stringstream ss;                                                        \
            ss << "bound check failed at " << __FILE__ << ":" << __LINE__ << std::endl   \
               << "Index '" << (index) << "' < bound '" << (bound) << "'.";              \
            DEBUG_PRINT(ss.str());                                                       \
            throw Debug::AssertionError(ss.str());                                       \
        }                                                                                \
    } while (0)
#else // DBG_ENA_BOUNDS_CHECK
#define BOUND_CHK_LT(test_expr, bound)
#define BOUND_CHK_LTE(index, bound)
#define BOUND_CHK_GT(index, bound)
#define BOUND_CHK_GTE(index, bound)
#endif // DEBUG_ENA_BOUNDS_CHECK

// ENSURE macro, which is an enhanced version of the assert() macro from C.
#ifdef DEBUG_ENA_ENSURE
#define ENSURE(expr)                                                                     \
    do {                                                                                 \
        if (!(expr)) {                                                                   \
            std::stringstream ss;                                                        \
            ss << "ENSURE condition '" << #expr << "' failed at " << __FILE__ << ":"     \
               << __LINE__;                                                              \
            DEBUG_PRINT(ss.str());                                                       \
            throw Debug::AssertionError(ss.str());                                       \
        }                                                                                \
    } while (0)
#else // DBG_ENA_ENSURE
#define ENSURE(expr)
#endif // DEBUG_ENA_ENSURE

#endif // DEBUG_MACROS_H
