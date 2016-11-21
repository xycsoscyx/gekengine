/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 895bd642687b9b4b70b544b22192a2aa11a6e721 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date:   Thu Oct 13 20:39:05 2016 +0000 $
#pragma once

#include <stdexcept>

#define GEK_ADD_EXCEPTION_2(TYPE, BASE) struct TYPE : public BASE { using BASE::BASE; };
#define GEK_ADD_EXCEPTION_1(TYPE) struct TYPE : public std::runtime_error { using std::runtime_error::runtime_error; };
#define GEK_ADD_EXCEPTION_0() GEK_ADD_EXCEPTION_1(Exception)
#define GEK_EXCEPTION_CHOOSER(_f1, _f2, _f3, ...) _f3
#define GEK_EXCEPTION_RECOMPOSER(argsWithParentheses) GEK_EXCEPTION_CHOOSER argsWithParentheses
#define GEK_EXCEPTION_CHOOSE_COUNT(...) GEK_EXCEPTION_RECOMPOSER((__VA_ARGS__, GEK_ADD_EXCEPTION_2, GEK_ADD_EXCEPTION_1, ))
#define GEK_ADD_EXCEPTION_ZERO() ,,GEK_ADD_EXCEPTION_0
#define GEK_ADD_EXCEPTION_MACRO(...) GEK_EXCEPTION_CHOOSE_COUNT(GEK_ADD_EXCEPTION_ZERO __VA_ARGS__ ())
#define GEK_ADD_EXCEPTION(...) GEK_ADD_EXCEPTION_MACRO(__VA_ARGS__)(__VA_ARGS__)

#define GEK_REQUIRE(CHECK)              do { if (!(CHECK)) { _ASSERTE(CHECK); exit(-1); } } while (false)
