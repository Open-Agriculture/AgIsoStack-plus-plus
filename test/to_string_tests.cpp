//================================================================================================
/// @file to_string_tests.cpp
///
/// @brief Checks that isobus::to_string gives the same text as writing the value to a default
/// std::ostringstream, which is what it was implemented with before.
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#include <gtest/gtest.h>

#include "isobus/isobus/isobus_virtual_terminal_objects.hpp"
#include "isobus/utility/to_string.hpp"

#include <cstdint>
#include <limits>
#include <sstream>
#include <string>

namespace
{
	template<typename T>
	std::string stream_text(const T &value)
	{
		std::ostringstream stream;
		stream << value;
		return stream.str();
	}

	template<typename T>
	void expect_like_stream(const T &value)
	{
		EXPECT_EQ(stream_text(value), isobus::to_string(value));
	}

	template<typename T>
	void expect_integer_limits_like_stream()
	{
		expect_like_stream(static_cast<T>(0));
		expect_like_stream(static_cast<T>(1));
		expect_like_stream(static_cast<T>(42));
		expect_like_stream(std::numeric_limits<T>::min());
		expect_like_stream(std::numeric_limits<T>::max());
		expect_like_stream(static_cast<T>(std::numeric_limits<T>::max() - 1));
	}
} // namespace

TEST(TO_STRING_TESTS, SignedIntegersMatchStream)
{
	expect_integer_limits_like_stream<short>();
	expect_integer_limits_like_stream<int>();
	expect_integer_limits_like_stream<long>();
	expect_integer_limits_like_stream<long long>();
	expect_integer_limits_like_stream<std::int16_t>();
	expect_integer_limits_like_stream<std::int32_t>();
	expect_integer_limits_like_stream<std::int64_t>();
	expect_like_stream(-1);
	expect_like_stream(-10);
	expect_like_stream(-123456789);
	expect_like_stream(1000000);
}

TEST(TO_STRING_TESTS, UnsignedIntegersMatchStream)
{
	expect_integer_limits_like_stream<unsigned short>();
	expect_integer_limits_like_stream<unsigned int>();
	expect_integer_limits_like_stream<unsigned long>();
	expect_integer_limits_like_stream<unsigned long long>();
	expect_integer_limits_like_stream<std::uint16_t>();
	expect_integer_limits_like_stream<std::uint32_t>();
	expect_integer_limits_like_stream<std::uint64_t>();
	expect_integer_limits_like_stream<std::size_t>();
	expect_like_stream(10u);
	expect_like_stream(0xFFFFFFFFu);
}

TEST(TO_STRING_TESTS, ConstIntegersMatchStream)
{
	const std::uint32_t pgn = 0xEF00;
	const volatile int address = 0x81;
	EXPECT_EQ("61184", isobus::to_string(pgn));
	EXPECT_EQ("129", isobus::to_string(address));
}

TEST(TO_STRING_TESTS, CharacterTypesAreWrittenAsCharacters)
{
	// A stream writes the character types, std::int8_t and std::uint8_t among them, as a character
	expect_like_stream('A');
	expect_like_stream(static_cast<signed char>('z'));
	expect_like_stream(static_cast<unsigned char>('0'));
	expect_like_stream(static_cast<std::uint8_t>(65));
	expect_like_stream(static_cast<std::int8_t>(66));
	EXPECT_EQ("A", isobus::to_string(static_cast<std::uint8_t>(65)));
}

TEST(TO_STRING_TESTS, BoolMatchesStream)
{
	expect_like_stream(true);
	expect_like_stream(false);
	EXPECT_EQ("1", isobus::to_string(true));
	EXPECT_EQ("0", isobus::to_string(false));
}

TEST(TO_STRING_TESTS, FloatingPointMatchesStream)
{
	expect_like_stream(0.0);
	expect_like_stream(-0.0);
	expect_like_stream(1.5);
	expect_like_stream(-2.25);
	expect_like_stream(0.1);
	expect_like_stream(1.0 / 3.0);
	expect_like_stream(123456.0);
	expect_like_stream(1234567.0);
	expect_like_stream(1e-5);
	expect_like_stream(6.02214076e23);
	expect_like_stream(1731330000.0);
	expect_like_stream(std::numeric_limits<double>::max());
	expect_like_stream(std::numeric_limits<double>::infinity());
	expect_like_stream(-std::numeric_limits<double>::infinity());
	expect_like_stream(0.5f);
	expect_like_stream(3.14159265f);
	expect_like_stream(static_cast<long double>(2.5));
	EXPECT_EQ("1.73133e+09", isobus::to_string(1731330000.0));
}

TEST(TO_STRING_TESTS, TextIsCopied)
{
	const std::string name = "AgIsoStack";
	const char *pointer = "VT";
	char buffer[] = "TC";
	expect_like_stream(name);
	expect_like_stream(pointer);
	expect_like_stream(buffer);
	expect_like_stream("literal");
	expect_like_stream(std::string());
	EXPECT_EQ("literal", isobus::to_string("literal"));
}

TEST(TO_STRING_TESTS, NullCharacterPointerIsEmpty)
{
	const char *nothing = nullptr;
	EXPECT_EQ("", isobus::to_string(nothing));
}

TEST(TO_STRING_TESTS, ConcatenatesLikeTheLogMessages)
{
	const std::uint8_t address = 0x1C;
	EXPECT_EQ("[AC]: Address 28 claimed, PGN 61184",
	          "[AC]: Address " + isobus::to_string(static_cast<int>(address)) + " claimed, PGN " + isobus::to_string(0xEF00));
}

TEST(TO_STRING_TESTS, ObjectIdToStringWritesNullOrTheNumberLikeStream)
{
	// object_id_to_string used to write the ID to a stream; the VT client puts it in log messages
	EXPECT_EQ("NULL", isobus::object_id_to_string(isobus::NULL_OBJECT_ID));
	for (const std::uint16_t objectId : { static_cast<std::uint16_t>(0), static_cast<std::uint16_t>(1), static_cast<std::uint16_t>(1000), static_cast<std::uint16_t>(65534) })
	{
		EXPECT_EQ(stream_text(objectId), isobus::object_id_to_string(objectId));
	}
}
