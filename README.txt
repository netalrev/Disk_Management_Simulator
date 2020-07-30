Kobiav
Yaakov Amsellem 312560162

The program shows a simulation for disk management on a computer. The program first opens an existing file in the folder representing the disk. The disk must be formatted (command 2) and divided into the required block size as well as how many blocks in each file are written directly (Direct Entries).
Each file will receive an additional block for saving pointers to additional blocks in order to add memory to the file (single in direct).


How to compile:
All the files in order to run the code on vs code (linux) are in ex_final folder:
Run on vscode, or handle by terminal

How to run and what is the input:
Commands:
0 - Exit, delete the disc, and delete the files.
1 - Print the contents of the disc
2 - Format the disk. Block size must also be sent. And quantity of direct blocks
3 - Create a file. The file name must be sent
4 - Open a file. The file name must be sent
5 - Close a file. The descriptor file number must be sent
6 - Write to a file. The file descriptor number and string must be sent for writing
7 - File reading. The descriptor file number and number of characters to read must be sent
8 - Deleting a file. The file name must be sent

