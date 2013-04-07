#define TYPE_FS_UNKNOW	0
#define TYPE_FS_SWAP	1
#define TYPE_FS_NTFS	2
#define TYPE_FS_EXT3	3
#define TYPE_FS_EXT4	4

void InitListDisk(const char * homedir,const char * uid);
int GetFreeSpace(const char * mount);
gchar * Dev2Mount(const char * dev);
gchar * Mount2Dev(const char * mount);
gchar * CheckTrash(gchar * name);

typedef struct _InfoDisk
{
	gchar * path;
	gchar * mount;
	gchar * label;	
	int num_dev;
	int full;
	int fs_type;
	long int total_size;
	long int free_size;
	gchar * path_trash;
} InfoDisk;

GList * GetListDisk(void);
InfoDisk * Mount2Info(const char * source);
int  GetDeviceType(const char * dev);

extern GList * List_disk;