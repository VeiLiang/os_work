#include "ucFS_Version.h"

# define UCFS_LIB_VERSION_MAJOR	0
# define UCFS_LIB_VERSION_MINOR	4
# define UCFS_LIB_VERSION_PATCH	9
# define UCFS_LIB_VERSION_EXTRA	" (beta)"

# define UCFS_LIB_VERSION_STRINGIZE(str)	#str
# define UCFS_LIB_VERSION_STRING(num)	UCFS_LIB_VERSION_STRINGIZE(num)

# define UCFS_LIB_VERSION		UCFS_LIB_VERSION_STRING(UCFS_LIB_VERSION_MAJOR) "."  \
				UCFS_LIB_VERSION_STRING(UCFS_LIB_VERSION_MINOR) "."  \
				UCFS_LIB_VERSION_STRING(UCFS_LIB_VERSION_PATCH)  \
				UCFS_LIB_VERSION_EXTRA
				
static char const ucFS_lib_version[]   = "Arkmicro FS Libary " UCFS_LIB_VERSION;

const char *GetFSLibVersion(void)
{
	return ucFS_lib_version;
}

