//================================================================================================
/// @file to_string.hpp
///
/// @brief A compatibility template to replace `std::to_string`
/// @details Some compilers don't support `std::to_string` so this file is meant to abstract it
/// away with a workaround if it's not supported. The text matches what writing the value to a
/// default `std::ostringstream` gives, but no stream is used: streams link the standard library's
/// locale support into the program, which costs more than 100 KB of code on a microcontroller and
/// needs wide character functions that newlib-nano does not have.
/// @author Adrian Del Grosso
///
/// @copyright 2022 The Open-Agriculture Developers
//================================================================================================
#ifndef TO_STRING_HPP
#define TO_STRING_HPP

#include <cstdio>
#include <string>
#include <type_traits>

namespace isobus
{
	/// @brief Implementation details of isobus::to_string
	namespace to_string_detail
	{
		/// @brief Selects text: std::string, character arrays and character pointers
		struct TextTag
		{
		};
		/// @brief Selects a single character, which is how a stream writes the character types
		struct CharacterTag
		{
		};
		/// @brief Selects bool, which a stream writes as 1 or 0
		struct BoolTag
		{
		};
		/// @brief Selects the other integer types
		struct IntegerTag
		{
		};
		/// @brief Selects floating point numbers
		struct FloatingPointTag
		{
		};
		/// @brief Selects every other type, which isobus::to_string does not convert
		struct UnsupportedTag
		{
		};

		/// @brief Picks the tag for a type
		/// @tparam T The type to classify
		template<typename T>
		struct TagFor
		{
			/// @brief The type without const or volatile
			using Plain = typename std::remove_cv<T>::type;
			/// @brief What a pointer or array type points to, to recognise character strings
			using Element = typename std::remove_cv<typename std::remove_pointer<typename std::decay<Plain>::type>::type>::type;

			/// @brief True for std::string, character arrays and character pointers
			static constexpr bool IS_TEXT = std::is_same<Plain, std::string>::value ||
			  ((std::is_array<Plain>::value || std::is_pointer<Plain>::value) && std::is_same<Element, char>::value);
			/// @brief True for char, signed char and unsigned char, which includes std::int8_t and std::uint8_t
			static constexpr bool IS_CHARACTER = std::is_same<Plain, char>::value ||
			  std::is_same<Plain, signed char>::value ||
			  std::is_same<Plain, unsigned char>::value;

			/// @brief The selected tag
			using Type = typename std::conditional<
			  IS_TEXT,
			  TextTag,
			  typename std::conditional<
			    std::is_same<Plain, bool>::value,
			    BoolTag,
			    typename std::conditional<
			      IS_CHARACTER,
			      CharacterTag,
			      typename std::conditional<
			        std::is_integral<Plain>::value,
			        IntegerTag,
			        typename std::conditional<std::is_floating_point<Plain>::value, FloatingPointTag, UnsupportedTag>::type>::type>::type>::type>::type;
		};

		/// @brief Writes an unsigned number in decimal
		/// @param[in] value The magnitude of the number
		/// @param[in] negative Set to true to put a minus sign in front
		/// @returns The number in decimal
		inline std::string decimal(unsigned long long value, bool negative)
		{
			char buffer[24]; // The 20 digits of 2^64 - 1, a sign and the terminator
			char *position = buffer + sizeof(buffer);
			*--position = '\0';
			do
			{
				*--position = static_cast<char>('0' + (value % 10));
				value /= 10;
			} while (0 != value);
			if (negative)
			{
				*--position = '-';
			}
			return std::string(position);
		}

		/// @brief Converts a signed integer
		/// @param[in] value The integer
		/// @param[in] isSigned Selects this overload for signed types
		/// @returns The integer in decimal
		inline std::string integer(long long value, std::true_type isSigned)
		{
			(void)isSigned;
			// Negate in unsigned arithmetic, so that the most negative value doesn't overflow
			const unsigned long long magnitude = (value < 0) ? (0ULL - static_cast<unsigned long long>(value)) : static_cast<unsigned long long>(value);
			return decimal(magnitude, value < 0);
		}

		/// @brief Converts an unsigned integer
		/// @param[in] value The integer
		/// @param[in] isSigned Selects this overload for unsigned types
		/// @returns The integer in decimal
		inline std::string integer(unsigned long long value, std::false_type isSigned)
		{
			(void)isSigned;
			return decimal(value, false);
		}

		/// @brief Copies a std::string
		/// @param[in] value The string
		/// @returns A copy of `value`
		inline std::string text(const std::string &value)
		{
			return value;
		}

		/// @brief Copies a character string
		/// @param[in] value The character string, which may be null
		/// @returns A copy of `value`, or an empty string for a null pointer
		inline std::string text(const char *value)
		{
			return (nullptr != value) ? std::string(value) : std::string();
		}

		/// @brief Converts a floating point number like a default stream, which writes it like
		/// printf's %g: 6 significant digits, in exponent form when that is shorter
		/// @param[in] value The number
		/// @returns The number as text
		inline std::string floating_point(double value)
		{
			char buffer[32];
			const int length = std::snprintf(buffer, sizeof(buffer), "%g", value);
			return (length > 0) ? std::string(buffer) : std::string();
		}

		/// @brief Converts a long double like a default stream
		/// @param[in] value The number
		/// @returns The number as text
		inline std::string floating_point(long double value)
		{
			char buffer[32];
			const int length = std::snprintf(buffer, sizeof(buffer), "%Lg", value);
			return (length > 0) ? std::string(buffer) : std::string();
		}

		template<typename T>
		std::string convert(const T &value, TextTag)
		{
			return text(value);
		}

		template<typename T>
		std::string convert(const T &value, CharacterTag)
		{
			return std::string(1, static_cast<char>(value));
		}

		template<typename T>
		std::string convert(const T &value, BoolTag)
		{
			return value ? "1" : "0";
		}

		template<typename T>
		std::string convert(const T &value, IntegerTag)
		{
			using IsSigned = std::integral_constant<bool, std::is_signed<T>::value>;
			using Wide = typename std::conditional<IsSigned::value, long long, unsigned long long>::type;
			return integer(static_cast<Wide>(value), IsSigned());
		}

		template<typename T>
		std::string convert(const T &value, FloatingPointTag)
		{
			using Wide = typename std::conditional<std::is_same<typename std::remove_cv<T>::type, long double>::value, long double, double>::type;
			return floating_point(static_cast<Wide>(value));
		}

		template<typename T>
		std::string convert(const T &, UnsupportedTag)
		{
			static_assert(!std::is_same<T, T>::value, "isobus::to_string converts numbers and strings only. Cast an enumeration to an integer first.");
			return std::string();
		}
	} // namespace to_string_detail

	template<typename T>
	/// @brief A replacement for std::to_string
	/// @tparam T The data type
	/// @param t The thing to convert to string
	/// @returns the string form of `t`
	std::string to_string(T const &t)
	{
		return to_string_detail::convert(t, typename to_string_detail::TagFor<T>::Type());
	}
} // namespace isobus

#endif // TO_STRING_HPP
