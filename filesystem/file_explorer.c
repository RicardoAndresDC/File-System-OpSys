#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//struct for reading instructions
typedef struct
{
	char** tokens;
	int numTokens;
} instruction;


//structure for reading in the fat information
typedef struct FATStruct
{
// Boot Sector and BPB Structure
	unsigned char JmpBoot[3]; // = 0xEB
	unsigned char BS_OEMName[8];
	unsigned short BPB_BytesPerSec;
	unsigned char BPB_SecPerClus;
	unsigned short BPB_RsvdSecCnt; // value typically 32
	unsigned char BPB_NumFATs; // should always containt hte value 2
	unsigned short BPB_RootEntCnt;// set to 0 
	unsigned short BPB_TotSec16;// must be 0 and BPB_TotSec32 != 0
	unsigned char BPB_Media; // = 0xF8
	unsigned short BPB_FATSz16; // must be 0, and BPB_FATSz32 contains
				    // the FAT size count
	unsigned short BPB_SecPerTrk;
	unsigned short BPB_NumHeads;
	unsigned int BPB_HiddSec;
	unsigned int BPB_TotSec32; // must != 0
// FAT32 Structure Starting at Offset 36
	unsigned int BPB_FATSz32;
	unsigned short BPB_ExtFlags;
	unsigned short BPB_FSVer;
	unsigned int BPB_RootClus; // usually 2 but not always
	unsigned short BPB_FSInfo; // usually 1
	unsigned short BPB_BkBootSec; // usually 6, no other value is recommened 
	unsigned char BPB_Reserved[12]; // always 0 for future updates
	unsigned char BS_DrvNum;
	unsigned char BS_Reserved1;
	unsigned char BS_BootSig;
	unsigned int BS_VolID;
	unsigned char BS_VolLab[11];
	unsigned char BS_FilSysType[8]; // always set to the string FAT32


}__attribute__((packed)) FATStruct;

//structure for basic file layout
typedef struct dirstruct
{
	unsigned char DIR_Name[11];
	unsigned char DIR_Attr;
	unsigned char DIR_NTRes;
	unsigned char DIR_CrtTimeTenth;
	unsigned short DIR_CrtTime;
	unsigned short DIR_CrtDate;
	unsigned short DIR_LstAccDate;
	unsigned short DIR_FstClusHI;
	unsigned short DIR_WrtTime;
	unsigned short DIR_WrtDate;
	unsigned short DIR_FstClusLO;
	unsigned int DIR_FileSize;
}__attribute__((packed)) dirstruct;

//global variables for ease of access in functions
dirstruct direct_struct;
FATStruct fatFile;	//Structure to hold FAT info
int root_address = 0;	//will hold address of root directory
int cur_location = 0;	//will hold current address
FILE * imageFile;	//file pointer for data sector traversal
FILE * fatstart;	//file pointer to start of fat table
FILE * lsptr;		//file pointer for functions in general
instruction instr;	//holds instruction, global for ease of access.
char cluster_read[4];	//array to read clusters from fat	
int fat_re_start;	//holds the start of the fat sector

//handle instruction parsing
instruction getTokens();
void addToken(instruction* instr_ptr, char* tok);
void printTokens(instruction* instr_ptr);
void clearInstruction(instruction* instr_ptr);
void addNull(instruction* instr_ptr);

//print directory name
void printinfo();

//other function declarations for commands
void list_structure(char * dir);
void change_directory(char * dir);
void size_of_file(char * filename);
void create_file(char * filename);


int main(int argc, char *argv[])
{
	//checks for correct usage
	if(argc != 2)
	{
		printf("Invalid usage: File_Explorer.exe [path to fat32 image]\n");
		return 1;
	}
	
	//general declarations and initializations
	int i = 0;
	instr.tokens = NULL;
	instr.numTokens = 0;
	
	//gets fat32 file structure
	imageFile = fopen(argv[1], "rb+");
	lsptr = fopen(argv[1], "rb+");
	if(imageFile == NULL)
	{
		printf("%s cannot be opened!\n", argv[1]);
		return 1;
	}
	fread(&fatFile, sizeof(FATStruct), 1, imageFile);
	dirstruct contentsCurr[10];
	

	//track location of fat region
	fat_re_start = fatFile.BPB_BytesPerSec * fatFile.BPB_RsvdSecCnt;
	fatstart = fopen(argv[1], "rb+");
	fseek(fatstart, fat_re_start, SEEK_SET);
	//sets a second file pointer to the start of the fat

	//variables for file data
	int FDS = 0;
	int RDS = 0;				
	int FSC = 0;
	int n = 0;

	// RDS is the RootDirSectors, the number of sectors in the root directory
	RDS = ((fatFile.BPB_RootEntCnt * 32) + (fatFile.BPB_BytesPerSec - 1))
		 / fatFile.BPB_BytesPerSec;

	// finds address of first data sector
	FDS = fatFile.BPB_RsvdSecCnt + 
		(fatFile.BPB_NumFATs * fatFile.BPB_FATSz32) + RDS;
	n = fatFile.BPB_RootClus;

	// find start or root in data sector. Equation
	FSC = (((n-2) * fatFile.BPB_SecPerClus) + FDS) * 
		(fatFile.BPB_BytesPerSec * fatFile. BPB_SecPerClus);
		
	//root address will always hold the address of the root sector 
	root_address = FSC;
	
	//keeps track of current location in the system starting at the root
	cur_location = FSC;

	fseek(imageFile, FSC, SEEK_SET); //set imageFile to root



	
	while (1) 
	{
		printf("Please enter an instruction:");
		
		instr = getTokens();

		if(strcmp(instr.tokens[0], "exit") == 0)
		{
			exit(0);
		}
		else if(strcmp(instr.tokens[0], "info") == 0){
			printf("BS_jmpBoot: %X %X %X\n", fatFile.JmpBoot[0],
				 fatFile.JmpBoot[1], fatFile.JmpBoot[2]);
			printf("BS_OEMName: ");
			for(i = 0; i < 8; i++)
				printf("%X ", fatFile.BS_OEMName[i]);
			printf("\n");
			printf("BPB_BytsPerSec: %d \n", fatFile.BPB_BytesPerSec);
			printf("BPB_SecPerClus: %X \n", fatFile.BPB_SecPerClus);
			printf("BPB_RsvdSecCnt: %d \n", fatFile.BPB_RsvdSecCnt);
			printf("BPB_NumFATs: %X \n", fatFile.BPB_NumFATs);
			printf("BPB_RootEntCnt: %d \n", fatFile.BPB_RootEntCnt);
			printf("BPB_TotSec16: %d \n", fatFile.BPB_TotSec16);
			printf("BPB_Media: %X \n", fatFile.BPB_Media);
			printf("BPB_FATSz16: %d \n", fatFile.BPB_FATSz16);
			printf("BPB_SecPerTrk: %d \n", fatFile.BPB_SecPerTrk);
			printf("BPB_NumHeads: %d \n", fatFile.BPB_NumHeads);
			printf("BPB_HiddSec: %d \n", fatFile.BPB_HiddSec);
			printf("BPB_TotSec32: %d \n", fatFile.BPB_TotSec32);
			printf("BPB_FATSz32: %d \n", fatFile.BPB_FATSz32);
			printf("BPB_ExtFlags: %d \n", fatFile.BPB_ExtFlags);
			printf("BPB_FSVer: %d \n", fatFile.BPB_FSVer);
			printf("BPB_RootClus: %d \n", fatFile.BPB_RootClus);
			printf("BPB_FSInfo: %d \n", fatFile.BPB_FSInfo);
			printf("BPB_BkBootSec: %d \n", fatFile.BPB_BkBootSec);
			printf("BPB_Reserved: ");
			for(i = 0; i < 12; i++)
				printf("%X ", fatFile.BPB_Reserved[i]);
			printf("\n");
			printf("BS_DrvNum: %X \n", fatFile.BS_DrvNum);
			printf("BS_Reserved1: %X \n", fatFile.BS_Reserved1);
			printf("BS_BootSig: %X \n", fatFile.BS_BootSig);
			printf("BS_VolID: %d \n", fatFile.BS_VolID);
			printf("BS_VolLab: ");
			for(i = 0; i < 11; i++)
				printf("%c", fatFile.BS_VolLab[i]);
			printf("\n");
			printf("BS_FilSysType: ");
			for(i = 0; i < 8; i++)
				printf("%c", fatFile.BS_FilSysType[i]);
			printf("\n");
			
		}
		else if(strcmp(instr.tokens[0], "ls") == 0)
		{
			list_structure(instr.tokens[1]);
		}
		else if(strcmp(instr.tokens[0], "cd") == 0){
			change_directory(instr.tokens[1]);
		}
		else if(strcmp(instr.tokens[0], "size") == 0){
			if(instr.numTokens != 3)
			{
				printf("Error: Too Many Arguments!\n");
			}
			else
			{
				size_of_file(instr.tokens[1]);
			}
		}
		else if(strcmp(instr.tokens[0], "creat") == 0){
			if(instr.numTokens != 3)
				printf("Error: Too Many Arguments!\n");
			else
			{
				create_file(instr.tokens[1]);
			}
		}
		else if(strcmp(instr.tokens[0], "mkdir") == 0){
			printf("Command \'mkdir\' not implemented.\n");
		}
		else if(strcmp(instr.tokens[0], "open") == 0){
			printf("Command \'open\' not implemented.\n");
		}
		else if(strcmp(instr.tokens[0], "close") == 0){
			printf("Command \'close\' not implemented.\n");
		}
		else if(strcmp(instr.tokens[0], "read") == 0){
			printf("Command \'read\' not implemented.\n");
		}
		else if(strcmp(instr.tokens[0], "write") == 0){
			printf("Command \'write\' not implemented.\n");
		}
		else if(strcmp(instr.tokens[0], "rm") == 0){
			printf("Command \'rm\' not implemented.\n");
		}
		else if(strcmp(instr.tokens[0], "rmdir") == 0){
			printf("Command \'rmdir\' not implemented.\n");
		}
		else
		{
			printf("Invalid command!\n");
		}


		//free and reset variables
		clearInstruction(&instr);
		
	}


	//close out files
	fclose (imageFile);
	fclose(lsptr);
	fclose(fatstart);
	clearInstruction(&instr);

	return 0;
}


//Functions in order to run commands

//runs the ls command
void list_structure(char * dir)
{
	//declares the variables
	int temp_location = 0;
	char * tempName;
	
	//runs if no parameter passed
	if(dir == NULL)
	{
		//tracks location and moves file pointer
		temp_location = cur_location;
		fseek(imageFile, temp_location, SEEK_SET);
		
		
		//variables for loops and checking if broken early
		int broken = 0;
		int i = 0;
		
		//if you start in root due to lack of . and ..
		if(temp_location == root_address)
		{
			for(i=0; i<8;i++)
			{
				fseek(imageFile, 32, SEEK_CUR);
				fread(&direct_struct, sizeof(dirstruct),
					 1, imageFile);
				if(direct_struct.DIR_Name[0] == 0)
				{
					broken = 1;
					break;
				}
				direct_struct.DIR_Name[10] = '\0';
				printinfo();
			}
		}
		else
		{
			//reads in . and .. followed by rest of cluster
			fread(&direct_struct, sizeof(dirstruct), 1, imageFile);
			printinfo();
			fread(&direct_struct, sizeof(dirstruct), 1, imageFile);
			printinfo();
			for(i=0; i<7;i++)
			{
				fseek(imageFile, 32, SEEK_CUR);
				fread(&direct_struct, sizeof(dirstruct),
					 1, imageFile);
				if(direct_struct.DIR_Name[0] == 0)
				{
					broken = 1;
					break;
				}
				direct_struct.DIR_Name[10] = '\0';
				printinfo();
			}
			
		}
		
		//if the ls doesnt end early move on to the next cluster
		// and continue printing
		if(broken == 0)
		{
			//moves to start of fat to start looking for next cluster
			fseek(fatstart, fat_re_start, SEEK_SET);
			int end_search = 0;
			int temp_cluster = ((temp_location - root_address)
				 / (fatFile.BPB_BytesPerSec * 
				 fatFile.BPB_SecPerClus)) + 2;
				 
			
			
			while(end_search == 0 && broken == 0)
			{
				//tracks current location
				fseek(lsptr, temp_location, SEEK_SET);
				for(i = 0; i < temp_cluster+1; i++)
				{
					//reads up to next cluster
					fread(&cluster_read, sizeof(char),
						 4, fatstart);
				}
				
				//ends cluster or moves on depending on hi
				if(cluster_read[1] == 255)
					end_search = 1;
				else
				{
					//acquires cluster for next section 
					//and calculates location for reading
					int temp = cluster_read[1] << 8;
					int temp2 = 256 + cluster_read[0];
					temp = temp + temp2;
					
					int bytesadded = ((temp - 2) * 
						fatFile.BPB_BytesPerSec * 
						fatFile.BPB_SecPerClus)
						+ root_address;
						
					fseek(lsptr, bytesadded, SEEK_SET);
					temp_location = bytesadded;
					for(i=0; i<8;i++)
					{
						//runs through new cluster
						fseek(lsptr, 32, SEEK_CUR);
						fread(&direct_struct,
						   sizeof(dirstruct), 1, lsptr);
						if(direct_struct.DIR_Name[0] == 0)
						{
							broken = 1;
							break;
						}
						direct_struct.DIR_Name[10] = '\0';
						printinfo();
					}
					if(broken == 0)
					{
						temp_cluster = ((temp_location - 
						   root_address) / 
						   (fatFile.BPB_BytesPerSec * 
						   fatFile.BPB_SecPerClus)) + 2;
						fseek(fatstart, 
						   fat_re_start, SEEK_SET);
					}
				}
			}
		}
		
		fseek(imageFile, cur_location, SEEK_SET);
	}
	else
	{	//for a parameter given jumps to directory and runs ls then jumps
		//back and resets location
		temp_location = cur_location;
		change_directory(dir);
		list_structure((char*)NULL);
		cur_location = temp_location;
	}
	
	
}

//Function for changing directories
void change_directory(char * dir)
{
	if(dir == NULL)	//only cd no parameters
	{
		//returns to root
		cur_location = root_address;
		fseek(imageFile, root_address, SEEK_SET);
	}
	else
	{
		//sets variables to find directory
		int i = 0;
		int unfound = 0;
		int found = 0;
		fseek(lsptr, cur_location, SEEK_SET);
		
		if(cur_location == root_address)
		{
			//tracks down directory with matching name
			for(i=0; i<8;i++)
			{
				fseek(lsptr, 32, SEEK_CUR);
				fread(&direct_struct, sizeof(dirstruct), 1, lsptr);
				
				if(direct_struct.DIR_Name[0] == 0)
				{
					//if no name then last entry so ends
					printf("Error: Invalid Directory Name\n");
					unfound = 1;
					break;
				}
				direct_struct.DIR_Name[10] = '\0';
				
				if(strncmp(dir, direct_struct.DIR_Name, strlen(dir)) == 0)
				{
					if(direct_struct.DIR_Attr != 16)
					{
						//if it is not a directory
						//does not change
						printf("Error: Invalid Directory Name\n");
						found = 1;
						break;
					}
					
					//if it is a directory moves to its
					//cluster
					int temp = direct_struct.DIR_FstClusHI
						 << 16;
					temp = temp + 
						direct_struct.DIR_FstClusLO;
					int bytesadded = ((temp - 2) * 
						fatFile.BPB_BytesPerSec * 
						fatFile.BPB_SecPerClus) + 
						root_address;
						
					fseek(imageFile, bytesadded, SEEK_SET);
					cur_location = bytesadded;
					found = 1;
					break;
				}
			}
		}
		else
		{
			//tracks down directory with matching names
			fread(&direct_struct, sizeof(dirstruct), 1, lsptr);
			direct_struct.DIR_Name[10] = '\0';
			
			if(strncmp(dir, direct_struct.DIR_Name, strlen(dir)) == 0)
			{
				//moves to found directory
				int temp = direct_struct.DIR_FstClusHI << 16;
				temp = temp + direct_struct.DIR_FstClusLO;
				int bytesadded = ((temp - 2) * 
					fatFile.BPB_BytesPerSec * 
					fatFile. BPB_SecPerClus) + root_address;
					
				fseek(imageFile, bytesadded, SEEK_SET);
				cur_location = bytesadded;
				found = 1;
			}
			if(found != 1)
			{
				fread(&direct_struct, sizeof(dirstruct), 1, lsptr);
				direct_struct.DIR_Name[10] = '\0';
				if(strncmp(dir, direct_struct.DIR_Name,
					strlen(dir)) == 0)
				{
					
					//checks directory and if valid then
					//moves to it
					if(direct_struct.DIR_Attr == 16)
					{
						found = 1;
						printf("Error: Invalid Directory Name!\n");
					}
					int temp = direct_struct.DIR_FstClusHI 
						<< 16;
					temp = temp + direct_struct.DIR_FstClusLO;
					int bytesadded = ((temp) * 
						fatFile.BPB_BytesPerSec * 
						fatFile. BPB_SecPerClus) + 
						root_address;
						
					fseek(imageFile, bytesadded, SEEK_SET);
					cur_location = bytesadded;
					found = 1;
				}
			}

			if(found != 1)
			{
				for(i=0; i<7;i++)
				{
					//reads through additional directories
					fseek(lsptr, 32, SEEK_CUR);
					fread(&direct_struct, 
						sizeof(dirstruct), 1, lsptr);
					if(direct_struct.DIR_Name[0] == 0)
					{
						printf("Error: Invalid Directory Name\n");
						unfound = 1;
						break;
					}
					direct_struct.DIR_Name[10] = '\0';
					if(strncmp(dir, direct_struct.DIR_Name, 
						strlen(dir)) == 0)
					{
						//moves to directory if valid
						if(direct_struct.DIR_Attr == 16)
						{
							found = 1;
							printf("Error: Invalid Directory Name!\n");
							break;
						}
						
						int temp = direct_struct.DIR_FstClusHI << 16;
						temp = temp + 
						    direct_struct.DIR_FstClusLO;
						    
						int bytesadded = ((temp - 2) * 
						    fatFile.BPB_BytesPerSec * 
						    fatFile. BPB_SecPerClus) + 
						    root_address;
						    
						fseek(imageFile, 
						    bytesadded, SEEK_SET);
						    
						cur_location = bytesadded;
						found = 1;
						break;
					}
				}
			}
			
		}
		
		if(found == 0 && unfound == 0)
		{
			//begins tracking to find more clusters
			int temp_location = cur_location;
			fseek(fatstart, fat_re_start, SEEK_SET);
			int end_search = 0;
			int temp_cluster = ((temp_location - root_address) / 
				(fatFile.BPB_BytesPerSec * fatFile.BPB_SecPerClus)) + 2;
				
			int i = 0;
			
			while(end_search == 0 && found == 0 && unfound == 0)
			{
				//moves to location and reads to cluster
				fseek(lsptr, temp_location, SEEK_SET);
				for(i = 0; i < temp_cluster+1; i++)
				{
					fread(&cluster_read, sizeof(char), 4, fatstart);
				}
				
				//ends if cluster is last
				if(cluster_read[1] == 255)
					end_search = 1;
				else
				{
					//moves to appropriate cluster and begins searching for
					//valid directory
					int temp = cluster_read[1] << 8;
					int temp2 = 256 + cluster_read[0];
					temp = temp + temp2;
					printf("%d\n", temp);
					int bytesadded = ((temp-2) * fatFile.BPB_BytesPerSec * 
						fatFile.BPB_SecPerClus) + root_address;
					fseek(lsptr, bytesadded, SEEK_SET);
					temp_location = bytesadded;
					
					//searches for the directoyr
					for(i=0; i<8;i++)
					{
						fseek(lsptr, 32, SEEK_CUR);
						fread(&direct_struct, sizeof(dirstruct), 1, lsptr);
						if(direct_struct.DIR_Name[0] == 0)
						{
							printf("ERROR: Invalid Directory Name!\n");
							unfound = 1;
							break;
						}
						direct_struct.DIR_Name[10] = '\0';
						if(strncmp(dir, direct_struct.DIR_Name, strlen(dir)) == 0)
						{
							//reads cluster for file and moves to it
							if(direct_struct.DIR_Attr == 16)
							{
								found = 1;
								printf("Error: Invalid Directory Name!\n");
								break;
							}
							int temp = direct_struct.DIR_FstClusHI << 16;
							temp = temp + direct_struct.DIR_FstClusLO;
							
							int bytesadded = ((temp-2) * fatFile.BPB_BytesPerSec
								 * fatFile.BPB_SecPerClus) + root_address;
								 
							fseek(imageFile, bytesadded, SEEK_SET);
							cur_location = bytesadded;
							found = 1;
							break;
						}
					}
					if(unfound == 0)
					{
						//moves files to begin next loop of tracking
						temp_cluster = ((temp_location - root_address) / 
							(fatFile.BPB_BytesPerSec * 
							fatFile.BPB_SecPerClus)) + 2;
						fseek(fatstart, fat_re_start, SEEK_SET);
					}
				}
			}
			
		}
		else if(found == 0 && unfound == 1)
		{
			printf("Error: Invalid Directory Name\n");
		}
	}
}

//function to print directory name, was originally used for testing that
//directory info read in correctly
void printinfo()
{
	printf("%s", direct_struct.DIR_Name);
	printf("\n");
}


//Function finds size of a file
void size_of_file(char * filename)
{
	//variables for tracking down filename
	int i = 0;
	int unfound = 0;
	int found = 0;
	fseek(lsptr, cur_location, SEEK_SET);
	
	//if in root triggers this
	if(cur_location == root_address)
	{
		//searches each entry for the file and if found prints size or breaks if
		//no entries left
		for(i=0; i<8;i++)
		{
			fseek(lsptr, 32, SEEK_CUR);
			fread(&direct_struct, sizeof(dirstruct), 1, lsptr);
			if(direct_struct.DIR_Name[0] == 0)
			{
				printf("Error: Invalid File Name\n");
				unfound = 1;
				break;
			}
			direct_struct.DIR_Name[10] = '\0';
			if(strncmp(filename, direct_struct.DIR_Name, strlen(filename)) == 0)
			{
				printf("%s is %d bytes.\n", filename, direct_struct.DIR_FileSize);
				found = 1;
				break;
			}
		}
	}
	else
	{
		//checks the . and .. files to see if they were passd in
		fread(&direct_struct, sizeof(dirstruct), 1, lsptr);
		direct_struct.DIR_Name[10] = '\0';
		if(strncmp(filename, direct_struct.DIR_Name, strlen(filename)) == 0)
		{
			
			
			printf("%s is %d bytes.\n", filename, direct_struct.DIR_FileSize);
			found = 1;
		}
		
		if(found != 1)
		{
			fread(&direct_struct, sizeof(dirstruct), 1, lsptr);
			direct_struct.DIR_Name[10] = '\0';
			if(strncmp(filename, direct_struct.DIR_Name, strlen(filename)) == 0)
			{
				
				printf("%s is %d bytes.\n", filename, direct_struct.DIR_FileSize);
				found = 1;
			}
		}

		// if not . or .. trigger
		if(found != 1)
		{
			//reads in start cluster until found, no entries remain, or must move
			//onto next cluster
			for(i=0; i<7;i++)
			{
				fseek(lsptr, 32, SEEK_CUR);
				fread(&direct_struct, sizeof(dirstruct), 1, lsptr);
				if(direct_struct.DIR_Name[0] == 0)
				{
					printf("Error: Invalid File Name\n");
					unfound = 1;
					break;
				}
				direct_struct.DIR_Name[10] = '\0';
				if(strncmp(filename, direct_struct.DIR_Name, strlen(filename)) == 0)
				{
					
					printf("%s is %d bytes.\n", filename, 
						direct_struct.DIR_FileSize);
					found = 1;
					break;
				}
			}
		}
		
	}
	
	//triggers if it needs to check for more clusters
	if(found == 0 && unfound == 0)
	{
		//tracks locations and searches for clusters
		int temp_location = cur_location;
		fseek(fatstart, fat_re_start, SEEK_SET);
		int end_search = 0;
		int temp_cluster = ((temp_location - root_address) / 
			(fatFile.BPB_BytesPerSec * fatFile.BPB_SecPerClus)) + 2;
		int i = 0;
		
		while(end_search == 0 && found == 0 && unfound == 0)
		{
			//sets ptr to the new location and then reads the fat section to 
			//find the next cluster
			fseek(lsptr, temp_location, SEEK_SET);
			for(i = 0; i < temp_cluster+1; i++)
			{
				fread(&cluster_read, sizeof(char), 4, fatstart);
			}
			
			//if next cluster continue otherwise break
			if(cluster_read[1] == 255)
				end_search = 1;
			else
			{
				//reads new cluster location and moves to it
				int temp = cluster_read[1] << 8;
				int temp2 = 256 + cluster_read[0];
				temp = temp + temp2;
				int bytesadded = ((temp-2) * fatFile.BPB_BytesPerSec * 
					fatFile.BPB_SecPerClus) + root_address;
				fseek(lsptr, bytesadded, SEEK_SET);
				temp_location = bytesadded;
				
				//searches new cluster for file and breaks if necessary
				for(i=0; i<8;i++)
				{
					fseek(lsptr, 32, SEEK_CUR);
					fread(&direct_struct, sizeof(dirstruct), 1, lsptr);
					if(direct_struct.DIR_Name[0] == 0)
					{
						printf("ERROR: Invalid File Name!\n");
						unfound = 1;
						break;
					}
					direct_struct.DIR_Name[10] = '\0';
					if(strncmp(filename, direct_struct.DIR_Name, strlen(filename)) == 0)
					{
						
						printf("%s is %d bytes.\n", filename, 
							direct_struct.DIR_FileSize);
						found = 1;
						break;
					}
				}
				if(unfound == 0)
				{
					//if not found and not ending early then moves to 
					//next cluster
					temp_cluster = ((temp_location - root_address) / 
						(fatFile.BPB_BytesPerSec * fatFile.BPB_SecPerClus)) + 2;
					fseek(fatstart, fat_re_start, SEEK_SET);
				}
			}
		}
		
	}
	else if(found == 0 && unfound == 1)
	{
		printf("Error: Invalid Directory Name\n");
	}
	
}	


//Function for creating a new file
void create_file(char * filename)
{
	//creates variables for possible moves to other clusters
	int free_entry = 0;
	int temp_location = cur_location;
	int found = 0;
	int broken = 0;
	int curr_cluster = ((cur_location-root_address) / 
		(fatFile.BPB_BytesPerSec * fatFile.BPB_SecPerClus)) + 2;
	
	//checks for break conditions and loops
	while(found != 1 && broken != 1)
	{
		//moves pointer to location and preps counter
		int i = 0;
		fseek(lsptr, temp_location, SEEK_SET);
		
		//loops over cluster to check for preexisting filename
		for(i = 0; i < 8; i++)
		{
			fseek(lsptr, 32, SEEK_CUR);
			fread(&direct_struct, sizeof(dirstruct), 1, lsptr);
			temp_location = temp_location + 64;
			
			//if empty entry places new entry there
			if(direct_struct.DIR_Name[0] == 0)
			{
				fseek(lsptr, -32, SEEK_CUR);
				temp_location = temp_location + 32;
				found = 1;
				dirstruct temp_file;   //temporary file to place
				strncpy(temp_file.DIR_Name, filename, 10);
				temp_file.DIR_Name[10] = '\0';
				temp_file.DIR_Attr = 32;
				temp_file.DIR_NTRes = 0;
				temp_file.DIR_CrtTimeTenth = 0;
				temp_file.DIR_CrtTime = 0;
				temp_file.DIR_CrtDate = 0;
				temp_file.DIR_FstClusHI = 0;
				temp_file.DIR_WrtTime = 0;
				temp_file.DIR_WrtDate = 0;
				temp_file.DIR_FstClusLO = 0;
				temp_file.DIR_FileSize = 0;
				fwrite(&temp_file, sizeof(dirstruct), 1, lsptr);
				break;
			}
			if(strncmp(filename, direct_struct.DIR_Name, 
				strlen(filename)) == 0)
			{
				//if file exists already then ends
				printf("Error: File already exists.\n");
				broken = 1;
				break;
			}
			
		}
		if(broken != 1 && found != 1)
		{
			//moves to next cluster if necessary
			fseek(fatstart, fat_re_start, SEEK_SET);
			int j = 0;
			for(j = 0; j < curr_cluster + 1; j++)
			{
				fread(&cluster_read, sizeof(char), 4, fatstart);
			}
			int temp_cluster = cluster_read[1] << 8;
			temp_cluster = cluster_read[0] + temp_cluster;
			temp_location = (temp_cluster - 2) * 
				(fatFile.BPB_BytesPerSec * 
				fatFile.BPB_SecPerClus) + root_address;
		}
	}
	
}




//Following code is taken from project1



//all the code was originally given to parse the commands that were given
instruction getTokens()
{
	char* token = NULL;
	char* temp = NULL;

	instruction instr;
	instr.tokens = NULL;
	instr.numTokens = 0;



	do {			// loop reads character sequences separated by whitespace
		scanf( "%ms", &token);									//scans for next token and allocates token var to size of scanned token
		temp = (char*)malloc((strlen(token)+1) * sizeof(char));	//allocate temp variable with same size as token

		int i;
		int start;

		start = 0;

		for (i = 0; i < strlen(token); i++)
		{
			//pull out special characters and make them into a separate token in the instruction
			if (token[i] == '|' || token[i] == '>' || token[i] == '<' || token[i] == '&') 
			{
				if (i-start > 0)
				{
					memcpy(temp, token + start, i - start);
					temp[i-start] = '\0';
					addToken(&instr, temp);						
				}

				char specialChar[2];
				specialChar[0] = token[i];
				specialChar[1] = '\0';

				addToken(&instr,specialChar);

				start = i + 1;
			}
		}
		if (start < strlen(token))
		{
			memcpy(temp, token + start, strlen(token) - start);
			temp[i-start] = '\0';
			addToken(&instr, temp);			
		}

		//free and reset variables
		free(token);
		free(temp);

		token = NULL;
		temp = NULL;

	} while ('\n' != getchar());    //until end of line is reached

	addNull(&instr);
	return instr;
	

}


//reallocates instruction array to hold another token
//allocates for new token within instruction array
void addToken(instruction* instr_ptr, char* tok)
{
	//extend token array to accomodate an additional token
	if (instr_ptr->numTokens == 0)
		instr_ptr->tokens = (char**)malloc(sizeof(char*));
	else
		instr_ptr->tokens = (char**)realloc(instr_ptr->tokens, (instr_ptr->numTokens+1) * sizeof(char*));

	//allocate char array for new token in new slot
	instr_ptr->tokens[instr_ptr->numTokens] = (char *)malloc((strlen(tok)+1) * sizeof(char));
	strcpy(instr_ptr->tokens[instr_ptr->numTokens], tok);

	instr_ptr->numTokens++;

}

void addNull(instruction* instr_ptr)
{
	//extend token array to accomodate an additional token
	if (instr_ptr->numTokens == 0)
		instr_ptr->tokens = (char**)malloc(sizeof(char*));
	else 
		instr_ptr->tokens = (char**)realloc(instr_ptr->tokens, (instr_ptr->numTokens+1) * sizeof(char*));

	instr_ptr->tokens[instr_ptr->numTokens] = (char*) NULL;

	instr_ptr->numTokens++;

}

void clearInstruction(instruction* instr_ptr)
{
	int i;
	for (i = 0; i < instr_ptr->numTokens; i++)
		free(instr_ptr->tokens[i]);
	free(instr_ptr->tokens);

	instr_ptr->tokens = NULL;
	instr_ptr->numTokens = 0;
}

void printTokens(instruction* instr_ptr)
{
	int i;
	printf("Tokens:\n");
	for (i = 0; i < instr_ptr->numTokens; i++)
	{
		if ((instr_ptr->tokens)[i] != NULL)
			printf("#%s#\n", (instr_ptr->tokens)[i]);
	}
}

	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	





















