#ifndef LIBROLE_GLOB_H
#define LIBROLE_GLOB_H

#define LIBROLE_START_LINESIZE		200 /* start buffer size for /etc/role line */
#define LIBROLE_MAX_NAME		    100 /* max user/group name length */

#define LIBROLE_ERROR_OPENING_DIRECTORY	158
#define LIBROLE_ERROR_PATH_TOO_LONG	157
#define LIBROLE_NO_SUCH_GROUP		156
#define LIBROLE_OUT_OF_RANGE		155
#define LIBROLE_MEMORY_ERROR		154
#define LIBROLE_IO_ERROR		    153
#define LIBROLE_UNKNOWN_ERROR		152
#define LIBROLE_NOFILE_ERROR		151
#define LIBROLE_INTERNAL_ERROR		150
#define LIBROLE_PAM_ERROR		    149
#define LIBROLE_INCORRECT_VALUE     148
#define LIBROLE_OK			0

#define LIBROLE_CONFIG "/etc/role"

#endif

