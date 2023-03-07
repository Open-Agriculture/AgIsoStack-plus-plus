// Note that this class currently only works for packets eight bytes or fewer.
class GroupBuilder
{
private:
	int bitOffset = 0;
	int bytesUsed = 0;
	char data[8];

};

