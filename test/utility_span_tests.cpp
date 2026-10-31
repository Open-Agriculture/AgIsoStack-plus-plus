//================================================================================================
/// @file utility_span_tests.cpp
///
/// @brief Unit tests for the Span class.
/// @author Nik Vzdornov
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================

#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <cstddef>
#include <numeric>
#include <vector>

#include "isobus/utility/span.hpp"

namespace test_helpers
{
	template<typename T>
	std::vector<T> generate_test_data(size_t count)
	{
		std::vector<T> data(count);
		std::iota(data.begin(), data.end(), static_cast<T>(1));
		return data;
	}

	template<typename SpanType, typename Container>
	void verify_span_content(const SpanType &span, const Container &expected)
	{
		ASSERT_EQ(span.size(), expected.size());
		EXPECT_TRUE(std::equal(span.begin(), span.end(), std::begin(expected)));
	}
}

template<typename T>
class SPAN_TEST : public ::testing::Test
{
protected:
	using ElementType = T;

	void SetUp() override
	{
		test_data_ = test_helpers::generate_test_data<T>(10);
	}

	std::vector<T> test_data_;
};

using TestTypes = ::testing::Types<int>;
TYPED_TEST_SUITE(SPAN_TEST, TestTypes);

TYPED_TEST(SPAN_TEST, DefaultConstructor)
{
	using SpanType = isobus::Span<typename TestFixture::ElementType>;

	SpanType span;
	EXPECT_EQ(span.size(), 0);
	EXPECT_EQ(span.data(), nullptr);
	EXPECT_TRUE(span.empty());
}

TYPED_TEST(SPAN_TEST, PointerAndCountConstructor)
{
	using ElementType = typename TestFixture::ElementType;
	using SpanType = isobus::Span<ElementType>;

	auto &data = this->test_data_;
	SpanType span(data.data(), data.size());

	EXPECT_EQ(span.size(), data.size());
	EXPECT_EQ(span.data(), data.data());

	test_helpers::verify_span_content(span, data);
}

TYPED_TEST(SPAN_TEST, TwoIteratorsConstructor)
{
	using ElementType = typename TestFixture::ElementType;
	using SpanType = isobus::Span<ElementType>;

	auto &data = this->test_data_;
	SpanType span(data.begin(), data.end());

	EXPECT_EQ(span.size(), data.size());
	EXPECT_EQ(span.data(), data.data());

	test_helpers::verify_span_content(span, data);
}

TYPED_TEST(SPAN_TEST, CArrayConstructor)
{
	using ElementType = typename TestFixture::ElementType;

	ElementType arr[5];
	std::iota(std::begin(arr), std::end(arr), static_cast<ElementType>(1));

	isobus::Span<ElementType, 5> span(arr);

	EXPECT_EQ(span.size(), 5);
	EXPECT_EQ(span.data(), arr);

	EXPECT_TRUE(std::equal(span.begin(), span.end(), std::begin(arr)));
}

TYPED_TEST(SPAN_TEST, StdArrayConstructor)
{
	using ElementType = typename TestFixture::ElementType;

	std::array<ElementType, 5> arr;
	std::iota(arr.begin(), arr.end(), static_cast<ElementType>(1));

	isobus::Span<ElementType, 5> span(arr);

	EXPECT_EQ(span.size(), 5);
	EXPECT_EQ(span.data(), arr.data());

	test_helpers::verify_span_content(span, arr);
}

TYPED_TEST(SPAN_TEST, ConstStdArrayConstructor)
{
	using ElementType = typename TestFixture::ElementType;

	std::array<ElementType, 5> arr;
	std::iota(arr.begin(), arr.end(), static_cast<ElementType>(1));

	const std::array<ElementType, 5> const_arr = arr;
	isobus::Span<const ElementType, 5> span(const_arr);

	EXPECT_EQ(span.size(), 5);
	EXPECT_EQ(span.data(), const_arr.data());

	test_helpers::verify_span_content(span, const_arr);
}

TYPED_TEST(SPAN_TEST, InitializerListConstructor)
{
	const std::array<int, 5> expected = { 1, 2, 3, 4, 5 };
	isobus::Span<const int> span({ 1, 2, 3, 4, 5 });

	EXPECT_EQ(span.size(), 5);
	EXPECT_NE(span.data(), nullptr);

	EXPECT_TRUE(std::equal(span.begin(), span.end(), expected.begin()));
}

TYPED_TEST(SPAN_TEST, ConvertingConstructor)
{
	using ElementType = typename TestFixture::ElementType;

	ElementType arr[5];
	std::iota(std::begin(arr), std::end(arr), static_cast<ElementType>(1));

	isobus::Span<ElementType, 5> fixed_span(arr);
	isobus::Span<ElementType> dynamic_span(fixed_span);

	EXPECT_EQ(dynamic_span.size(), 5);
	EXPECT_EQ(dynamic_span.data(), arr);

	EXPECT_TRUE(std::equal(dynamic_span.begin(), dynamic_span.end(), std::begin(arr)));
}

TYPED_TEST(SPAN_TEST, ElementAccess)
{
	using ElementType = typename TestFixture::ElementType;
	using SpanType = isobus::Span<ElementType>;

	auto &data = this->test_data_;
	SpanType span(data.data(), data.size());

	for (size_t i = 0; i < span.size(); ++i)
	{
		EXPECT_EQ(span[i], data[i]);
		EXPECT_EQ(&span[i], &data[i]);
	}

	for (size_t i = 0; i < span.size(); ++i)
	{
		EXPECT_EQ(span.at(i), data[i]);
	}

	if (!span.empty())
	{
		EXPECT_EQ(span.front(), data.front());
		EXPECT_EQ(&span.front(), &data.front());

		EXPECT_EQ(span.back(), data.back());
		EXPECT_EQ(&span.back(), &data.back());
	}

	EXPECT_EQ(span.data(), data.data());
}

TYPED_TEST(SPAN_TEST, Iterators)
{
	using ElementType = typename TestFixture::ElementType;
	using SpanType = isobus::Span<ElementType>;

	auto &data = this->test_data_;
	SpanType span(data.data(), data.size());

	EXPECT_TRUE(std::equal(span.begin(), span.end(), data.begin()));

	EXPECT_TRUE(std::equal(span.rbegin(), span.rend(), data.rbegin()));

	const SpanType &const_span = span;
	EXPECT_TRUE(std::equal(const_span.cbegin(), const_span.cend(), data.begin()));
}

TYPED_TEST(SPAN_TEST, Observers)
{
	using ElementType = typename TestFixture::ElementType;
	using SpanType = isobus::Span<ElementType>;

	auto &data = this->test_data_;
	SpanType span(data.data(), data.size());

	EXPECT_EQ(span.size(), data.size());
	EXPECT_EQ(span.size_bytes(), data.size() * sizeof(ElementType));
	EXPECT_FALSE(span.empty());

	SpanType empty_span;
	EXPECT_TRUE(empty_span.empty());
	EXPECT_EQ(empty_span.size(), 0);
	EXPECT_EQ(empty_span.size_bytes(), 0);
}

TYPED_TEST(SPAN_TEST, FirstMethod)
{
	using ElementType = typename TestFixture::ElementType;
	using SpanType = isobus::Span<ElementType>;

	auto &data = this->test_data_;
	SpanType span(data.data(), data.size());

	const size_t count = 3;
	auto first_span = span.first(count);

	EXPECT_EQ(first_span.size(), count);
	EXPECT_EQ(first_span.data(), data.data());

	EXPECT_TRUE(std::equal(first_span.begin(), first_span.end(), data.begin()));

	auto fixed_first = span.template first<3>();
	static_assert(fixed_first.extent == 3, "Extent should be 3");
	EXPECT_EQ(fixed_first.size(), 3);
}

TYPED_TEST(SPAN_TEST, LastMethod)
{
	using ElementType = typename TestFixture::ElementType;
	using SpanType = isobus::Span<ElementType>;

	auto &data = this->test_data_;
	SpanType span(data.data(), data.size());

	const size_t count = 3;
	auto last_span = span.last(count);

	EXPECT_EQ(last_span.size(), count);
	EXPECT_EQ(last_span.data(), data.data() + (data.size() - count));

	EXPECT_TRUE(std::equal(last_span.begin(), last_span.end(), data.end() - count));
}

TYPED_TEST(SPAN_TEST, SubspanMethod)
{
	using ElementType = typename TestFixture::ElementType;
	using SpanType = isobus::Span<ElementType>;

	auto &data = this->test_data_;
	SpanType span(data.data(), data.size());

	const size_t offset = 2;
	const size_t count = 4;
	auto subspan = span.subspan(offset, count);

	EXPECT_EQ(subspan.size(), count);
	EXPECT_EQ(subspan.data(), data.data() + offset);

	EXPECT_TRUE(std::equal(subspan.begin(), subspan.end(), data.begin() + offset));

	auto to_end = span.subspan(offset);
	EXPECT_EQ(to_end.size(), data.size() - offset);

	auto fixed_subspan = span.template subspan<2, 4>();
	static_assert(fixed_subspan.extent == 4, "Extent should be 4");
	EXPECT_EQ(fixed_subspan.size(), 4);
}

TYPED_TEST(SPAN_TEST, MakeSpanUtilities)
{
	using ElementType = typename TestFixture::ElementType;

	auto &data = this->test_data_;
	auto span1 = isobus::make_span(data.data(), data.size());
	EXPECT_EQ(span1.size(), data.size());
	EXPECT_EQ(span1.data(), data.data());

	ElementType arr[5];
	std::iota(std::begin(arr), std::end(arr), static_cast<ElementType>(1));
	auto span2 = isobus::make_span(arr);
	static_assert(span2.extent == 5, "Extent should be 5");
	EXPECT_EQ(span2.size(), 5);
	EXPECT_TRUE(std::equal(span2.begin(), span2.end(), std::begin(arr)));

	std::array<ElementType, 5> std_arr;
	std::iota(std_arr.begin(), std_arr.end(), static_cast<ElementType>(1));
	auto span3 = isobus::make_span(std_arr);
	static_assert(span3.extent == 5, "Extent should be 5");
	EXPECT_EQ(span3.size(), 5);
	EXPECT_TRUE(std::equal(span3.begin(), span3.end(), std_arr.begin()));

	const std::array<ElementType, 5> const_std_arr = std_arr;
	auto span4 = isobus::make_span(const_std_arr);
	static_assert(span4.extent == 5, "Extent should be 5");
	EXPECT_EQ(span4.size(), 5);
	EXPECT_TRUE(std::equal(span4.begin(), span4.end(), const_std_arr.begin()));
}
