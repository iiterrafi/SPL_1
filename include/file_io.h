#ifndef FILE_IO_H
#define FILE_IO_H

#include <stddef.h>

/*

* Stores file contents in memory.
*
* data: Pointer to the file's bytes.
* size: Number of bytes stored.
*
* For an empty file, data is NULL and size is 0.
  */
  typedef struct
  {
  unsigned char *data;
  size_t size;
  } FileBuffer;

/*

* Reads an entire file into memory.
*
* filename: Path of the file to read.
* buffer: Pointer to an initialized FileBuffer.
*
* Returns:
* 0  on success
* -1  on failure
*
* Initialize the buffer as {NULL, 0} before calling.
* Release its allocated memory using free_file_buffer().
  */
  int read_file(const char *filename, FileBuffer *buffer);

/*

* Writes data to a file.
*
* filename: Destination file path.
* data: Bytes to write.
* size: Number of bytes to write.
*
* data may be NULL only when size is 0.
*
* Returns:
* 0  on success
* -1  on failure
  */
  int write_file(const char *filename, const unsigned char *data, size_t size);

/*

* Releases the memory owned by a FileBuffer
* and resets its fields.
  */
  void free_file_buffer(FileBuffer *buffer);

#endif
