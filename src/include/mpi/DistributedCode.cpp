//
// Created by frank on 1/15/25.
//

#include "DistributedCode.hpp"

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <dirent.h>
#include <mpi.h>
#include <vector>

#include "mpi_data.hpp"
#include "../ramfs/FileSystem.hpp"

using namespace std;

int DistributedCode::mpiRank = -1;
int DistributedCode::mpiWorldSize = 0;
string DistributedCode::fsPath = string();

DistributedCode *DistributedCode::instance = nullptr;
DataBlockManager *DistributedCode::dataBlockManager = nullptr;

DistributedCode *DistributedCode::getInstance(int rank, int worldSize, const char *mountpointPath) {
	if (instance == nullptr) {
		instance = new DistributedCode(rank, worldSize, mountpointPath);
	}

	return instance;
}

DistributedCode::DistributedCode(int rank, int worldSize, const char* mountpointPath) {
	mpiRank = rank;
	mpiWorldSize = worldSize;
	fsPath = mountpointPath;
	fsPath += "/"+to_string(mpiRank);
	dataBlockManager = DataBlockManager::getInstance(mpiWorldSize);
}


void DistributedCode::setup() {

}

void DistributedCode::start() {
	bool running = true;
	while (running) {
		//cout << "Process " << mpiRank << " - Waiting for a request" <<endl;
		RequestPacket request;
		MPI_Status status;
		MPI_Recv(&request,sizeof(RequestPacket), MPI_BYTE, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
		switch (request.type) {
		case WRITE:
			if (mpiRank != status.MPI_SOURCE) {
				//cout << "Process " << mpiRank << " - Invoking DAGonFS_Write()" <<endl;
				IORequestPacket ioRequest;
				MPI_Status status;
				MPI_Recv(&ioRequest, sizeof(IORequestPacket), MPI_BYTE, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
				cout << "Process " << mpiRank << " - Received WRITE from P"<<status.MPI_SOURCE<<": ioRequest.inode="<<ioRequest.inode<<", ioRequest.fileSize="<<ioRequest.fileSize<< endl;
				DAGonFS_Write(status.MPI_SOURCE, MPI_IN_PLACE, ioRequest.inode, ioRequest.fileSize);
			}
			break;
		case READ:
			if (mpiRank != status.MPI_SOURCE) {
				//cout << "Process " << mpiRank << " - Invoking DAGonFS_Read()" <<endl;
				IORequestPacket ioRequest;
				MPI_Status status;
				MPI_Recv(&ioRequest, sizeof(IORequestPacket), MPI_BYTE, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
				cout << "Process " << mpiRank << " - Received READ from P"<<status.MPI_SOURCE<<":"<< endl;
				cout << "\tioRequest.inode="<<ioRequest.inode<<endl;
				cout << "\tioRequest.fileSize="<<ioRequest.fileSize<<endl;
				cout << "\tioRequest.reqSize="<<ioRequest.reqSize<<endl;
				cout << "\tioRequest.offset="<<ioRequest.offset<<endl;
				DAGonFS_Read(status.MPI_SOURCE, ioRequest.inode, ioRequest.fileSize, ioRequest.reqSize, ioRequest.offset);
			}
			//DAGonFS_Read();
			break;
		case CREATE_FILE:
			if (mpiRank != status.MPI_SOURCE) {
				createFile();
			}
			break;
		case DELETE_FILE:
			if (mpiRank != status.MPI_SOURCE) {
				deleteFile();
			}
			break;
		case CREATE_DIR:
			if (mpiRank != status.MPI_SOURCE) {
				createDir();
			}
			break;
		case DELETE_DIR:
			if (mpiRank != status.MPI_SOURCE) {
				deleteDir();
			}
			break;
		case TERMINATE:
			cout << "Process " << mpiRank << " - Received termination request" <<endl;
			running = false;
			if (mpiRank != status.MPI_SOURCE) {
				unmountFileSystem();
			}
			break;
		default:
			break;
		}
	}
}

void DistributedCode::unmountFileSystem() {
	FileSystem::unmountFromThread = true;
	string unmountScript = fsPath + "/../unmount.sh " + fsPath;
	system(unmountScript.c_str());
}

void DistributedCode::DAGonFS_Write(int sourceRank, void *buffer, fuse_ino_t inode, size_t fileSize) {
	if (mpiRank == sourceRank) {
		IORequestPacket ioRequest;
		ioRequest.inode = inode;
		ioRequest.fileSize = fileSize;
		cout << "Process " << mpiRank << " - Notify all other process for writing operation: ioRequest.inode="<<ioRequest.inode<<", ioRequest.fileSize="<<ioRequest.fileSize<< endl;
		for (int i=0; i<mpiWorldSize; i++) {
			MPI_Request request;
			if (i != mpiRank) {
				MPI_Isend(&ioRequest,sizeof(IORequestPacket), MPI_BYTE, i, 0, MPI_COMM_WORLD, &request);
			}
		}
	}

	size_t numberOfBlocksForRequest = fileSize / FILE_SYSTEM_SINGLE_BLOCK_SIZE + (fileSize % FILE_SYSTEM_SINGLE_BLOCK_SIZE > 0);
	size_t blockPerProcess = numberOfBlocksForRequest / mpiWorldSize;
	size_t remainingBlocks = numberOfBlocksForRequest % mpiWorldSize;
	size_t effectiveBlocks = blockPerProcess + (mpiRank < remainingBlocks);

	//Scatter dei dati
	void *localScatBuf = malloc(effectiveBlocks*FILE_SYSTEM_SINGLE_BLOCK_SIZE);
	int *scatterCounts = new int[mpiWorldSize];
	int *scatterDispls = new int[mpiWorldSize];
	int scatterOffset = 0;
	for (int i=0; i<mpiWorldSize; i++) {
		//Calculating elements
		scatterCounts[i] = ( blockPerProcess + (i < remainingBlocks) ) * FILE_SYSTEM_SINGLE_BLOCK_SIZE;

		//Calculating displacements
		scatterDispls[i] = scatterOffset;
		scatterOffset += scatterCounts[i];
	}

	//Gather dei puntatori
	PointerPacket *localGathBuf = new PointerPacket[effectiveBlocks];
	int *gatherCounts = new int[mpiWorldSize];
	int *gatherDispls = new int[mpiWorldSize];
	int gatherOffset = 0;
	for (int i=0; i<mpiWorldSize; i++) {
		//Calculating elements
		gatherCounts[i] = ( blockPerProcess + (i < remainingBlocks) ) * sizeof(PointerPacket);

		//Calculating displacements
		gatherDispls[i] = gatherOffset;
		gatherOffset += gatherCounts[i];
	}

	if (mpiRank == sourceRank) {
		cout << "Process " << mpiRank << "=="<<sourceRank<<" - Sending Scatterv" << endl;
		MPI_Scatterv(buffer, scatterCounts, scatterDispls, MPI_BYTE, localScatBuf, scatterCounts[mpiRank], MPI_BYTE, sourceRank, MPI_COMM_WORLD);
	}
	else {
		cout << "Process " << mpiRank << "!="<<sourceRank<<" - Receiving Scatterv" << endl;
		MPI_Scatterv(MPI_IN_PLACE, scatterCounts, scatterDispls, MPI_BYTE, localScatBuf, scatterCounts[mpiRank], MPI_BYTE, sourceRank, MPI_COMM_WORLD);
	}

	for (int i=0; i<effectiveBlocks; i++) {
		void *data_p = malloc(FILE_SYSTEM_SINGLE_BLOCK_SIZE);
		memcpy(data_p, localScatBuf+i*FILE_SYSTEM_SINGLE_BLOCK_SIZE, FILE_SYSTEM_SINGLE_BLOCK_SIZE);
		localGathBuf[i].address = data_p;
	}

	PointerPacket *addresses = new PointerPacket[numberOfBlocksForRequest];
	MPI_Allgatherv(localGathBuf, gatherCounts[mpiRank], MPI_BYTE, addresses, gatherCounts, gatherDispls, MPI_BYTE, MPI_COMM_WORLD);

	Blocks *blocksManager = Blocks::getInstance();
	vector<DataBlock *> &dataBlockList = blocksManager->getDataBlockListOfInode(inode);
	int additionBlocks = numberOfBlocksForRequest - dataBlockList.size();
	if (additionBlocks > 0) {
		dataBlockManager->addDataBlocksTo(dataBlockList, additionBlocks, inode, sourceRank);
	}
	for (int i=0; i < numberOfBlocksForRequest; i++) {
		DataBlock *dataBlock = dataBlockList[i];
		dataBlock->setData(addresses[i].address);
	}

	delete[] scatterCounts;
	delete[] scatterDispls;
	free(localScatBuf);
	delete[] gatherCounts;
	delete[] gatherDispls;
	delete[] localGathBuf;

	if (mpiRank != sourceRank) {
		Nodes *INodeManager = Nodes::getInstance();
		INode *inode_p = INodeManager->getINodeByINodeNumber(inode);
		inode_p->m_fuseEntryParam.attr.st_size = fileSize;
		inode_p->m_fuseEntryParam.attr.st_blocks = dataBlockList.size();
	}

	//DEBUG
	int i=0;
	for (auto &dataBlock : dataBlockList) {
		cout << "Process " << mpiRank << " - address["<<i++<<"]="<<dataBlock->getData()<<endl;
	}
}

void* DistributedCode::DAGonFS_Read(int sourceRank, fuse_ino_t inode, size_t fileSize, size_t reqSize, off_t offset) {
	if (mpiRank == sourceRank) {
		IORequestPacket ioRequest;
		ioRequest.inode = inode;
		ioRequest.fileSize = fileSize;
		ioRequest.reqSize = reqSize;
		ioRequest.offset = offset;
		cout << "Process " << mpiRank << " - Notify all other process for reading operation" << endl;
		cout << "\tioRequest.inode="<<ioRequest.inode<<endl;
		cout << "\tioRequest.fileSize="<<ioRequest.fileSize<<endl;
		cout << "\tioRequest.reqSize="<<ioRequest.reqSize<<endl;
		cout << "\tioRequest.offset="<<ioRequest.offset<<endl;
		for (int i=0; i<mpiWorldSize; i++) {
			MPI_Request request;
			if (i != mpiRank) {
				MPI_Isend(&ioRequest,sizeof(IORequestPacket), MPI_BYTE, i, 0, MPI_COMM_WORLD, &request);
			}
		}
	}

	if (fileSize == 0)
		return nullptr;

	size_t numberOfBlocksForRequest;
	if (reqSize > fileSize)
		numberOfBlocksForRequest = fileSize / FILE_SYSTEM_SINGLE_BLOCK_SIZE + (fileSize % FILE_SYSTEM_SINGLE_BLOCK_SIZE > 0);
	else
		numberOfBlocksForRequest = reqSize / FILE_SYSTEM_SINGLE_BLOCK_SIZE + (reqSize % FILE_SYSTEM_SINGLE_BLOCK_SIZE > 0);

	void *readBuff = nullptr;
	if (mpiRank == sourceRank) {
		readBuff = malloc(numberOfBlocksForRequest * FILE_SYSTEM_SINGLE_BLOCK_SIZE);
		if (readBuff == nullptr) {
			abort();
		}
	}

	size_t blockPerProcess = numberOfBlocksForRequest / mpiWorldSize;
	size_t remainingBlocks = numberOfBlocksForRequest % mpiWorldSize;
	size_t effectiveBlocks = blockPerProcess + (mpiRank < remainingBlocks);

	//Scatter dei puntatori
	PointerPacket *addressesToScat = nullptr;
	if (mpiRank == sourceRank) {
		addressesToScat = new PointerPacket[numberOfBlocksForRequest];
		Blocks *blocksManager = Blocks::getInstance();
		vector<DataBlock *> &dataBlockList = blocksManager->getDataBlockListOfInode(inode);
		for (int i=0; i<numberOfBlocksForRequest; i++) {
			addressesToScat[i].address = dataBlockList[i]->getData();
		}
	}

	PointerPacket *localScatBuf = new PointerPacket[effectiveBlocks];
	int *scatterCounts = new int[mpiWorldSize];
	int *scatterDispls = new int[mpiWorldSize];
	int scatterOffset = 0;
	for (int i=0; i<mpiWorldSize; i++) {
		//Calculating elements
		scatterCounts[i] = ( blockPerProcess + (i < remainingBlocks) ) * sizeof(PointerPacket);

		//Calculating displacements
		scatterDispls[i] = scatterOffset;
		scatterOffset += scatterCounts[i];
	}

	//Gather dei dati
	void *localGathBuf = malloc(effectiveBlocks*FILE_SYSTEM_SINGLE_BLOCK_SIZE);
	int *gatherCounts = new int[mpiWorldSize];
	int *gatherDispls = new int[mpiWorldSize];
	int gatherOffset = 0;
	for (int i=0; i<mpiWorldSize; i++) {
		//Calculating elements
		gatherCounts[i] = ( blockPerProcess + (i < remainingBlocks) ) * FILE_SYSTEM_SINGLE_BLOCK_SIZE;

		//Calculating displacements
		gatherDispls[i] = gatherOffset;
		gatherOffset += gatherCounts[i];
	}

	if (mpiRank == sourceRank) {
		MPI_Scatterv(addressesToScat, scatterCounts, scatterDispls, MPI_BYTE, localScatBuf, scatterCounts[mpiRank], MPI_BYTE, sourceRank, MPI_COMM_WORLD);
	}
	else {
		MPI_Scatterv(MPI_IN_PLACE, scatterCounts, scatterDispls, MPI_BYTE, localScatBuf, scatterCounts[mpiRank], MPI_BYTE, sourceRank, MPI_COMM_WORLD);
	}

	for (int i=0; i<effectiveBlocks; i++) {
		memcpy(localGathBuf + i*FILE_SYSTEM_SINGLE_BLOCK_SIZE, localScatBuf[i].address, FILE_SYSTEM_SINGLE_BLOCK_SIZE);
	}

	MPI_Gatherv(localGathBuf, gatherCounts[mpiRank], MPI_BYTE, mpiRank == sourceRank ? readBuff : MPI_IN_PLACE, gatherCounts, gatherDispls, MPI_BYTE, sourceRank, MPI_COMM_WORLD);

	delete[] scatterCounts;
	delete[] scatterDispls;
	delete[] addressesToScat;
	delete[] gatherCounts;
	delete[] gatherDispls;
	free(localGathBuf);

	return readBuff;
}

void DistributedCode::createFile() {
	FileSystem::createFileFromThread = true;
	FileCreationRequest fileCreationRequest;

	MPI_Status status;
	MPI_Recv(&fileCreationRequest,sizeof(FileCreationRequest), MPI_BYTE, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);

	string absPath = fsPath + fileCreationRequest.name;
	//cout << "Process " << mpiRank << " - Creating file '" << absPath << "'" << endl;

	FILE *file = fopen(absPath.c_str(), "w");
	fclose(file);
}

void DistributedCode::deleteFile() {
	FileSystem::deleteFileFromThread = true;
	FileDeletionRequest fileDeleteRequest;

	MPI_Status status;
	MPI_Recv(&fileDeleteRequest,sizeof(FileDeletionRequest), MPI_BYTE, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);

	string absPath = fsPath + fileDeleteRequest.name;
	//cout << "Process " << mpiRank << " - Deleting file '" << absPath << "'" << endl;

	unlink(absPath.c_str());
}

void DistributedCode::createDir() {
	FileSystem::createDirFromThread = true;
	DirectoryCreationRequest dirCreateRequest;

	MPI_Status status;
	MPI_Recv(dirCreateRequest.absolutePath,sizeof(DirectoryCreationRequest), MPI_BYTE, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);

	string absPath = fsPath + dirCreateRequest.absolutePath;
	//cout << "Process " << mpiRank << " - Creating directory '" << absPath << "'" << endl;

	mkdir(absPath.c_str(), 0777);
}

void DistributedCode::deleteDir() {
	FileSystem::deleteDirFromThread = true;
	DirectoryDeletionRequest dirDeleteRequest;

	MPI_Status status;
	MPI_Recv(dirDeleteRequest.absolutePath,sizeof(DirectoryDeletionRequest), MPI_BYTE, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);

	string absPath = fsPath + dirDeleteRequest.absolutePath;
	//cout << "Process " << mpiRank << " - Deleting directory '" << absPath << "'" << endl;

	rmdir(absPath.c_str());
}
