#include <iostream>
#include <vector>
#include <map>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

#define DISK_SIZE 256
#define ON 1
#define OFF 0


void decToBinary(int n , char &c)
{
    // array to store binary number
    int binaryNum[8];
    // counter for binary array
    int i = 0;
    while (n > 0) {
          // storing remainder in binary array
        binaryNum[i] = n % 2;
        n = n / 2;
        i++;
    }
  
    // printing binary array in reverse order
    for (int j = i - 1; j >= 0; j--) {
        if (binaryNum[j]==1)
            c = c | 1u << j;
    }
 }

// just to print error nicely 
void error(string s){
    cout << "[!ERROR] " << s << endl;
}

// #define SYS_CALL
// ============================================================================
class fsInode {
    int fileSize;
    int block_in_use;
    int *directBlocks;
    int singleInDirect;
    int num_of_direct_blocks;
    int block_size;


    public:
    fsInode(int _block_size, int _num_of_direct_blocks) {
        
        fileSize = 0;
        block_in_use = 0; 
        block_size = _block_size;
        num_of_direct_blocks = _num_of_direct_blocks;
        directBlocks = new int[num_of_direct_blocks];
		assert(directBlocks);
        for (int i = 0 ; i < num_of_direct_blocks; i++) {   
            directBlocks[i] = -1;
        }
        singleInDirect = -1;
    }
    
    ~fsInode() {
        delete[] directBlocks;
    }
    
    int getFileSize(){
        return fileSize;
    }
    
    void setFileSize(int size){
        fileSize = size;
        block_in_use = size/block_size;
        if(size%block_size > 0)
            block_in_use++;
    }
    
    int getBlockInUse(){
        return block_in_use;
    }

    int getDirectBlock(int i){
        return directBlocks[i];
    }
    void setDirectBlock(int block){
        directBlocks[block_in_use] = block;
        block_in_use++;
    }
    
    int getInDirectBlock(){
        return singleInDirect;
    }
    
    void setInDirect(int block){
        singleInDirect = block;
    }

};

// ============================================================================
class FileDescriptor {
    pair<string, fsInode*> file;
    bool inUse;

    public:
    FileDescriptor(string FileName, fsInode* fsi) {
        file.first = FileName;
        file.second = fsi;
        inUse = true;
    }

    string getFileName() {
        return file.first;
    }
    
    void setFileName(string s) {
        file.first = s;
    }

    fsInode* getInode() {
        return file.second;
    }
    
    void setInode(fsInode *inode) {
        file.second = inode;
    }

    bool isInUse() { 
        return (inUse); 
    }
    
    void setInUse(bool _inUse) {
        inUse = _inUse ;
    }
};
 
#define DISK_SIM_FILE "DISK_SIM_FILE.txt"
// ============================================================================
class fsDisk {
    
    FILE *sim_disk_fd;
    bool is_formated;

	// BitVector - "bit" (int) vector, indicate which block in the disk is free
	//              or not.  (i.e. if BitVector[0] == 1 , means that the 
	//             first block is occupied. 
    int BitVectorSize;
    int *BitVector;

    // Unix directories are lists of association structures, 
    // each of which contains one filename and one inode number.
    map<string, fsInode*>  MainDir;

    // OpenFileDescriptors --  when you open a file, 
	// the operating system creates an entry to represent that file
    // This entry number is the file descriptor. 
    vector<FileDescriptor> OpenFileDescriptors;

    int direct_enteris;
    int block_size;
    int maxSize;
    int freeBlocks;
    // ------------------------------------------------------------------------
    // check if the file descriptor number is in the range
    bool fdIsVaild(int fd){
        if(fd <0 || fd >= OpenFileDescriptors.size())
            return false;
        return true;
    }
    // ------------------------------------------------------------------------
    // func to get a pointer to free block if posible
    int getFreeBlock(){
        if(freeBlocks == 0)
            return -1;
        for (int i = 0; i<BitVectorSize; i++) {
            if(BitVector[i] == OFF){
                BitVector[i] = ON;
                freeBlocks--;
                return i;
            }
        }
        return -1;
    }
    // ------------------------------------------------------------------------
    // func to clear a block on disk & update the BitVector
    void deleteBlock(int i){
        int pointer = i*block_size;
        fseek(sim_disk_fd, pointer, SEEK_SET);
        for (int del = 0; del < block_size; del++)
            assert(1 == fwrite("\0", 1, 1, sim_disk_fd));
        BitVector[i] = OFF;
        freeBlocks++;
    }

    public:
    // ------------------------------------------------------------------------
    fsDisk() {
        sim_disk_fd = fopen(DISK_SIM_FILE , "r+");
        assert(sim_disk_fd);
        for (int i=0; i < DISK_SIZE ; i++) {
            int ret_val = fseek ( sim_disk_fd , i , SEEK_SET );
            ret_val = fwrite( "\0" ,  1 , 1, sim_disk_fd);
            assert(ret_val == 1);
        }
        fflush(sim_disk_fd);
        direct_enteris = 0;
        block_size = 0;
        is_formated = false;
    }
    // ------------------------------------------------------------------------
    
    ~fsDisk() {
        if(is_formated){
            for (auto it = begin (OpenFileDescriptors); it != end (OpenFileDescriptors); ++it)
                if(MainDir[it->getFileName()])
                    DelFile(it->getFileName());
            delete[] BitVector;
        }
        fclose(sim_disk_fd);
    }
    // ------------------------------------------------------------------------
    void listAll() {
        int i = 0;    
        for (auto it = begin (OpenFileDescriptors); it != end (OpenFileDescriptors); ++it) {
            cout << "index: " << i << ": FileName: " << it->getFileName() <<  " , isInUse: " << it->isInUse() << endl; 
            i++;
        }
        char bufy;
        cout << "Disk content: '" ;
        for (i=0; i < DISK_SIZE ; i++) {
            int ret_val = fseek ( sim_disk_fd , i , SEEK_SET );
            ret_val = fread(  &bufy , 1 , 1, sim_disk_fd );
            cout << bufy;
        }
        cout << "'" << endl;
    }
    // ------------------------------------------------------------------------
    // func to format the disk and divide to requested prats
    void fsFormat(int blockSize = 4, int direct_Enteris_ = 3) {
        if(blockSize <1 || direct_Enteris_ < 1 || direct_Enteris_ > blockSize)
            return;
        block_size = blockSize;
        direct_enteris = direct_Enteris_;
        BitVectorSize = DISK_SIZE/blockSize;
        freeBlocks = BitVectorSize;
        maxSize = (block_size+direct_enteris)*block_size;
        BitVector = new int[BitVectorSize];
        assert(BitVector);
        for (int i = 0; i<BitVectorSize ; i++)
            BitVector[i] = OFF;
        is_formated = true;
    }
    // ------------------------------------------------------------------------
    // checks that is a free space in disk & no other file with this name. create new file & update disk data structures
    int CreateFile(string fileName) {
        if(!is_formated){
            error("Invalid file creation request - Disk Not Formated");
            return -1;
        } else if(freeBlocks == 0) {
            error("Invalid file creation request - no Free space avilable");
            return -1;
        } else if (MainDir[fileName]){
            error("Invalid file creation request - a file with this name already exists");
            return -1;
        }
        fsInode *inode = new fsInode(block_size, direct_enteris);
        assert(inode);
        MainDir[fileName] = inode;
        int i = 0;
        
        for (auto it = begin (OpenFileDescriptors); it != end (OpenFileDescriptors); ++it, i++){
            if(it->getFileName().compare("\0") == 0){
                it->setFileName(fileName);
                it->setInode(inode);
                it->setInUse(true);
                break;
            }
        }
        if(i == OpenFileDescriptors.size()){
            FileDescriptor fd(fileName, inode);
            OpenFileDescriptors.push_back(fd);
        }
        
        return i;
    }
    // ------------------------------------------------------------------------
    // chacks if there is file with such this name, it is closed, and opens it in the File Descriptors
    int OpenFile(string fileName) {
        if(!is_formated){
            error("Invalid file open request - Disk Not Formated");
            return -1;
        }
        fsInode *inode = MainDir[fileName];
        if(!inode){
            error("Invalid file open request - no such file");
            return -1;
        }
        int i = 0;
        for (auto it = begin (OpenFileDescriptors); it != end (OpenFileDescriptors); ++it, i++){
            if(it->getFileName().compare(fileName) == 0){
                if(it->isInUse()){
                    error("Invalid file open request - file already open");
                    return -1;
                }
                it->setInUse(true);
                return i;
            }
        }
        return -1;
    }
    // ------------------------------------------------------------------------
    // chacks if there is file with such this name, it is opened, and close it in the File Descriptors
    string CloseFile(int fd) {
        if(!is_formated){
            error("Invalid file closing request - Disk Not Formated");
            return "-1";
        } else if(!fdIsVaild(fd)) {
            error("Invalid file closing request - File Descriptor is not exist");
            return "-1";
        } else if (!OpenFileDescriptors[fd].isInUse()){
            error("Invalid file closing request - This file is already closed");
            return "-1";
        }
        OpenFileDescriptors[fd].setInUse(false);
        return "1";
    }
    // ------------------------------------------------------------------------
    // checks is a free space avilable in disk to this write session
    bool isFreeSpaceAvilavle(int curSize, int reqSize){
        if(reqSize > maxSize - curSize)
            return false;
        int curBlocks = curSize/block_size;
        if(curSize%block_size > 0)
            curBlocks++;
        int totBlocks = (curSize+reqSize)/block_size;
        if((curSize+reqSize)%block_size > 0)
            totBlocks++;
        if(curBlocks <= direct_enteris &&  totBlocks > direct_enteris)
            totBlocks++;
        if(totBlocks-curBlocks > freeBlocks)
            return false;
        return true;
    }
    // ------------------------------------------------------------------------
    // get a fd to write into, checks that the fd is exists and it is open,
    // checks if write to direct blocks or maybe to inDirect, if needed it request free blocks
    // and connect it to the file & write into, func write ONLY if there is free space in disk and the
    // current file.
    int WriteToFile(int fd, char *buf, int len) {
        if(!is_formated){
            error("Invalid file writing request - Disk Not Formated");
            return -1;
        } else if(!fdIsVaild(fd)) {
            error("Invalid file writing request - File Descriptor is not exist");
            return -1;
        } else if (!OpenFileDescriptors[fd].isInUse()){
            error("Invalid file writing request - This file is closed");
            return -1;
        }
        fsInode *inode = OpenFileDescriptors[fd].getInode();

        if(!isFreeSpaceAvilavle(inode->getFileSize(), len)){
            error("Invalid file writing request - input is over size");
            return -1;
        }
        int inUse = inode->getBlockInUse();
        int offset = inode->getFileSize()%block_size;
        int fcheck, pointer, writed = 0;
        // it write block after block.
        while(writed < len && (inode->getFileSize()+writed)<maxSize) {
            if(offset == 0){                            // case write into new block
                int newBlock = getFreeBlock();
                if(newBlock <0)
                    return -1;
                if(inUse<direct_enteris){               // case write into direct block
                    inode->setDirectBlock(newBlock);
                } else {                                // case write into in direct block
                    if(inode->getInDirectBlock() < 0){  // init single in direct block
                        inode->setInDirect(newBlock);
                        newBlock = getFreeBlock();
                        if(newBlock<0)
                            return -1;
                    }
                    pointer = (inode->getInDirectBlock())*block_size+(inUse-direct_enteris);
                    fseek(sim_disk_fd, pointer, SEEK_SET);
                    char inDir = 0;
                    decToBinary(newBlock, inDir);
                    fcheck = fwrite(&inDir, 1, 1, sim_disk_fd);  // write into single in direct the pointer to next block
                    assert(fcheck == 1);
                }
                pointer = newBlock*block_size;
                fseek(sim_disk_fd, pointer, SEEK_SET);
                while (offset == 0 || (writed < len && offset%block_size != 0)) {
                    fcheck = fwrite(&buf[writed], 1, 1, sim_disk_fd);
                    assert(fcheck == 1);
                    offset++;
                    writed++;
                }
                inUse++;
            } else {            // case we need to write in new block
                if(inUse <= direct_enteris){
                    pointer = (inode->getDirectBlock(inUse-1))*block_size+offset;
                } else {
                    int inDirectBlock = inode->getInDirectBlock();
                    int inDirectOffset = inUse-direct_enteris;
                    pointer = (inode->getInDirectBlock())*block_size + inUse-direct_enteris-1;
                    fseek(sim_disk_fd, pointer, SEEK_SET);
                    char inDir = 0;
                    fcheck = fread(&inDir, 1, 1, sim_disk_fd);
                    assert(fcheck == 1);
                    pointer = inDir*block_size+offset;
                }
                
                fseek(sim_disk_fd, pointer, SEEK_SET);
                while (writed < len && offset%block_size != 0) {
                    fcheck = fwrite(&buf[writed], 1, 1, sim_disk_fd);
                    assert(fcheck == 1);
                    offset++;
                    writed++;
                }
            }
            offset = 0;
        }
        inode->setFileSize(inode->getFileSize()+writed);
        fflush(sim_disk_fd);
        return writed;
    }
    // ------------------------------------------------------------------------
    // checks that disk is formated, file exists and it is open. if the request is vaild the function read
    // from file, start with the direct blocks and after read the indirect block to get the next blocks with data
    int ReadFromFile(int fd, char *buf, int len) {
        buf[0] = '\0';
        if(!is_formated){
            error("Invalid file reading request - Disk Not Formated");
            return -1;
        } else if(!fdIsVaild(fd)) {
            error("Invalid file reading request - File Descriptor is not exist");
            return -1;
        } else if (!OpenFileDescriptors[fd].isInUse()){
            error("Invalid file reading request - This file is closed");
            return -1;
        }
        fsInode *inode = OpenFileDescriptors[fd].getInode();
        int fileSize = inode->getFileSize();
        int fcheck, pointer, offset, block, readed;
        for (readed = 0; readed < fileSize && readed < len ; readed++) {
            block = readed/block_size;
            offset = readed%block_size;
            if(block < direct_enteris){
                block = inode->getDirectBlock(readed/block_size);
                pointer = block*block_size+offset;
            } else {
                int inDirectOffset = block - direct_enteris;
                int inDirectBlock = inode->getInDirectBlock();
                pointer = inDirectBlock*block_size+inDirectOffset;
                fseek(sim_disk_fd, pointer, SEEK_SET);
                char inDir = 0;
                fcheck = fread(&inDir, 1, 1, sim_disk_fd);
                assert(fcheck);
                pointer = inDir*block_size + offset;
            }
            fseek(sim_disk_fd, pointer, SEEK_SET);
            fcheck = fread(&buf[readed], 1, 1, sim_disk_fd);
            assert(fcheck == 1);
        }
        buf[readed] = '\0';
        return readed;
    }
    // ------------------------------------------------------------------------
    // check that the request is vaild, after delete the file blocks on disk and delete all the disk data structures
    int DelFile(string FileName) {
        if(!is_formated){
            error("Invalid delete file request - Disk Not Formated");
            return -1;
        }
        fsInode *inode = MainDir[FileName];
        if(!inode){
            error("Invalid delete file request - File is not exist");
            return -1;
        }
        int blockToDel, pointer, fcheck;
        int singleInDirect = inode->getInDirectBlock();
        for (int i = inode->getBlockInUse(); i > 0; i--) {
            if(i > direct_enteris){         // there is a in direct block to delete
                int InDirOffset = i - direct_enteris -1;
                pointer = singleInDirect*block_size+InDirOffset;
                fseek(sim_disk_fd, pointer, SEEK_SET);
                char inDir = 0;
                fcheck = fread(&inDir, 1, 1, sim_disk_fd);
                assert(fcheck == 1);
                blockToDel = inDir;
            } else {                        // there is a direct block to delete
               blockToDel = inode->getDirectBlock(i-1);
            }
            deleteBlock(blockToDel);
        }
        if(singleInDirect>=0)               // there is a single in direct block to delete
            deleteBlock(singleInDirect);
        delete inode;
        fflush(sim_disk_fd);
        MainDir.erase(FileName);
        int i = 0;
        for (auto it = begin (OpenFileDescriptors); it != end (OpenFileDescriptors); ++it, i++){
            if(it->getFileName().compare(FileName) == 0){
                it->setInUse(false);
                it->setFileName("\0");
                return i;
            }
        }
        return -1;
    }
};
    
int main() {
    int blockSize; 
	int direct_entries;
    string fileName;
    char str_to_write[DISK_SIZE];
    char str_to_read[DISK_SIZE];
    int size_to_read; 
    int _fd;
    int writed;

    fsDisk *fs = new fsDisk();
    int cmd_;
    while(1) {
        cin >> cmd_;
        switch (cmd_)
        {
            case 0:   // exit
				delete fs;
				exit(0);
                break;

            case 1:  // list-file
                fs->listAll();
                break;
            case 2:    // format
                cin >> blockSize;
				cin >> direct_entries;
                fs->fsFormat(blockSize, direct_entries);
                break;
          
            case 3:    // creat-file
                cin >> fileName;
                _fd = fs->CreateFile(fileName);
                cout << "CreateFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;
            
            case 4:  // open-file
                cin >> fileName;
                _fd = fs->OpenFile(fileName);
                cout << "OpenFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;
             
            case 5:  // close-file
                cin >> _fd;
                fileName = fs->CloseFile(_fd); 
                cout << "CloseFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;
           
            case 6:   // write-file
                cin >> _fd;
                cin >> str_to_write;
                writed = fs->WriteToFile( _fd , str_to_write , strlen(str_to_write) );
                cout << "Writed: " << writed << " Char's into File Descriptor #: " << _fd << endl;
                break;
          
            case 7:    // read-file
                cin >> _fd;
                cin >> size_to_read ;
                fs->ReadFromFile( _fd , str_to_read , size_to_read );
                cout << "ReadFromFile: " << str_to_read << endl;
                break;
           
            case 8:   // delete file 
                 cin >> fileName;
                _fd = fs->DelFile(fileName);
                cout << "DeletedFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;
            default:
                break;
        }
    }
} 

