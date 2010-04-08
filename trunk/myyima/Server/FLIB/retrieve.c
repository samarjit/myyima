/*******************************************************************************

                          Yima Personal Edition v1.0
                               January 10, 2003

                            Streaming Media System
                        http://streamingmedia.usc.edu

                        Department of Computer Science
                        Integrated Media Systems Center
                       University of Southern California
                             Los Angeles, CA 90089

    Developed by:

    Roger Zimmermann
    Cyrus Shahabi
    Kun Fu
    Sahitya Gupta
    Mehrdad Jahangiri
    Farnoush Banaei-Kashani
    Hong Zhu


Copyright 2003, University of Southern California. All Rights Reserved.

"This software is experimental in nature and is provided on an AS-IS basis only.
The University SPECIFICALLY DISCLAIMS ALL WARRANTIES, EXPRESS AND IMPLIED,
INCLUDING WITHOUT LIMITATION ANY WARRANTY AS TO MERCHANTIBILITY OR FITNESS FOR
A PARTICULAR PURPOSE.

This software may be reproduced and used for non-commercial purposes only,
so long as this copyright notice is reproduced with each such copy made."

*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <strings.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "FileInterface.h"
#include "../sysinclude/OSDebug.h"
#include "../sysinclude/OSDefines.h"

static char directory[BLOCKNAMESIZE];

static MOVIE *Movie = NULL;

using namespace std;

/* BlockName should end with digits. Otherwise, this function returns 0. */
static int getBlockNum(const char* BlockName)
{
	char *TokenPtr;

	if (BlockName && (TokenPtr = strrchr(BlockName,'_')) ) {
		return atoi(TokenPtr+1);
	}
	return 0;
}

static int InitDisks()
{
	FILE* cfgfile;
	char key[BLOCKNAMESIZE];

	memset(key, 0, BLOCKNAMESIZE);

	if ((cfgfile=fopen(ConfigFile,"r"))==NULL) {
		printf("InitDisks: Cannot open config file for this movie\n");
		return -1;
	}

	while (!feof(cfgfile)) {
		// configuration file should start with keyword "Disk".
		// otherwise, default directory that stores media files will be set.
		if ((fscanf(cfgfile,"%s\n",key)) == 1) {
			if (!strcmp(key,"Disk")) {
				if ((fscanf(cfgfile,"%s\n",directory)) != 1) {
					printf("Error reading value for directory from config file\n");
					ASSERT(0);
				}
			}
			else {
				fgets(key,BLOCKNAMESIZE,cfgfile);
			}
		}
	}
	 
	if (directory[0] == '\0') { 
		// if directory is not set, just use default directory path.
		strncpy(directory,DIRECTORY,BLOCKNAMESIZE);
	}

	return 0;
}

static int InitMovies()
{
	FILE* cfgfile;
	char key[BLOCKNAMESIZE];
	char name[BLOCKNAMESIZE];
	UInt64 size; 
	int duration; // in seconds
	MOVIE *Movie_Curr = NULL;
	MOVIE *Movie_Temp = NULL;

	if ((cfgfile=fopen(ConfigFile,"r"))==NULL) {
		printf("InitMovies: Cannot open config file for this movie\n");
		return -1;
	}

	memset(key, 0, BLOCKNAMESIZE);
	memset(name, 0, BLOCKNAMESIZE);

	while (!feof(cfgfile)) {
		if ((fscanf(cfgfile,"%s\n",key)) == 1) {
			// for every movie item registered in the configuration file
			if (!strcmp(key,"Movie")) {
				// read movie information
				if ((fscanf(cfgfile,"%s %llu %d\n",name,&size,&duration))==3) {

			
					// it may make more sense to examine 
					// whether the same movie name exists
					// in the movie linked list.
				

					Movie_Temp = (MOVIE *)malloc(sizeof(MOVIE));
					if (!Movie_Temp) {
						printf("InitMovies: Cannot allocate memory for Movie_Temp\n");
						ASSERT(0);
					}
					// copy movie information to a MOVIE structure, 
					// that will be added to a linked list later.
					strncpy(Movie_Temp->name,name,BLOCKNAMESIZE); // movie name


					// TODO:
					// fill out every data field in MOVIE data structure
					printf("\nTODO: incomplete during the movie intializataion. partially done\n");
					Movie_Temp->size = size;
					Movie_Temp->numpkts = NUMPKTS;
					Movie_Temp->pktsize = MAX_PKTSIZE;
					Movie_Temp->time = duration;
					FILE *fpblockmeta;
					char packetsizedata[3000]; //=500('pkts')*5(sizze of num)+500(',')
					int numpacketperblk;
					int block_no;
					int ipacketcounter;
					char movie_block_meta_filename[BLOCKNAMESIZE];
					char temp_packet_offset[6];
					sprintf(movie_block_meta_filename,"%s_blk_meta.txt",name);
					fpblockmeta = fopen(movie_block_meta_filename,"r");
					strcpy(Movie_Temp->blkmetafilename, movie_block_meta_filename);
					printf("Block metadata filename:%s",Movie_Temp->blkmetafilename);
					while((fscanf(fpblockmeta,"%d %d %s\n",&block_no,&numpacketperblk,packetsizedata))==3){
						Movie_Temp->block_dtl_ar[block_no].nPackets = numpacketperblk;
						Movie_Temp->num_blocks = Movie_Temp->num_blocks + 1;
						//printf("\nInitMovie:block deatils blkNo:%d Pkts per blk:%d",block_no,numpacketperblk);
						strcpy(temp_packet_offset, strtok(packetsizedata,","));
						ipacketcounter = 0;
						Movie_Temp->block_dtl_ar[block_no].packet_lens_array[ipacketcounter]= atoi(temp_packet_offset);
						while((++ipacketcounter) < numpacketperblk){
							strcpy(temp_packet_offset,  strtok(NULL,","));
							Movie_Temp->block_dtl_ar[block_no].packet_lens_array[ipacketcounter]= atoi(temp_packet_offset);

						}

					}
					fclose(fpblockmeta);
					Movie_Temp->next = NULL;

					// attach newly generated MOVIE to the end of the MOVIE list.
					if (!Movie) {
						Movie = Movie_Temp;
						Movie_Curr = Movie_Temp;
					}
					else {
						Movie_Curr->next = Movie_Temp;
						Movie_Curr = Movie_Curr->next;
					}
				}
				else {
					printf("InitMovies: Error initializing...\n");
					fclose(cfgfile);
					return -1;
				}
			}
			else {
				fgets(key,BLOCKNAMESIZE,cfgfile);
			}
		}
	}
	fclose(cfgfile);
	return 0;   
}


extern "C" MOVIE *getMovieDetails(const char *movieName)
{
	MOVIE* Movie_Curr = Movie;
	while (Movie_Curr) {
		if (!strcmp(Movie_Curr->name,movieName)) {
			return Movie_Curr;
		}
		Movie_Curr = Movie_Curr->next;
	}
	return NULL; 
}


extern "C" int InitFileSystem()
{
	if (InitDisks()==-1) {
		printf("InitFileSystem: Error occurred initializing directory...\n");
		return -1;
	}
  
	if (InitMovies()==-1) {
		printf("InitFileSystem: Error occurred initializing movies...\n");
		return -1;
	}

	printf("Max number of RTP packets per block file = %d\n", NUMPKTS);
	printf("Max RTP length = %d\n", MAX_PKTSIZE);
	printf("Content repository = %s\n", directory);
	printf("Configuration fie = %s\n", ConfigFile);
	printf("\n");

	return 0;
}


extern "C" int getStartBlockName(const char* movieName,
				 float start_time,
				 char* blockName)
{
	MOVIE *m;

	m = getMovieDetails(movieName);
	if (!m) {
		printf("getStartBlockName: Movie %s: Not Found\n", movieName);
		return -1;
	}
	if (start_time > m->time) {
		printf("getStartBlockName: Movie is of shorter duration than the requested time\n");
		return -1;
	}

	// TODO:
	// map the given start_time to its corresponding block number correctly.
	char temp[BLOCKNAMESIZE];

	strcpy(temp,m->name);
	sprintf(blockName,"%s%s_1",directory,temp);
	printf("\nTODO: incomplete at getStartBlockName() now gets irrespective of timestamp\n");

	return 0; 
}


extern "C" int getNextBlockName(const char* movieName, 
  		     const char* blockName, 
	             char* nextblockName)
{
	MOVIE *m;
	int CurrentBlockNumber;

	m = getMovieDetails(movieName);
	if (!m || !blockName) {
		printf("getNextBlockName: Movie Not Found\n");
		return -1;
	}

	CurrentBlockNumber = getBlockNum(blockName) + 1;

	// TODO:
	// if there exists next block file to read,
	// set the block name at "nextblockName" and returns 0
	// otherwise, set "nextblockName[0] = 0 and returns -1
	sprintf(nextblockName,"%s_%d",movieName,CurrentBlockNumber);
	FILE *fp = fopen(nextblockName,"r");
	if(fp != NULL){
		fclose(fp);
	}else{nextblockName[0] = 0;
	return -1;
	}
	printf("\nTODO: incomplete at getNextBlockName() error condition implemented\n");


	return 0;
}


extern "C" int getNumPackets_PackSize(	const char* movieName, const char* blockName,
				int* numpkts,
				int* pkt_lens_in_a_block)
{
	MOVIE *m;
        int BlockNum;

	m = getMovieDetails(movieName);
	if (!m) {
		printf("getNumPackets_PackSize: Movie Not Found\n");
		return -1;
	}
  
	BlockNum = getBlockNum(blockName);
	//printf("getNumPackets_PackSize: getting blocks of no. %d",BlockNum);
	// TODO:
	// If the given Block file name is incorrect,
	// clean up the output values (numpkt and pktsize) in particular.
	// If the given Block file is the last block,
	// configure "numpkt" and "pktsize" carefully.
	// NOTE that pktsize may be given as a NULL pointer.
	int i;

			for(i=0;i< m->block_dtl_ar[BlockNum].nPackets;i++){
				pkt_lens_in_a_block[i] =  m->block_dtl_ar[BlockNum].packet_lens_array[i];
			//	printf("pkt_lens_in_a_block[%d]=%d",i,pkt_lens_in_a_block[i]);
			}
	if(BlockNum  ==  m->numpkts){
		//*pkt_lens_in_a_block = m->size - m->pktsize * m->numpkts;

		printf("getNumPackets_PackSize:End of a block reached, what to do?");
	}else{
		//*pkt_lens_in_a_block = m->pktsize;
	}
	*numpkts =  m->block_dtl_ar[BlockNum].nPackets; //just for the time being
	printf("<getNumPackets_PackSize>BlockName%s blk_num:%d packet per blk:%d",blockName,BlockNum, m->block_dtl_ar[BlockNum].nPackets);

	printf("\nTODO: incomplete at getNumPackets_PackSize()...\n");

	
	return 0;  
}

// read one block from a disk.
extern "C" int readBlock( const char* movieName, const char* blockName, 
			  char* buff_addr, 
		 	  int* numpkts, 
			  int* pkt_lens_in_a_block)
{
	struct stat stats;
	int blockfile;

	if (getNumPackets_PackSize(movieName,blockName,numpkts,pkt_lens_in_a_block)==-1) {
		printf("readBlock: Could not execute getNumPackets_PackSize\n");
		return -1;
	}

	if (stat(blockName, &stats) < 0) {
		printf("readBlock: cannot read the status of %s\n", blockName);
	}
	 
	if ((blockfile=open(blockName,O_RDONLY))==-1) {
		printf("readBlock: Error opening block file: %s\n", blockName);
	}
	  
	if (read(blockfile,buff_addr,(int)stats.st_size)==-1) {
		printf("readBlock: Error reading from block file %s\n", blockName);
	}
	close(blockfile);

	return 0;
}
