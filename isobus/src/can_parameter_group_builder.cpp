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
			unsigned int bit = get_write_bit_offset();
			// Adjust first, and revert later.
			writeOffset += bits;

			// -------------------------------
			//  Stage naught - trivial cases.
			// -------------------------------

			// How much space is there left in this byte?
			unsigned int remaining = 8 - bit;
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
				unsigned char mask = (1 << bit) - 1;
				buffer[byte] = (buffer[byte] & mask) | (*data << bit);
				return true;
			}
			
			// Whole bytes (one byte is often handled above.)
			if (bit == 0 && bits % 8 == 0)
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
			unsigned char mask = (1 << bit) - 1;
			buffer[byte] = (buffer[byte] & mask) | (*data << bit);
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
				buffer[byte] = (buffer[byte] & mask) | (*data << bit);
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
			if (bit < bits)
			{
				// The final output spans two bytes of input.
				++data;
				buffer[byte] = (buffer[byte] & mask) | (*data << bit);
			}
			
			return true;
		}

	public:
		unsigned int get_written_bits() const
		{
			return writeOffset;
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
			if (data)
			{
				unsigned char one = 255;
				return write_bits(&one, 1);
			}
			else
			{
				unsigned char naught = 0;
				return write_bits(&naught, 1);
			}
		}

		template <>
		bool write<char *>(bool const & data)
		{
			// Only use a single bit for booleans.
			if (data)
			{
				unsigned char one = 255;
				return write_bits(&one, 1);
			}
			else
			{
				unsigned char naught = 0;
				return write_bits(&naught, 1);
			}
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
			// Only use a single bit for booleans.
			unsigned int revert = writeOffset;
			unsigned char data = value ? 255 : 0;
			while (bits > 8)
			{
				if (!write_bits(&data, 8))
				{
					writeOffset = revert;
					return false;
				}
				bits -= 8;
			}
			if (bits)
			{
				if (!write_bits(&data, bits))
				{
					writeOffset = revert;
					return false;
				}
			}
			return true;
		}
	};
}

