#include "tifReader.h"

tifReader::tifReader(std::string fileLocation)
{
	std::ifstream* ifstream = new std::ifstream();
	ifstream->open(fileLocation, std::ios_base::binary);
	_stream = ifstream;
	_stream->seekg(0, std::ios::end);
	_stream_length = _stream->tellg();
	_stream->seekg(0, std::ios::beg);
	_processHeader();
}

char* tifReader::get_values()
{
	char* values = nullptr;
	unsigned int value_pos = 0;
	values = new char[(_bitesPerSample / 8) * _length * _width];
	if (values == nullptr)
	{
		return nullptr;
	}

	for (int strip = 0; strip < _stripOffsets.size(); strip++)
	{
		_stream->seekg(_stripOffsets[strip], std::ios::beg);
		for (int pos = 0; pos < _expectedBytesPerStrip[strip]; pos++)
		{
			_stream->read(&values[value_pos++], 1);
		}
	}
	return values;
}

void tifReader::_processHeader()
{
	//Detect Byte Order Mark
	if (_stream_length >= 8)
	{
		char begChars[4];
		_stream->read(begChars, 4);
		if (begChars[0] == '\x49' && begChars[1] == '\x49' && begChars[2] == '\x2A' && begChars[3] == '\00')
		{
			_littleEndian = true;
			_valid = true;
		}
		else if (begChars[0] == '\x4D' && begChars[1] == '\x4D' && begChars[2] == '\x2A' && begChars[3] == '\00')
		{
			_littleEndian = false;
			_valid = true;
		}
		else
		{
			_valid = false;
		}

		if (_valid)
		{
			int position = 0;
			int size = sizeof(position);
			_stream->read((char*)&position, sizeof(position));
			_stream->seekg(position, std::ios::beg);

			short num_entries;
			_stream->read((char*)&num_entries, sizeof(num_entries));
		
			for (short entry = 0; entry < num_entries; entry++)
			{
				auto current_start = _stream->tellg();
				short tag;
				_stream->read((char*)&tag, sizeof(tag));
				short type;
				_stream->read((char*)&type, sizeof(type));

				_processDirectoryEntry(tag, type);
				_stream->seekg((current_start += 12), std::ios::beg);
			}

			size = size++;
		}
	}
}

void tifReader::_processDirectoryEntry(short tag, short type)
{
	int count;
	_stream->read((char*)&count, sizeof(count));
	
	unsigned int value;
	_stream->read((char*)&value, sizeof(value));

	switch (tag)
	{
	case 256: //Image Width - number of columns in the image (the number of pixels per row).
		_width = value;
		break;

	case 257: //Image Length - number of rows of pixels in the image
		_length = value;
		break;

	case 258: //BitsPerSample
		_bitesPerSample = (short)value;
		break;

	case 273: //StripOffsets- For each strip, the byte offset of that strip.
		_stream->seekg(value, std::ios::beg);
		for (unsigned int i = 0; i < count; i++)
		{
			unsigned int stripOffset;
			_stream->read((char*)&stripOffset, sizeof(stripOffset));

			_stripOffsets.push_back(stripOffset);
		}
		break;

	case 278: //Rows per strip - RowsPerStrip and ImageLength together tell us the number of strips in the entire mage.
		//StripsPerImage = floor ((ImageLength + RowsPerStrip - 1) / RowsPerStrip).
		_expectedStripsPerImage = (_length + value - 1) / value;		
		break;

	case 279: //StripByteCounts - Number of bytes in strip after compression
		_stream->seekg(value, std::ios::beg);
		for (unsigned int i = 0; i < count; i++)
		{
			unsigned int stripByteCount;
			_stream->read((char*)&stripByteCount, sizeof(stripByteCount));
			_expectedBytesPerStrip.push_back(stripByteCount);
		}
		break;

	case 277: //Samples per pixel
		break;

	}
}
