/*
 Inspired by bin2tap from http://metalbrain.speccy.org/.

 This program takes a binary or obj ZX Spectrum machine code program and
 creates a tap file from it.
 
 It adds a ZX Basic loader as well. I.e. the user just needs to type
 LOAD ""
 
 This will load the ZX Basic loader, which contains a program that consists basically of
 LOAD "" CODE 16384
 LOAD "" CODE 
 RANDOMIZE USR EXEC_ADDR
 
 The tap file is configurable:
 - name of the program; shown after LOAD ""
 - EXEC_ADDR: The start address of the machine code executable
 - screen data: Screen$ to load
 - binary code: the machine code data
 */

//#include <stdio.h>
#include <iostream>
#include <string.h>
//#include <linux/limits.h>
#include <vector>
#include <stdarg.h>
#include <iterator>
#include <fstream>


// The version string.
#define VERSION "1.1.0"


// ZX Basic commands coding.
#define CLEAR		"\xFD"
#define CLS			"\xFB"
#define LOAD		"\xEF"
#define CODE		"\xAF"
#define RANDOMIZE	"\xF9"
#define REM			"\xEA"
#define USR			"\xC0"
#define BORDER		"\xE7"
#define POKE		"\xF4"
#define PAPER		"\xDA"
#define INK			"\xD9"
#define VAL			"\xB0"
#define BLACK		VAL "\"0\""
#define WHITE		VAL "\"7\""



// TAP types
#define TAP_HDR_BASIC	0
#define TAP_HDR_CODE	3


using namespace std;


// Define byte.
typedef char byte;


// Prototypes.
void print_help();
void tap_create_zx_basic_loader( FILE *fp, char* name, int start_address, int exec_address, char* screen_file_name );
void tap_create_code( FILE *fp, char* file_name, int start_address );
vector<byte> create_zx_basic_line(int line_number, const char* line_fmt, ... );
void tap_write_data_block_with_checksum( FILE* fp, byte flags, vector<byte>& data );
void tap_write_prg_header( FILE* fp, char* fname, vector<byte>& data );
void tap_write_code_header( FILE* fp, const char* fname, vector<byte>& data, int start_address );


///////////////////////////////////////////////////////////////////
/// Main.
///////////////////////////////////////////////////////////////////
int main( int argc, char* argv[] ) {
	int i = 1;
    char* screen_file_name = NULL;
    char* code_file_name = NULL;
	char* prg_name = NULL;
	int start_address = -1;
	int exec_address = -1;
	char tap_file_name[PATH_MAX];
	tap_file_name[0] = 0;

    while( i < argc ) {
        char* s = argv[i];
        if( ! strncmp( "-", s, 1 ) ) {
            // some option
            if( ! strcmp( "-h", s ) ) {
                // Help
                print_help();
                exit( EXIT_SUCCESS );
             }
            else if( ! strcmp( "-code", s ) ) {
                // machine code file
                i++;
                code_file_name = argv[i];
             }
            else if( ! strcmp( "-screen", s ) ) {
                // screen data file
                i++;
                screen_file_name = argv[i];
            }
            else if( ! strcmp( "-start", s ) ) {
                // Start address of machine code
                i++;
                start_address = atoi( argv[i] );
             }
            else if( ! strcmp( "-exec", s ) ) {
                // Execution address of machine code
                i++;
                exec_address = atoi( argv[i] );
             }
            else if( ! strcmp( "-o", s ) ) {
                // Output filename
                i++;
				strcpy(tap_file_name, argv[i] );
             }
            else {
                // unknown option
                cout << "Error: Unknown option '" << s << "'" << endl << endl;
                print_help();
                exit( EXIT_FAILURE );
            }
        }
        else {
            // No option, so it is the program name
            if( prg_name ) {
				// already a name given
				cout << "Error: two program names given '" << s << "'" << endl << endl;
				print_help();
				exit( EXIT_FAILURE );
			}
			// Use program name
			prg_name = s;
         }
        // Next
        i++;
    }

    // Check for program name
    if( ! prg_name ) {
        // No program name
        cout << "Error: No program name given." << endl << endl;
        print_help();
        exit( EXIT_FAILURE );
    }

    // Check for filename
    if( ! code_file_name ) {
        // No filename
        cout << "Error: Expected a binary filename." << endl << endl;
        print_help();
        exit( EXIT_FAILURE );
    }

    // Check for a start address
    if( start_address < 0 ) {
        // No start address
        cout << "Error: No start address given." << endl << endl;
        print_help();
        exit( EXIT_FAILURE );
    }

    // Check for a execution address
    if( exec_address < 0 ) {
        // No execution address
        cout << "Error: No execution address given." << endl << endl;
        print_help();
        exit( EXIT_FAILURE );
    }

	// Check if filename was given.
	if(tap_file_name[0] == 0) 
		sprintf( tap_file_name, "%s.tap", prg_name );
	// Open file
    FILE* fp_out = fopen( tap_file_name, "w" );
    if (fp_out == NULL) {
        // Error
        cout << "Error: Couldn't open file '" << code_file_name << "' for writing." << endl;
        exit(EXIT_FAILURE);
    }

	// Process the data.
	// ZX Basic loader:
	tap_create_zx_basic_loader( fp_out, prg_name, start_address, exec_address, screen_file_name );
	// Screen data:
	if( screen_file_name ) {
		tap_create_code( fp_out, screen_file_name, 16384 );
	}
	// The machine code data:
	tap_create_code( fp_out, code_file_name, start_address );
	
    // close
    fclose( fp_out );

    // exit
    return EXIT_SUCCESS; 
}


/// Prints the usage.
void print_help() {  
    cout << "code2tap (v" << VERSION << ")" << endl;
    cout << "Usage:" << endl;
    cout << " code2tap prg_name -code code_file_name -start addr1 -exec addr2 [-screen screen_file_name] [-o tap_file_name]" << endl;
    cout << " prg_name: The name of the program." << endl;
    cout << "     I.e. the name presented while loading." << endl;
    cout << " -code code_file_name: The file containing the machine code binary." << endl;
    cout << " -start addr1: The load code start address." << endl;    
    cout << " -exec addr2: The machine code execution start address." << endl;
    cout << " -screen screen_file_name: The file name of the screen data." << endl;
    cout << " -o tap_file_name: The filename for the tap file." << endl;
	cout << "     If omitted 'prg_name'.tap is used." << endl;
}


/// Creates a ZX Basic loader tap binary code.
/// @param fp The file pointer to write to.
/// @param name The name of the program.
/// @param start_address The LOAD""CODE start address.
/// @param exec_address The USR execution address.
/// @param screen_file_name The file name for the screen data.
void tap_create_zx_basic_loader( FILE *fp, char* name, int start_address, int exec_address, char* screen_file_name ) {
	vector<byte> basic_loader;
	vector<byte> basic_line;
	
	int line_number = 10;
	
	// CLEAR start_address-1
	basic_line = create_zx_basic_line( line_number, CLEAR VAL "\"%d\"", start_address-1 );
	basic_loader.insert(basic_loader.end(), basic_line.begin(), basic_line.end());
	line_number += 10;
	
	// BORDER black, INK white, PAPER black, CLS
	basic_line = create_zx_basic_line( line_number, BORDER BLACK ":" PAPER BLACK ":" INK WHITE ":" CLS );
	basic_loader.insert(basic_loader.end(), basic_line.begin(), basic_line.end());
	line_number += 10;
	
#if 01
	// POKE 23739,111: Redirect output stream address to 0x096f which contains a simple "ret" (111=6F),
	// so that "CODE: name" or whatever will not be written to the screen.
	basic_line = create_zx_basic_line( line_number, POKE VAL "\"23739\"," VAL "\"111\"" );
	basic_loader.insert(basic_loader.end(), basic_line.begin(), basic_line.end());
	line_number += 10;
#endif
	
	// LOAD "" CODE 16384
	if( screen_file_name ) {
		basic_line = create_zx_basic_line( line_number, LOAD "\"\"" CODE  );
		basic_loader.insert(basic_loader.end(), basic_line.begin(), basic_line.end());
		line_number += 10;
	}
	
	// LOAD "" CODE
	basic_line = create_zx_basic_line( line_number, LOAD "\"\"" CODE );
	basic_loader.insert(basic_loader.end(), basic_line.begin(), basic_line.end());
	line_number += 10;
	
	// POKE 23739,244: So that text output will be visible again
	basic_line = create_zx_basic_line( line_number, POKE VAL "\"23739\"," VAL "\"244\"" );
	basic_loader.insert(basic_loader.end(), basic_line.begin(), basic_line.end());
	line_number += 10;

	// RANDOMIZE USR EXEC_ADDR
	basic_line = create_zx_basic_line( line_number, RANDOMIZE USR VAL "\"%d\"", exec_address  );
	basic_loader.insert(basic_loader.end(), basic_line.begin(), basic_line.end());
	//line_number += 10;
	
	//cout << "basic_size=" << basic_loader.size() << endl;
	
	// Write to tap
	tap_write_prg_header(fp, name, basic_loader);
	tap_write_data_block_with_checksum( fp, (byte)-1, basic_loader );
}


/// Creates a sinlge line of basic in memory.
/// @param line_number The Basic listing line number
/// @param line_fmt The Basic commands etc. as string. Acts
/// as a format string if argumens should follow.
/// @param ... arguments
vector<byte> create_zx_basic_line(int line_number, const char* line_fmt, ... ) {
	// ZX Basic line, see http://www.worldofspectrum.org/ZXBasicManual/zxmanchap24.html
	// 2 bytes:	line number (big endian)
	// 2 bytes: length of text + enter (little endian)
	// x bytes: text (the basic command(s))
	// 1 byte: enter (0x0D = "\r")
	// Basic commands might be separated by ":" (0x3A like ASCII)
	
	// create line string from format.
	char line_string[1024];	// should be enough
    va_list argptr;
    va_start(argptr, line_fmt);
    vsprintf(line_string, line_fmt, argptr);
    va_end(argptr);
    
    // Encode line  	
	vector<byte> line;
	// Basic line number
	line.push_back(line_number >> 8);
	line.push_back(line_number & 0xFF);
	
	// add 2 (for now) empty entries
	line.push_back(0);
	line.push_back(0);
	
	// add the basic commands
	int slen = strlen(line_string);
	for( int i=0; i<slen; i++ ) {
		byte b = line_string[i];
		line.push_back(b);
	}
	
	// add 'enter'
	line.push_back('\r');

	// line length
	int line_length = slen+1;
	line[2] = line_length & 0xFF;
	line[3] = line_length >> 8;

	// return
	return line;
}



/// Creates a tap binary code.
/// @param fp The file pointer to write to.
/// @param fname the filename to put into the header.
/// @param start_address of the code area.
void tap_create_code( FILE *fp, char* file_name, int start_address ) {
	if( ! file_name )
		return;

	// open input data
	ifstream in_file(file_name, ios::binary);
	if( in_file.fail() ) {
       // Error
        cout << "Error: Couldn't open file '" << file_name << "'." << endl;
        exit(EXIT_FAILURE);
    }
		
	// load data into vector
	vector<byte> data((istreambuf_iterator<byte>(in_file)), istreambuf_iterator<byte>());
	
	// Write header
	tap_write_code_header(fp, "code", data, start_address);

	// tap data
	tap_write_data_block_with_checksum(fp, (byte)-1, data);
}


/// Writes a tap program header.
/// @param fp The file pointer to write to.
/// @param fname the filename to put into the header.
/// @param data the data to write. Used only for the size of the data.
void tap_write_prg_header( FILE* fp, char* fname, vector<byte>& data ) {
	vector<byte> hdr;
	// See http://www.zx-modules.de/fileformats/tapformat.html
	// 2 bytes: length
	// 1 byte: always 0
	// Here the block starts:
	// 1 byte: type, 0=basic prgm
	// 10 char:	file name
	// 2 byte: length of following data (basic+variables)
	// 2 byte: autostart line number or start address
	// 2 byte: basic program length
	// 1 byte: checksum

	// Type
	hdr.push_back(TAP_HDR_BASIC);
	// filename
	size_t fname_len = strlen(fname);
	for( size_t i=0; i<10; i++ ) {
		if( i < fname_len )
			hdr.push_back(fname[i]);
		else
			hdr.push_back(' ');
	}
	// length of basic+variables
	int len_basic = data.size();
	hdr.push_back(len_basic & 0xFF);
	hdr.push_back(len_basic >> 8);
	// autostart line number. Use the first line of the program.
#if 01
	hdr.push_back(data[1]);	// exchange high and low byte
	hdr.push_back(data[0]);
#else
	// No autostart line
	hdr.push_back(0);	
	hdr.push_back(0x80);
#endif
	// basic program length
	hdr.push_back(len_basic & 0xFF);
	hdr.push_back(len_basic >> 8);
	
	// Write data block with check sum
	tap_write_data_block_with_checksum(fp, 0, hdr);
}

/// Writes a tap code header.
/// @param fp The file pointer to write to.
/// @param fname the filename to put into the header.
/// @param data the data to write. Used only for the size of the data.
/// @param start_address of the code area.
void tap_write_code_header( FILE* fp, const char* fname, vector<byte>& data, int start_address ) {
	vector<byte> hdr;
	// See http://www.zx-modules.de/fileformats/tapformat.html
	// 2 bytes: length
	// 1 byte: always 0
	// Here the block starts:
	// 1 byte: type, 3=code
	// 10 char:	file name
	// 2 byte: code length
	// 2 byte: start address
	// 2 byte: 32768
	// 1 byte: checksum

	// Type
	hdr.push_back(TAP_HDR_CODE);
	// filename
	size_t fname_len = strlen(fname);
	for( size_t i=0; i<10; i++ ) {
		if( i < fname_len )
			hdr.push_back(fname[i]);
		else
			hdr.push_back(' ');
	}
	// length of code
	int len_data = data.size();
	hdr.push_back(len_data & 0xFF);
	hdr.push_back(len_data >> 8);
	// start address
	hdr.push_back(start_address & 0xFF);
	hdr.push_back(start_address >> 8);
	// 32768
	hdr.push_back(0);
	hdr.push_back(0x80);
	
	// Write data block with check sum
	tap_write_data_block_with_checksum(fp, 0, hdr);
}


/// Writes a tap data block with checksum.
/// @param fp The file pointer to write to.
/// @param flags The flags to use for the block.
/// @param data the data to write.
void tap_write_data_block_with_checksum( FILE* fp, byte flags, vector<byte>& data ) {
	// Write length of block
	int16_t len = data.size() + 2;	// size of complete block
	fwrite(&len, sizeof(len), 1, fp );
	// Flags = 0
	fputc( flags, fp );
	byte checksum = flags;
	// Write data
	for( auto b : data ) {
		// Write
		fputc( b, fp );
		// Checksum
		checksum ^= b;
	}
	// Checksum
	fputc( checksum, fp );
}





