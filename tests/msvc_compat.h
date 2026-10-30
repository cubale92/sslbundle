#pragma once
// The pinned toolkit declares an optional weak callback using GCC syntax.
// These host tests never reference that callback; production builds use GCC.
#ifdef _MSC_VER
#define __attribute__(x)
#endif
