#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>


char * find_trash_dev(int dev);
void scan_trash(void);
void DeleteFile(char * full_path);
void g_check_free(char ** p);
const char * get_name_trash( const char * filename );
gchar * get_trashinfo(char * filename,char * trash_name);
gchar * get_full_restore_name( const char * source, const char * trash_name, const char * dest );
const char * Filename2TrashDir(const char * fullname);
