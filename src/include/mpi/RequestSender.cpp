//
// Created by frank on 1/15/25.
//

#include "RequestSender.hpp"

#include <cstring>
#include <iostream>
#include <mpi.h>

#include "DistributedCode.hpp"
#include "mpi_data.hpp"

using namespace std;

void RequestSender::sendWriteRequest(int sourceRank, int mpiWorldSize) {
	RequestPacket loopRequest;
	loopRequest.type = WRITE;
	for (int i=0; i<mpiWorldSize; i++) {
		MPI_Request loop;
		if (i != sourceRank) {
			MPI_Isend(&loopRequest, sizeof(RequestPacket), MPI_BYTE, i, 0, MPI_COMM_WORLD, &loop);
		}
	}
}

void RequestSender::sendReadRequest(int sourceRank, int mpiWorldSize) {
	RequestPacket loopRequest;
	loopRequest.type = READ;
	for (int i=0; i<mpiWorldSize; i++) {
		MPI_Request loop;
		if (i != sourceRank) {
			MPI_Isend(&loopRequest, sizeof(RequestPacket), MPI_BYTE, i, 0, MPI_COMM_WORLD, &loop);
		}
	}
}

void RequestSender::sendCreateFileRequest(string name, int sourceRank, int mpiWorldSize) {
	RequestPacket loopRequest;
	loopRequest.type = CREATE_FILE;
	FileCreationRequest fileCreateRequest;
	memcpy(fileCreateRequest.name, name.c_str(), name.size());
	for (int i=0 ; i<mpiWorldSize ; i++) {
		MPI_Request loop,file;
		MPI_Isend(&loopRequest, sizeof(RequestPacket), MPI_BYTE, i, 0, MPI_COMM_WORLD, &loop);
		if (i != sourceRank) {
			MPI_Isend(&fileCreateRequest, sizeof(FileCreationRequest), MPI_BYTE, i, 0, MPI_COMM_WORLD, &file);
		}
	}
}

void RequestSender::sendDeleteFileRequest(string name, int sourceRank, int mpiWorldSize) {
	RequestPacket loopRequest;
	loopRequest.type = DELETE_FILE;
	FileDeletionRequest fileDeleteRequest;
	memcpy(fileDeleteRequest.name, name.c_str(), name.size());
	for (int i=0 ; i<mpiWorldSize ; i++) {
		MPI_Request loop,file;
		MPI_Isend(&loopRequest, sizeof(RequestPacket), MPI_BYTE, i, 0, MPI_COMM_WORLD, &loop);
		if (i != sourceRank) {
			MPI_Isend(&fileDeleteRequest, sizeof(FileDeletionRequest), MPI_BYTE, i, 0, MPI_COMM_WORLD, &file);
		}
	}
}

void RequestSender::sendCreateDirectoryRequest(string dirAbsPath, int sourceRank, int mpiWorldSize) {
	RequestPacket loopRequest;
	loopRequest.type = CREATE_DIR;
	DirectoryCreationRequest dirCreateRequest;
	memcpy(dirCreateRequest.absolutePath, dirAbsPath.c_str(), dirAbsPath.size());
	for (int i=0; i< mpiWorldSize; i++) {
		MPI_Request loop,dir;
		MPI_Isend(&loopRequest, sizeof(RequestPacket), MPI_BYTE, i, 0, MPI_COMM_WORLD, &loop);
		if (i != sourceRank)
			MPI_Isend(&dirCreateRequest, sizeof(DirectoryCreationRequest), MPI_BYTE, i, 0, MPI_COMM_WORLD, &dir);
	}
}

void RequestSender::sendDeleteDirectoryRequest(string dirAbsPath, int sourceRank, int mpiWorldSize) {
	RequestPacket loopRequest;
	loopRequest.type = DELETE_DIR;
	DirectoryDeletionRequest dirDeleteRequest;
	memcpy(dirDeleteRequest.absolutePath, dirAbsPath.c_str(), dirAbsPath.size());
	for (int i=0; i< mpiWorldSize; i++) {
		MPI_Request loop,dir;
		MPI_Isend(&loopRequest, sizeof(RequestPacket), MPI_BYTE, i, 0, MPI_COMM_WORLD, &loop);
		if (i != sourceRank)
			MPI_Isend(&dirDeleteRequest, sizeof(DirectoryDeletionRequest), MPI_BYTE, i, 0, MPI_COMM_WORLD, &dir);
	}
}

void RequestSender::sendTerminationRequest(int sourceRank, int mpiWorldSize) {
	cout << "Process " << sourceRank << " - Sending termination request" << endl;
	RequestPacket termationRequest;
	termationRequest.type = TERMINATE;
	for (int i=0; i<mpiWorldSize; i++) {
		MPI_Request request;
		if (i==sourceRank){
                  MPI_Isend(&termationRequest, sizeof(RequestPacket), MPI_BYTE, i, 0, MPI_COMM_SELF, &request);
		}
		else{
		  MPI_Isend(&termationRequest, sizeof(RequestPacket), MPI_BYTE, i, 0, MPI_COMM_WORLD, &request);
		}
	}

}
