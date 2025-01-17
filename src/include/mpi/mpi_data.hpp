//
// Created by frank on 12/30/24.
//

#ifndef MPI_DATA_HPP
#define MPI_DATA_HPP

#include "../utils/fuse_headers.hpp"

typedef enum {WRITE, READ, CREATE_FILE, DELETE_FILE, CREATE_DIR, DELETE_DIR, TERMINATE} RequestType;

typedef struct RequstPacket {
	RequestType type;
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
