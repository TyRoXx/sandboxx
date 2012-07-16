#ifndef WS_FS_DIRECTORY_H
#define WS_FS_DIRECTORY_H


#include "config.h"


struct directory_entry_t;


bool initialize_file_system_directory(
	struct directory_entry_t *entry,
	const char *path
	);


#endif
