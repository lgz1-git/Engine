#pragma once
#include "h_type.h"

struct fs_file_handle {
	void* handle;
	bool  is_valid;
};

enum fs_file_mode {
	FILE_MODE_READ = 0x1,
	FILE_MODE_WRITE = 0x2 ,
};

bool fs_exists(const char* path);

bool fs_open(const char* path, fs_file_mode mode, bool binary, fs_file_handle* handle);

void fs_close(fs_file_handle* handle);

bool fs_read_line(fs_file_handle* handle,char** line);
bool fs_write_line(fs_file_handle* handle,const char* text);

bool fs_read(fs_file_handle* handle, size_t size, void* out_data, size_t* bytes_read);
bool fs_read_all_bytes(fs_file_handle* handle, u8** bytes , size_t* bytes_read);

bool fs_write(fs_file_handle* handle, size_t data_size, const void* data, size_t* bytes_written);