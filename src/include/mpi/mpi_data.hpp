//
// Created by frank on 12/30/24.
//

#ifndef MPI_DATA_HPP
#define MPI_DATA_HPP

#include "../utils/fuse_headers.hpp"

#define TERMINATE_REQ 0
#define WRITE_REQ 1
#define READ_REQ 2
#define CREATE_FILE_REQ 3
#define DELETE_FILE_REQ 4
#define CREATE_DIR_REQ 5
#define DELETE_DIR_REQ 6

//typedef enum {WRITE, READ, CREATE_FILE, DELETE_FILE, CREATE_DIR, DELETE_DIR, TERMINATE} RequestType;

typedef struct RequstPacket {
	//RequestType type;
	char type;
} RequestPacket;


typedef struct IORequestPacket {
	fuse_ino_t inode;
	void *data;
	size_t fileSize;
	size_t reqSize;
	off_t offset;
} IORequestPacket;


typedef struct PointerPacket {
	void *address;
} PointerPacket;

typedef struct FileCreationRequest {
	char name[256] = {0};
} FileCreationRequest;

typedef struct FileDeletionRequest {
	char name[256] = {0};
} FileDeletionRequest;

typedef struct DirectoryCreationRequest {
	char absolutePath[256] = {0};
} DirectoryCreationRequest;

typedef struct DirectoryDeletionRequest {
	char absolutePath[256] = {0};
} DirectoryDeletionRequest;

#endif //MPI_DATA_HPP
