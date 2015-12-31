// Copyright 2009-2010 Mars Saxman.
//
// Radian is free software: you can redistribute it and/or modify it under the terms of the GNU General Public 
// License as published by the Free Software Foundation, either version 3 of the License, or (at your option)
// any later version.
//
// Radian is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with Radian.  If not, see
// <http://www.gnu.org/licenses/>.

#include <vector>
#include <string>
using namespace std;
#include <Windows.h>
#include "radian.h"

static wstring UTF8ToUTF16( const char *utf8Str )
{
	// Calculate the size of the resulting string we'll need
	int size = ::MultiByteToWideChar( CP_UTF8, 0, utf8Str, -1, NULL, 0 );

	// Create a string and reserve enough space for the UTF-16 version of it
	wstring ret;
	wchar_t *converted_arg = (wchar_t *)ret.get_allocator().allocate( size );

	// Now perform the conversion
	::MultiByteToWideChar( CP_UTF8, 0, utf8Str, -1, converted_arg, size );

	// We want to copy in all of the wide chars
	ret.assign( converted_arg, size );

	return ret;
}

static string UTF16ToUTF8( const wchar_t *utf16Str )
{
	// Calculate the size of the resulting string we'll need
	int size = ::WideCharToMultiByte( CP_UTF8, 0, utf16Str, -1, NULL, 0, NULL, NULL );

	// Create a string and reserve enough space for the UTF-8 version of it
	string ret;
	char *converted_arg = (char *)ret.get_allocator().allocate( size );

	// Now perform the conversion
	::WideCharToMultiByte( CP_UTF8, 0, utf16Str, -1, converted_arg, size, NULL, NULL );

	ret.assign( converted_arg, size );

	return ret;
}

class Win32Radian : public Radian
{
	protected:
		string LoadFile( string filepath );
		string LibDir();
		string RuntimeDir();
		int RunExecutable( string outfile, const vector<string> &args );
		string PathSeparator() { return "\\"; }
		string CCompilerCommand() { return "C:\\MinGW\\bin\\gcc.exe"; }
};

int wmain( int argc, wchar_t *argv[] ) 
{
	// All of our args are in UCS-2, so we want to convert them to being
	// UTF-8 when passing them to the compiler
	vector<string> args;
	for (int i = 0; i < argc; i++) {
		args.push_back( UTF16ToUTF8( argv[ i ] ) );
	}
	
	Win32Radian compiler;
	return compiler.Run( args );
}

// Win32Radian::LoadFile
//
// Given a file path, load the contents up as a string. I'd like to change this API someday so we can use
// mmap instead of the file APIs.
//
string Win32Radian::LoadFile( string filepath )
{
	string source;
	wstring filepath_utf16 = UTF8ToUTF16( filepath.c_str() );

	HANDLE hFile = ::CreateFileW( filepath_utf16.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
	if (hFile != INVALID_HANDLE_VALUE) {
		DWORD length = ::GetFileSize( hFile, NULL );
		char *buffer = new char[ length ];
		DWORD amountRead = 0;
		::ReadFile( hFile, buffer, length, &amountRead, NULL );
		if (amountRead == length) {
			source.assign( buffer, length );
		} else {
			Abort( "Could not read entire contents of file " + filepath, ::GetLastError() );
		}
		delete [] buffer;
		::CloseHandle( hFile );
	} else {
		Abort( "could not open file " + filepath, ::GetLastError() );
	}
	return source;
}

// Win32Radian::RuntimeDir
//
// Where should we look for the runtime support files?
// 
string Win32Radian::RuntimeDir()
{
	return "runtime";
}

// Win32Radian::LibDir
//
// Where should we look for the standard object library files?
// 
string Win32Radian::LibDir()
{
	return "library";
}

int Win32Radian::RunExecutable( string outfile, const vector<string> &args )
{
	STARTUPINFOW si = { 0 };
	si.cb = sizeof( si );
	PROCESS_INFORMATION pi = { 0 };

	wstring outfile_utf16 = UTF8ToUTF16( outfile.c_str() );

	wchar_t buffer[ 1024 ] = { 0 };
	int j = swprintf_s( buffer, _countof( buffer ), L"%s", outfile_utf16.c_str() );
	for (vector< string >::const_iterator iter = args.begin(); iter != args.end(); ++iter) {
		wstring arg_utf16 = UTF8ToUTF16( iter->c_str() );
		j += swprintf_s( buffer + j, _countof( buffer ), L" %s", arg_utf16.c_str() );
	}

	if (!::CreateProcessW( NULL, buffer, NULL, NULL, false, 0, NULL, NULL, &si, &pi )) {
		Abort( "Failed to launch the process at " + outfile, ::GetLastError() );
		return 0;
	}

	::WaitForSingleObject( pi.hProcess, INFINITE );

	::CloseHandle( pi.hProcess );
	::CloseHandle( pi.hThread );

	return 1;
}
