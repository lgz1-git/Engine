#include "h_filesystem.h"
#include "h_clogger.h"


bool fs_exists(const char* path)
{
	struct stat buffer;
	return stat(path, &buffer);
}

bool fs_open(const char* path, fs_file_mode mode, bool binary, fs_file_handle* handle)
{
	handle->is_valid = false;
	handle->handle = 0;
	const char* mode_str;

	if ((mode & FILE_MODE_READ) != 0 && (mode & FILE_MODE_WRITE) != 0) {
		mode_str = binary ? "w+b" : "w+";
	}
	else if ((mode & FILE_MODE_READ) != 0 && (mode & FILE_MODE_WRITE) == 0) {
		mode_str = binary ? "rb" : "r";
	}
	else if ((mode & FILE_MODE_READ) == 0 && (mode & FILE_MODE_WRITE) != 0) {
		mode_str = binary ? "wb" : "w";
	}
	else {
		LERR("file mode is invalid to file: " << path); 
		return false;
	}

	FILE* file = 0;
	fopen_s(&file,path, mode_str);//TODO:fopen_s ?
	if (!file) {
		LERR("open file failed: " << path);
		return false;
	}

	handle->handle = file;
	handle->is_valid = true;

	return true;
}

void fs_close(fs_file_handle* handle)
{
	if (handle->handle)
	{
		fclose((FILE*)handle->handle);
		handle->handle = 0;
		handle->is_valid = false;
	}
}

bool fs_read_line(fs_file_handle* handle, char** line)
{
	if (handle->handle) {
		char* buf = (char*)alloca(32000 * sizeof(char));
		if (fgets(buf, 32000, (FILE*)handle->handle) != 0) {
			size_t length = strlen(buf);
			*line = (char*)malloc(length * sizeof(char) + 1);
			strcpy_s(*line,length, buf);//TODO:_s ?
			return true;
		}
	}
	return false;
}
bool fs_write_line(fs_file_handle* handle, const char* text)
{
	if (handle->handle) {
		i32 result = fputs(text, (FILE*)handle->handle);
		if (result != EOF) {
			result = fputc('\n', (FILE*)handle->handle);
		}

		fflush((FILE*)handle->handle);
		return result != EOF;
	}
	return false;
}

bool fs_read(fs_file_handle* handle, size_t size, void* out_data, size_t* bytes_read)
{
	if (handle->handle && out_data) {
		*bytes_read = fread(out_data, 1, size, (FILE*)handle->handle);
		if (*bytes_read != size) {
			return false;
		}
		return true;
	}
	return false;
}
bool fs_read_all_bytes(fs_file_handle* handle, u8** bytes, size_t* bytes_read)
{
	if (handle->handle) {
		fseek((FILE*)handle->handle, 0, SEEK_END);
		size_t size = ftell((FILE*)handle->handle);
		rewind((FILE*)handle->handle);

		*bytes = (u8*)malloc(sizeof(u8) * size);
		*bytes_read = fread(*bytes, 1, size, (FILE*)handle->handle);
		if (*bytes_read != size) {
			return false;
		}
		return true;
	}
	return false;
}

bool fs_write(fs_file_handle* handle, size_t data_size, const void* data, size_t* bytes_written)
{
	if (handle->handle) {
		*bytes_written = fwrite(data, 1, data_size, (FILE*)handle->handle);
		if (*bytes_written != data_size) {
			return false;
		}
		fflush((FILE*)handle->handle);
		return true;
	}
	return false;
}