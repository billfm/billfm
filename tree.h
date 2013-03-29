#include <sys/stat.h>
#include <sys/types.h>
#include <exo/exo.h>

#include <iostream>
#include <string>
#include <errno.h>

using namespace std;

#define		ICON_GNOME_TEXT				0
#define		ICON_GNOME_FS_FOLDER		1
#define		ICON_GNOME_X_APP			2
#define		ICON_GNOME_X_SCRIPT			3
#define		ICON_GNOME_FS_HOME			4
#define		ICON_GNOME_FS_DESKTOP		5
#define		ICON_GNOME_FS_EMPTY_TRASH	6
#define		ICON_GNOME_FS_FULL_TRASH	7
#define		ICON_GNOME_BREAK_LINK		8
#define		ICON_GNOME_DROPBOX			9
#define		ICON_GNOME_HARD				10
#define		ICON_GNOME_APPLICATION		11
#define		ICON_GNOME_BOOKMARK 		12
#define		ICON_GNOME_REMOVABLE		13
#define		ICON_GNOME_OPTICAL  		14
#define		ICON_GNOME_NETWORK			15
#define		ICON_GNOME_COMPUTER  		16
#define		ICON_GNOME_SMB_SHARE  		17
#define		ICON_GNOME_COMMANDER  		18

#define		FIRST_FREE_INDEX			19


#define		MIME_FLAG_LINK				0x8000
#define		MIME_FLAG_DROPBOX_OK		0x4000
#define		MIME_FLAG_DROPBOX_SYNC		0x2000

#define		TV_COUNT			2
#define 	MAX_COUNT_COLOMNS	6



#define TYPE_SORT_NONE		0
#define TYPE_SORT_NAME		1
#define TYPE_SORT_EXT		2
#define TYPE_SORT_TIME		3
#define TYPE_SORT_SIZE		4
#define TYPE_SORT_UNKNOW	5

enum
{
	COL_ICON,//0
	MODEL_TEXT_NAME,//1
	MODEL_TEXT_COL_EXT,//2
	MODEL_TEXT_COL_RIGHT,//3
	MODEL_TEXT_COL_SIZE,//4
	MODEL_TEXT_COL_DATETIME,//5
	MODEL_TEXT_FULL_NAME,//6
	MODEL_INT_COL_MIME,
	MODEL_INT_COL_FILE_TYPE,	
	MODEL_INT_COL_ISDIR,
	MODEL_INT_COL_SIZE,
	MODEL_INT_COL_DATETIME,	
	N_COLS
}; 

#define VIEW_COL_NAME			MODEL_TEXT_NAME
#define VIEW_COL_EXT			MODEL_TEXT_COL_EXT
#define VIEW_COL_RIGHT			MODEL_TEXT_COL_RIGHT
#define VIEW_COL_SIZE			MODEL_TEXT_COL_SIZE
#define VIEW_COL_DATETIME		MODEL_TEXT_COL_DATETIME

//-----------------------------------------------------------------------------

class ClassPanel
{
 protected:
	gchar * ColumnsName[MAX_COUNT_COLOMNS];
 protected:
    ExoIconView*	iconview;
	GtkTreeView*	treeview;
 public:
    int SavePath;
	int ShowHidden;
	int InDropboxFlag;
	int OkSearchFlag;		
	GList* list_black_files;		
	int TypeFS;
	GList* file_list;
	GList* paste_list;
	gchar * MyPath;
	int SortType;
	int MyIndex;
	int MyMode;
	GtkButton *	but_path;
	GList * history;
	ClassPanel();		
	void reload(void);
	void __LoadDir(const char * path_folder);
	void LoadDir(const char * path_folder);
	void LoadMenu(const char * path_folder);

	const char * get_path(void);
	void CreatePanels(void);
	void  OnDoubleClick(GtkTreePath *path);
	void  ItemDoubleClick( const char *path );
	void SetMyPath(const char * path_folder);
	gchar* GetSelectedItem(void);
	GList* GetSelectedFiles(void);
	gchar* GetSelectedFile(void);
	gchar* GetSelectedDir(void);
	int GetSelectedItem(gchar ** fullname, int * mime,gchar ** name);
		
	virtual void SetVisibleColumn(int i, int val);
	virtual	void SetColumnTitle(int i, char * name);
	virtual	int GetPair(void);
	virtual	void CreatePanel(void)=0;
	virtual	void SetCursor(void)=0;
	virtual void SetActive(void) {};
	virtual void SetNotActive(void) {};		
	virtual	GList * get_selected(GtkTreeModel** model);
	virtual GtkTreeModel * GetModel(void);
	virtual void SetModel(GtkTreeModel * model);
	virtual GtkWidget* GetWidget(void);
	virtual void OnButtonEditCell(void);
	virtual void DeInit(void)=0;
	virtual	void SetCursor(const char * tag)=0;
	virtual	void SelectAll(void){};
	virtual	void UnSelectAll(void){};
	virtual	void SelectPattern(const char * pattern){};
	virtual	void SetSort(int new_sort){};
	class OnError
	{

	};

	void TestExeption(void)
	{
		throw OnError();
	}

	void BackPanel(void);
	GtkListStore * CreateNewModel();
	int GetSelectedCount(void);
	int InTrash(const char * name);
	int SetPropertyItem(GtkListStore *store, const char * name);
	void InsertItem(const char * name);
	void LoadBlackFiles();
	int IsBlackFile(const char * name,const char * ext);
	void DeleteItem(const char * name);
	gboolean FindIter(const char * tagname,GtkTreeIter * iter,int col);

	void InfoSelectedInStatusBar(void);
	void InfoSingleInStatusBar(void);
	void InfoMultiInStatusBar(void);		
} ;

//-----------------------------------------------------------------------------

class ClassTreePanel : public ClassPanel
{
	public :

	ClassTreePanel()
	{
		MyMode = 0;
		iconview = 0;
		treeview = 0;
		but_path = 0;
		MyPath =0;
	}
	virtual	void CreatePanel(void); 
	virtual	void SetCursor(void);
	virtual void SetActive(void);
	virtual void SetNotActive(void);		
	void create_text_column( gint index_col );
	void create_icon_column( gint icon_col );
	virtual void DeInit(void);
	virtual void SetCursor(const char * tag);

	void SelectAll(void);
	void UnSelectAll(void);	
	void SelectPattern(const char * pattern);
	void SetSort(int new_sort);
	void Diff(const char * p1,const char * p2, int flags);
	void LoadFromFile(const char * source);				
};

//-----------------------------------------------------------------------------

class ClassSidePanel : public ClassTreePanel
{
public :
//	gchar * DropboxStorePath;
	ClassSidePanel();
	void LoadDevice(void);
	virtual	void CreatePanel(void); 
	virtual	void SetCursor(void);
	virtual	int GetPair(void);
	void SetDropboxIcon(int mode);
	void ScanBookmark(GtkListStore * store);
	void ScanDiskByLabel(GtkListStore * store);
	void ScanGvfs(GtkListStore * store);
	void ScanTrash(GtkListStore * store);
};

//-----------------------------------------------------------------------------

class ClassIconPanel : public ClassPanel
{
public :
	ClassIconPanel()
	{
		MyMode = 1;
	}
	virtual	void CreatePanel(void); 
	virtual	void SetCursor(void);
	virtual	GList * get_selected(GtkTreeModel** model);
    virtual GtkTreeModel * GetModel(void);
    virtual void SetModel(GtkTreeModel * model);
    virtual GtkWidget* GetWidget(void);
	virtual void SetVisibleColumn(int i, int val);
	virtual	void SetColumnTitle(int i, char * name);
	virtual void DeInit(void);
	virtual	void SetCursor(const char * tag) {};
	void SelectAll(void);
	void UnSelectAll(void);	
	void SelectPattern(const char * pattern);
};

//-----------------------------------------------------------------------------

extern ClassPanel * Panels[TV_COUNT];

extern GdkPixbuf* ICON_FOLDER;
extern	int ActivePanel;
extern	GtkBuilder *builder;
extern	GtkStatusbar* StatusBar;
extern	GtkWidget * SidePanel;
extern	GtkWidget * OnePanel;
extern	ClassSidePanel side_panel;
extern	GtkWindow * topWindow;

extern	int ShowSide;
extern	int ShowOne;
extern	gchar * DropboxFolder;


void create_text_column( GtkTreeView * treeview, gint index_col );
void create_icon_column( GtkTreeView *treeview, gint icon_col );
void InitPanel(ClassPanel * panels);

void  treeview_OnDoubleClick (GtkTreeView        *treeview,
                       GtkTreePath        *path,
                       GtkTreeViewColumn  *col,
                       gpointer            userdata);


void InitButton(void);


void OnButtonEdit( GtkButton* button, int index_operation );
void OnButtonDelete( GtkButton* button, int index_operation );
void OnButtonHidden( GtkButton* button, int index_operation );
void OnButtonNewDir( GtkButton* button, int index_operation );
void OnButtonCopy( GtkButton* button, int index_operation );
void OnButtonMove( GtkButton* button, int index_operation );
void OnButtonZip( GtkButton* button, int index_operation );
void DeleteFile(char * full_path);

void OnButtonTest( GtkButton* button, int index_operation );
void OnShiftEdit( GtkButton* button, int index_operation );
void OnButtonOnePanel( GtkButton* button, int index_operation );
void OnButtonSide( GtkButton* button, int index_operation );
void OnButtonHome( GtkButton* button, int index_operation );
void OnButtonReload( GtkButton* button, int index_operation);
void OnMenuOnePanel (GtkObject *object, gpointer user_data);
void OnMenuSidePanel (GtkObject *object, gpointer user_data);
void OnButtonModeView( GtkButton* button, int index_operation );

void	SetButtonTitle( int i , char * title);
gboolean  OnButtonPressedSidePanel (GtkWidget *treeview, GdkEventButton *event, gpointer userdata);
gboolean  ClickActivePanel(GtkWidget* widget, GdkEventFocus* event, gpointer user_data);
void	ReadSettings(void);
void	SaveSetting(void);

#define OPERATION_COPY	0
#define OPERATION_CUT	1

GList* ptk_clipboard_paste_files( int * action );
void ptk_clipboard_cut_or_copy_files(GList* files, int action);
void DrawPathButton(const char * path, int save_path);
GtkWidget* ptk_path_entry_new();
void CreateNewDirDialog(const char * workdir);
void OnButtonRestore( GtkButton* button, int index_operation );

void ptk_show_error(GtkWindow* parent, const char* title, const char* message );
gchar * EditFileName(const char * name);
void CreatePanel(int i, int mode);
void OnButtonBack( GtkButton* button, int data );
void OnButtonBookmark( GtkButton* button, int index_operation );
void RestoreSelectedFiles();
void ClickExecFile(const char * name, struct stat * file_stat);
void NewSearch(const char * workdir);
gchar * NewPattern(void);
void OnButtonSelAll( GtkButton* button, int index_operation );
void OnButtonUnselAll( GtkButton* button, int index_operation );
void OnButtonSelPattern( GtkButton* button, int index_operation );
int GetMime(const char * path_folder, const char * full_name);
gchar * get_file_rigth_string( mode_t mode );
gchar * get_file_mtime_string(time_t mtime);
void DialogFileProperty(const char * name);
void ScanMime(void);
gchar * GetExt(const char * fullname);
void OnButtonUp( GtkButton* button, void * datauser );

typedef struct _InfoMime
{
	gchar * type;
	int  icon_index;
	GList * list_command;
} InfoMime;

GList * GetMimeListCommand(const char *name);
extern GdkPixbuf*  IconPixbuf[];
GdkPixbuf * GetIconByName(const char * name, int size);

void ClearDir(const char * source);
void MoveFiles(GList * l, const char * dest_dir);
void CopyFiles(GList * l, const char * dest_dir);

#define	ICON_SIZE	16
extern int SidePanelSelectDropbox;
extern int SelectedSidePanel;
void InitFarCopy(GtkProgressBar* pb);
void DoneFarCopy();
void CreateNewFileDialog(const char * workdir,const char * command);
void OnShiftSearch( GtkButton* button, int index_operation ); 
void CopyProperty(const char * source, const char * dest);

void InsertInListDropbox(int num, const char * fullname);
int InDropbox(int num,const char * path);
int PanelUpdateItemDropbox(int num,gchar * tagname);
void OnButtonDiff( GtkButton* button, int index_operation );
void ConnectMemuSignal();
void UtilsSaveFiles(const char * name);
int  find_userbin_mime(const char * p);
void ScanUsrbin(void);
void InsertCommand(char * ext,char * command);
gchar * GetNetworkPath(void);
gchar * GetArchivePath(void);
gchar * GetDesktopKey(const char * file_name,const char * key_name);
const char * IsAlreadyTrashed( const char* name );
gchar * GetDesktopCommand(const char* name);


extern GdkPixbuf* PixbufEmblemLink;
extern GdkPixbuf* PixbufEmblemDropboxOk;
extern GdkPixbuf* PixbufEmblemDropboxSync;

void ConnectDropbox();
extern gchar * DropboxFolder;
void InitDropbox();
int GetDropboxStatus();
int GetDropboxStatusFile(const char * fullname);
void DrawIconDropbox();
GtkBuilder* CreateForm(const char * name);
int InMenuPath(const char * path);