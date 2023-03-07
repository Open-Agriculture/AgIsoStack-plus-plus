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
		int bitOffset = 0;
		char data[8];

		unsigned int get_byte_offset() const
		{
			// Which byte to write to.
			return bitOffset / 8;
		}
		
		unsigned int get_bit_offset() const
		{
			// Which bit to write to in the current byte.
			return bitOffset % 8;
		}

		bool write_bits(unsigned char const * data, unsigned int bits)
		{
			// First make a backup of the current write position, so that if there is an error
			// writing some data we can roll back the entire change.  This is replicated in the
			// string writing code because it has to roll back the entire string, not just the last
			// character.
			unsigned int revert = bitOffset;
			
			return true;
		}

	public:
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
			unsigned int revert = bitOffset;
			while (*data)
			{
				if (!write_bits(data, 8))
				{
					bitOffset = revert;
					return false;
				}
			}
			if (includeNull)
			{
				unsigned char naught = 0;
				if (!write_bits(&naught, 8))
				{
					bitOffset = revert;
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
			unsigned int revert = bitOffset;
			unsigned char data = value ? 255 : 0;
			while (bits > 8)
			{
				if (!write_bits(&data, 8))
				{
					bitOffset = revert;
					return false;
				}
				bits -= 8;
			}
			if (bits)
			{
				if (!write_bits(&data, bits))
				{
					bitOffset = revert;
					return false;
				}
			}
			return true;
		}
	};
}

