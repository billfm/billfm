CC=g++
LDLIBS=`pkg-config --libs gtk+-2.0 gmodule-2.0 exo-0.3`
CFLAGS=-Wall -g `pkg-config --cflags gtk+-2.0 gmodule-2.0 exo-0.3`

billfm: billfm.o trash.o mime.o panels.o on_button.o exopanel.o treepanel.o sidepanel.o override.o\
 ptk-clipboard.o path-entry.o input-dialog.o icon-cell-renderer.o utils.o disks.o setting.o\
 fileprop.o dropbox.o fsmonitor.o mainmenu.o bufgets.o extutils.o\

	$(CC) $(LDLIBS) -o billfm billfm.o trash.o mime.o panels.o on_button.o exopanel.o treepanel.o\
	 sidepanel.o override.o ptk-clipboard.o path-entry.o input-dialog.o icon-cell-renderer.o\
	 utils.o disks.o fileprop.o dropbox.o fsmonitor.o setting.o mainmenu.o bufgets.o extutils.o

bufgets.o: bufgets.cpp
	$(CC) $(CFLAGS) -c bufgets.cpp

mainmenu.o: mainmenu.cpp
	$(CC) $(CFLAGS) -c mainmenu.cpp

setting.o: setting.cpp
	$(CC) $(CFLAGS) -c setting.cpp

fsmonitor.o: fsmonitor.cpp
	$(CC) $(CFLAGS) -c fsmonitor.cpp

dropbox.o: dropbox.cpp
	$(CC) $(CFLAGS) -c dropbox.cpp

fileprop.o: fileprop.cpp
	$(CC) $(CFLAGS) -c fileprop.cpp

override.o: override.cpp
	$(CC) $(CFLAGS) -c override.cpp

billfm.o: billfm.cpp
	$(CC) $(CFLAGS) -c billfm.cpp

trash.o: trash.cpp
	$(CC) $(CFLAGS) -c trash.cpp

mime.o: mime.cpp
	$(CC) $(CFLAGS) -c mime.cpp

panels.o: panels.cpp
	$(CC) $(CFLAGS) -c panels.cpp

on_button.o: on_button.cpp
	$(CC) $(CFLAGS) -c on_button.cpp

exopanel.o: exopanel.cpp
	$(CC) $(CFLAGS) -c exopanel.cpp

treepanel.o: treepanel.cpp
	$(CC) $(CFLAGS) -c treepanel.cpp

sidepanel.o: sidepanel.cpp
	$(CC) $(CFLAGS) -c sidepanel.cpp

ptk-clipboard.o: ptk-clipboard.c
	$(CC) $(CFLAGS) -c ptk-clipboard.c

path-entry.o: path-entry.cpp
	$(CC) $(CFLAGS) -c path-entry.cpp

input-dialog.o: input-dialog.cpp
	$(CC) $(CFLAGS) -c input-dialog.cpp

icon-cell-renderer.o: icon-cell-renderer.cpp
	$(CC) $(CFLAGS) -c icon-cell-renderer.cpp

utils.o: utils.cpp
	$(CC) $(CFLAGS) -c utils.cpp

extutils.o: extutils.cpp
	$(CC) $(CFLAGS) -c extutils.cpp

disks.o: disks.cpp
	$(CC) $(CFLAGS) -c disks.cpp


clean:
	rm -f billfm
	rm -f *.o
