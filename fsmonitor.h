void FsMonitorDone(void);
void FsMonitorInit(void);
void FsMonitorChangePath(int num, const char * fullname, int InDropbox);
void PanelInsertItem(int num,const char * fullname);
void PanelDeleteItem(int num,const char * fullname);
void ClearListStrings(GList ** list);
void PanelReload(int num);
void InsertInListDropbox(int num, const char * name);
int FsMonitorChangeMtab(void);