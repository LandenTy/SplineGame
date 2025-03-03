CC = gcc
CFLAGS = -Wall -mwindows
OBJ = main.o
EXE = spline.exe

$(EXE): $(OBJ)
	$(CC) $(CFLAGS) -o $(EXE) $(OBJ)

ik_win32.o: ik_win32.c
	$(CC) $(CFLAGS) -c main.c

clean:
	del $(OBJ) $(EXE)
