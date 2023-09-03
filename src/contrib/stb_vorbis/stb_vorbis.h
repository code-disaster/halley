#pragma once

#define STB_VORBIS_NO_STDIO
#define STB_VORBIS_NO_INTEGER_CONVERSION

#if !defined(STB_VORBIS_IMPLEMENTATION)
#define STB_VORBIS_HEADER_ONLY
#endif

#include "stb_vorbis.c"
