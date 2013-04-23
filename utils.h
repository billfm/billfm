#define		PATH_CACHE_ARCHIVE			"/tmp/billfm/cache"
#define		PATH_INFO_LOG				"/tmp/billfm/log/info.txt"
#define		PATH_INFO_PROGRESS			"/tmp/billfm/log/progress.txt"
#define		PATH_INFO_FIND				"/tmp/billfm/log/find.txt"
#define		PATH_FIND					"/tmp/billfm/find"


#define TASK_MOVE 			1
#define TASK_DELETE 		2
#define TASK_CLEAR_DIR		3
#define TASK_INFO			4
#define TASK_CREATE_DIR		5
#define TASK_CLEAR_TRASH	6
#define TASK_COPY 			7
#define TASK_FIND 			8
#define TASK_SMB_MOUNT		9
#define TASK_READ_TAR		10
#define TASK_DIR_RIGHT		11
#define TASK_FILE_RIGHT		12

#define SOURCE_NO_DELETE	0
#define SOURCE_DELETE 		1


extern int SizeDirShow;

class ClassString
{
	public :
	gchar * s;

	private :
	ClassString  &set_new_value(gchar * str)
	{
		if(s)
		{
//			printf(">=Delete %s\n",s);
			g_free(s);
			s=0;
		}
		s=str;
//		printf(">=Create %s \n",s);
		return *this;
	}
		
	public :
	gchar * getstr(void)
	{
		return g_strdup(s);
	}
			
	ClassString(gchar * str)
	{
		s=str;
//		printf(">Create %s \n",s);		
	};

	ClassString()
	{
		s=0;
	};

	~ClassString()
	{
		if(s)
		{
//			printf(">Delete %s\n",s);
			g_free(s);
			s=0;
		}
	};

	ClassString  &operator=(const char * str)
	{
		return set_new_value(g_strdup(str));
	};

	ClassString  &operator=(gchar * str)
	{
		return set_new_value(str);
	};
};

class InfoOperation
{
	public :
	int func;//тип операции
	int mode_link;//что делать со ссылками
	int source_delete;//
	int dest_override;//
	int dest_open;//
	int source_open;//
	mode_t source_mode;//
    off_t source_size;//		
	mode_t dest_mode;//
    off_t dest_size;//
	int dest_errno;//
	int log;//
	int progress;//
    long int all_done;//размер скопированного-перемещенного
	long int all_size;//полный размер файлов
	int	all_link;//
	int	all_dirs;//
	int	all_files;//	
	int all_hidden;//
	int open_error;//
	mode_t st_mode;//
	InfoOperation()
	{
        progress=-1;
		log=-1;
		all_size=0;
		func=0;
		mode_link=0;
		source_delete=0;
		dest_override=0;
		dest_open=-1;
		source_open=-1;
		source_mode=0;
		source_size=0;
		dest_mode=0;
		dest_size=0;
		dest_errno=0;
		all_link=0;
		all_dirs=0;
		all_files=0;
		all_hidden=0;
		open_error=0;
		all_done=0;
	};
	int Lstat_dest(const char *file_name, const char * mes);
	int Lstat_dest_new(const char *file_name, const char * mes);		
	int Lstat_source(const char *file_name, const char * mes);		

} ;

#define	RESPONSE_RENAME			1
#define	RESPONSE_OVERWRITE		2
#define	RESPONSE_OVERWRITEALL	3
#define	RESPONSE_SKIP			4
#define	RESPONSE_SKIPALL		5
#define	RESPONSE_CANCEL			-1


int DialogOverride( gchar * name, const char ** dest_name );
void BigFileCopy(const char * source,  const char * dest, int func);
void BigRemdir(const char * source);
gchar * PrepareRestore(const char * source);
gchar * showfilesize(long int size);
GList * PanelGetSelected(void);
const char * PanelGetDestDir(void);

void scan_trash(const char * homedir);

void SudoCopyFiles(GList * l, const char * dest_dir);
int IsEmptyDir(const char * source);

gchar * InfoDir(GList * l, InfoOperation * fo);
void UtilsUnlink(GList * l);
void UtilsMoveInTrash(GList * l);
void UtilsMoveFiles(GList * l, const char * dest_dir);
void UtilsClearTrash(void);
void ShowMessage3(GtkWindow* parent, const char* title, const char* message );
void ProcessCopyFiles(const char * dest_dir, InfoOperation * fo);
int FileMoveFunc(const char * source, const char * dest, InfoOperation * fo);
void ExternalFileCopy(uid_t user,int operation);
void ShowCancel(gchar * str);
void CreateNewDirDialog(const char * workdir);
void ExternalCreateDir(const char * dest_dir);
void ExternalClearTrash(const char * dest_dir);
void ShowWarning(gchar * str);
long int GetSizeDir(const char * source);
int DialogCopy(int task, const char * dest, GList * l);
void UtilsCreateLink(const char * source, const char * dest_dir);
void ExternalFind(const char * mask,const char * text, const char * dest_dir, int mode);
gchar * utf8tolower(const char * s);
gchar * utf8toupper(const char * s);
gchar * Link2File(const char * fullname);
int GetTypeFS(const char * source);
int DialogYesNo(const char * mes);
int CreateDirInDir(const char * dest);

typedef int (*func_for_gets)(const char * buf); 
void LoadGets(const char * fullname, func_for_gets func);
void CommandGets(const char * com, func_for_gets func);
void ExternalListTar(const char * fullname, const char * dest_dir);
int CreateNewSymlink(const char * link, const char * dest);
gchar * Untar(const char * name,const char * fullname);
int IsZip(const char * fullname);
int IsTar(const char * destdir);
int IsRar(const char * fullname);
int IsDeb(const char * fullname);
int LinkDialogCopy(InfoOperation * fo, const char * source, const char * dest);
extern gchar * util_path;
extern gchar * app_path;
extern gchar * config_path;
void DrawProgress(void);
void InitExtUtils(void);
void ExternalFileCopy4(uid_t user,int operation,GList * l, const char * dest_dir);
gchar * GetDeletedTime(const char * source);
void SetRightDir(const char * source, int mask);
void ShowFileOperation(gchar * str);