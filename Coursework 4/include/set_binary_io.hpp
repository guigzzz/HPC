#ifndef set_binary_io_hpp
#define set_binary_io_hpp

#if !(defined(_WIN32) || defined(_WIN64))
#include <unistd.h>
void set_binary_stdin()
{}

void set_binary_stdout()
{}

void set_binary_io()
{}
#else
// http://stackoverflow.com/questions/341817/is-there-a-replacement-for-unistd-h-for-windows-visual-c
// http://stackoverflow.com/questions/13198627/using-file-descriptors-in-visual-studio-2010-and-windows

#include <io.h>
#include <fcntl.h>

#define read _read
#define write _write
#define STDIN_FILENO 0
#define STDOUT_FILENO 1

void set_binary_stdin()
{
	_setmode(_fileno(stdin), _O_BINARY);
}

void set_binary_stdout()
{
	_setmode(_fileno(stdout), _O_BINARY);
}

void set_binary_io()
{
	set_binary_stdin();
	set_binary_stdout();
}
#endif

#endif
