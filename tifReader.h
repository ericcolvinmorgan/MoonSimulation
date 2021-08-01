#pragma once

#include <string>
#include <vector>
#include <fstream>

#define TIF_READER_BUFFER_LENGTH (512)

class tifReader
{
public:
	tifReader(std::string fileLocation);
    char* get_values();
    unsigned int getValueCount() { return (_width * _length); }
    //std::vector<std::string> read_strip();

private:
    std::istream* _stream;
    std::streamsize _stream_length = 0;
    unsigned short _buffer_position = 0;
    std::streamsize _buffer_length = 0;
    char _buffer[TIF_READER_BUFFER_LENGTH] = { 0 };
    bool _littleEndian;
    bool _valid = false;
    unsigned int _width = 0;
    unsigned int _length = 0;
    unsigned int _expectedStripsPerImage = 0;
    unsigned short _bitesPerSample = 0;
    std::vector<unsigned int> _stripOffsets;
    std::vector<unsigned int> _expectedBytesPerStrip;
    void _processHeader();
    void _processDirectoryEntry(short tag, short type);
};

