/* miniz (MIT, third_party/miniz) as one unit of the app: the build compiles what is under app/. */
#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES
#include "../../third_party/miniz/miniz.c"
#include "../../third_party/miniz/miniz_tdef.c"
#include "../../third_party/miniz/miniz_tinfl.c"
#include "../../third_party/miniz/miniz_zip.c"
