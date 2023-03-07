namespace isobus
{
	// Note that this class currently only works for packets eight bytes or fewer, because that is
	// all the use-cases I needed.  The spec has a whole section on how data can span byte
	// boundaries, and how to align the bits when a data-type that is not a multiple of eight bits
	// crosses said boundaries.  Then every single parameter group uses padding to avoid those
	// cases!  The five-bit tractor command types have three bits of padding to make them exactly
	// one byte.  Why even specify the scheme if it isn't used?
	//
	// I wrote a huge amount of this file once, then somehow it vanished.  I don't know how!
	class GroupBuilder
	{
	private:
		int writeOffset = 0;
		int readOffset = 0;
		unsigned char buffer[8];

		unsigned int get_write_byte_offset() const
		{
			// Which byte to write to.
			return writeOffset / 8;
		}

		unsigned int get_write_bit_offset() const
		{
			// Which bit to write to in the current byte.
			return writeOffset % 8;
		}

		unsigned int get_read_byte_offset() const
		{
			// Which byte to read from.
			return readOffset / 8;
		}

		unsigned int get_read_bit_offset() const
		{
			// Which bit to read from in the current byte.
			return readOffset % 8;
		}

		bool write_bits(unsigned char const * data, unsigned int bits)
		{
			if (bits == 0)
			{
				// Err, OK, I guess...
				return true;
			}
			// First make a backup of the current write position, so that if there is an error
			// writing some data we can roll back the entire change.  This is replicated in the
			// string writing code because it has to roll back the entire string, not just the last
			// character.
			unsigned int revert = writeOffset;
			unsigned int byte = get_write_byte_offset();
			unsigned int offset = get_write_bit_offset();
			// Adjust first, and revert later.
			writeOffset += bits;

			// -------------------------------
			//  Stage naught - trivial cases.
			// -------------------------------

			// How much space is there left in this byte?
			unsigned int remaining = 8 - offset;
			if (remaining >= bits)
			{
				if (byte == 8)
				{
					// Out of space for an eight byte packet.
					writeOffset = revert;
					return false;
				}
				// Everything will fit in the current byte, which must mean there's at most one byte
				// of data to write.  Hence we put this version first because it covers more single
				// byte cases.
				unsigned char mask = (1 << offset) - 1;
				buffer[byte] = (buffer[byte] & mask) | (*data << offset);
				if (writeOffset % 8 != 0)
				{
					// Mask the top bits to hide excess data.
					buffer[byte] |= 255 << (writeOffset % 8);
				}
				return true;
			}

			// Whole bytes (one byte is often handled above.)
			if (offset == 0 && bits % 8 == 0)
			{
				// Whole bytes, which are aligned.
				do
				{
					if (byte == 8)
					{
						// Out of space for an eight byte packet.
						writeOffset = revert;
						return false;
					}
					buffer[byte] = data++;
					byte += 1;
					bits -= 8;
				}
				while (bits);
				// No additional masking required in this case.
				return true;
			}

			// ----------------------------------------
			//  Stage one - fill out the current byte.
			// ----------------------------------------

			// All this data can come from the first byte of the input.
			if (byte == 8)
			{
				// Out of space for an eight byte packet.
				writeOffset = revert;
				return false;
			}
			// Everything will fit in the current byte, which must mean there's at most one byte
			// of data to write.  Hence we put this version first because it covers more single
			// byte cases.
			unsigned char mask = (1 << offset) - 1;
			buffer[byte] = (buffer[byte] & mask) | (*data << offset);
			bits -= remaining;
			++byte;

			// -------------------------------
			//  Stage two - copy whole bytes.
			// -------------------------------

			while (bits > 8)
			{
				if (byte == 8)
				{
					// Out of space for an eight byte packet.
					writeOffset = revert;
					return false;
				}
				buffer[byte] = (*data >> remaining);
				++data;
				buffer[byte] = (buffer[byte] & mask) | (*data << offset);
				bits -= 8;
				++byte;
			}

			// -------------------------------
			//  Stage three - copy whole bytes.
			// -------------------------------

			// `bits` is number of bits left.  It may or may not span a byte boundary in the input.
			if (bits == 0)
			{
				// Nothing left to copy.
				if (writeOffset % 8 != 0)
				{
					// Mask the top bits to hide excess data.
					buffer[byte] |= 255 << (writeOffset % 8);
				}
				return true;
			}

			// I'm sure this code can be compressed with the code above.
			if (byte == 8)
			{
				// Out of space for an eight byte packet.
				writeOffset = revert;
				return false;
			}
			buffer[byte] = (*data >> remaining);
			if (offset < bits)
			{
				// The final output spans two bytes of input.
				++data;
				buffer[byte] = (buffer[byte] & mask) | (*data << offset);
			}
			if (writeOffset % 8 != 0)
			{
				// Mask the top bits to hide excess data.
				buffer[byte] |= 255 << (writeOffset % 8);
			}

			return true;
		}
		
		bool read_bits(unsigned char * data, unsigned int bits)
		{
			unsigned int revert = readOffset;
			unsigned int byte = get_read_byte_offset();
			unsigned int input = get_read_bit_offset();
			unsigned int remaining = 8 - input;
			unsigned int output = 0;
			unsigned int space = 8 - output;
			// Mark as read, even though we actually haven't yet.
			readOffset += bits;
			// Initialise the current destination byte.
			*data = 0;
			while (bits)
			{
				if (byte == 8)
				{
					// Ran out of data.
					readOffset = revert;
					return false;
				}
				// Move in the new data.
				*data = (*data & ~(255 << output)) | ((buffer[byte] >> input) << output);
				if (space > remaining)
				{
					if (remaining >= bits)
					{
						bits = 0;
					}
					else
					{
						bits -= remaining;
					}
					space -= remaining;
					output = 8 - space;
					remaining = 0;
					input = 0;
					++byte;
				}
				else if (remaining > space)
				{
					if (space >= bits)
					{
						bits = 0;
					}
					else
					{
						bits -= space;
					}
					remaining -= space;
					input = 8 - remaining;
					space = 8;
					output = 0;
					++data;
					if (bits)
					{
						*data = 0;
					}
				}
				else
				{
					if (remaining >= bits)
					{
						bits = 0;
					}
					else
					{
						bits -= space;
					}
					// Perfectly matched.
					remaining = 8;
					input = 0;
					space = 8;
					output = 0;
					++data;
					if (bits)
					{
						*data = 0;
					}
					++byte;
				}
			}
			if (output)
			{
				// Bits left to mask.
				*data = *data & ~(255 << output);
			}
			return true;
		}

	public:
		unsigned int get_written_bits() const
		{
			return writeOffset;
		}

		unsigned int get_written_bytes() const
		{
			return (writeOffset + 7) / 8;
		}

		unsigned int get_read_bits() const
		{
			return readOffset;
		}

		unsigned int get_read_bytes() const
		{
			return (readOffset + 7) / 8;
		}

		template <typename T>
		bool write(T const & data)
		{
			return write_bits((unsigned char const *)&data, sizeof (T) * 8);
		}

		template <typename T>
		bool write(T const & data, unsigned int bits)
		{
			return write_bits((unsigned char const *)&data, bits);
		}

		template <>
		bool write<bool>(bool const & data)
		{
			unsigned char bits = data ? 255 : 0;
			return write_bits(&bits, 1);
		}

		template <>
		bool write<char const *>(char const * const & data)
		{
			// What should the default for including NULL be?
			return write((unsigned char const *)data, false);
		}

		template <>
		bool write<char *>(char * const & data)
		{
			// What should the default for including NULL be?
			return write((unsigned char const *)data, false);
		}

		template <>
		bool write<unsigned char const *>(unsigned char const * const & data)
		{
			// What should the default for including NULL be?
			return write((unsigned char const *)data, false);
		}

		template <>
		bool write<unsigned char *>(unsigned char * const & data)
		{
			// What should the default for including NULL be?
			return write((unsigned char const *)data, false);
		}

		bool write(char const * data, bool includeNull)
		{
			return write((unsigned char const *)data, includeNull);
		}

		bool write(char * data, bool includeNull)
		{
			return write((unsigned char const *)data, includeNull);
		}

		bool write(unsigned char const * data, bool includeNull)
		{
			// Base case.  Write each byte separately so they don't get put in little-endian, which
			// makes no sense for strings.
			unsigned int revert = writeOffset;
			while (*data)
			{
				if (!write_bits(data, 8))
				{
					writeOffset = revert;
					return false;
				}
				++data;
			}
			if (includeNull)
			{
				unsigned char naught = 0;
				if (!write_bits(&naught, 8))
				{
					writeOffset = revert;
					return false;
				}
			}
			return true;
		}

		bool write(unsigned char * data, bool includeNull)
		{
			return write((unsigned char const *)data, includeNull);
		}

		bool pad(unsigned int bits, bool value = true)
		{
			unsigned int revert = writeOffset;
			unsigned char data = value ? 255 : 0;
			unsigned int byte = get_write_byte_offset();
			unsigned int offset = get_write_bit_offset();
			unsigned int remaining = 8 - offset;
			unsigned char mask = 255 >> remaining;
			writeOffset += bits;
			for ( ; ; )
			{
				if (byte == 8)
				{
					writeOffset = revert;
					return false;
				}
				buffer[byte] = (buffer[byte] & mask) | (data << offset);
				if (remaining > bits)
				{
					// Weirdly we need a second masking here, to keep untouched bits as `1`.
					buffer[byte] |= 255 << (bits + offset);
					break;
				}
				else if (remaining == bits)
				{
					// Done.
					break;
				}
				else
				{
					bits -= remaining;
					offset = 0;
					remaining = 8;
					mask = 0;
					++byte;
				}
			}
			return true;
		}

		template <typename T>
		bool read(T & data)
		{
			return read_bits((unsigned char *)&data, sizeof (T) * 8);
		}

		template <typename T>
		bool read(T & data, unsigned int bits)
		{
			return read_bits((unsigned char *)&data, bits);
		}

		template <>
		bool read<bool>(bool & data)
		{
			unsigned char bits = 0;
			if (read_bits(&bits, 1))
			{
				data = !!bits;
				return true;
			}
			return false;
		}

		template <>
		bool read<char *>(char * & data)
		{
			// Read until NULL.
			return read((unsigned char *)data);
		}

		template <>
		bool read<unsigned char *>(unsigned char * & data)
		{
			// Read until NULL.
			unsigned int revert = readOffset;
			// Don't modify `data`!
			unsigned char * ptr = data;
			for ( ; ; )
			{
				if (!read_bits(ptr, 8))
				{
					readOffset = revert;
					return false;
				}
				if (*ptr == 0)
				{
					// Found a NULL byte.
					break;
				}
				++ptr;
			}
			return true;
		}

		template <>
		bool read<char *>(char * & data, unsigned int bits)
		{
			// It is a bit awkward to specify how much of a string to read.
			return read((unsigned char *)data, bits);
		}

		template <>
		bool read<unsigned char *>(unsigned char * & data, unsigned int bits)
		{
			if (bits % 8 != 0)
			{
				// This requires normal string sizes.
				return false;
			}
			// Don't modify `data`!
			unsigned char * ptr = data;
			// Don't write NULL, just assume the caller handles that.
			unsigned int revert = readOffset;
			while (bits)
			{
				if (!read_bits(ptr, 8))
				{
					readOffset = revert;
					return false;
				}
				bits -= 8;
				++ptr;
			}
			return true;
		}

		bool skip(unsigned int bits)
		{
			// Easy!
			readOffset += bits;
		}
	};
}

