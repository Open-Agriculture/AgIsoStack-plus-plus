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
		int bytesUsed = 0;
		char data[8];

	};
}

