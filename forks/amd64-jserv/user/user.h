// claude: this port's additions to the shared API. Everything else lives in
// include/user/user.h; this file is found first because the fork root comes
// earlier on the include path, and pulls the shared set in by relative path.
#include "../../../include/user/user.h"

int chmod(const char*, int);
