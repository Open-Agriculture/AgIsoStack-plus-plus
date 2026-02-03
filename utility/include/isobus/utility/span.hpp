//================================================================================================
/// @file span.hpp
///
/// @brief Implementation of std::span-like class for freestanding C++14.
/// Based on C++20 std::span [views.span] (ISO/IEC 14882:2020 [views.span]).
///
/// @note This implementation provides a C++14 freestanding version of std::span without RTTI
///       or exceptions. Some facilities marked as "freestanding-deleted" in the standard are
///       either omitted or provide freestanding-compatible alternatives.
/// @author Nik Vzdornov
///
/// @copyright 2026 Open Agriculture
//================================================================================================
#ifndef SPAN_HPP
#define SPAN_HPP

#include <array>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <type_traits>

namespace isobus
{
	// [span.syn] 23.7.2.1 Header <span> synopsis
	/// @brief Constant indicating dynamic extent in span template parameter.
	/// Defined in [span.overview]/1: constant representing dynamic_extent in std::span.
	static constexpr std::size_t DYNAMIC_EXTENT = std::numeric_limits<std::size_t>::max();

	namespace detail
	{
		/// @brief Byte type for as_bytes functions.
		/// Defined in [span.objectrep]: byte type used to represent object representation.
		enum class byte : unsigned char
		{
		};

		/// @brief Polyfill for std::to_address (C++20) for C++14 freestanding.
		/// Converts iterator or pointer to address as per [span.cons]/6 requirements.
		/// @tparam T Pointer type
		/// @param pointer Raw pointer
		/// @return The pointer itself
		/// @note Freestanding-compatible alternative to C++20 std::to_address.
		template<typename T>
		constexpr T *to_address(T *pointer) noexcept
		{
			static_assert(!std::is_function<T>::value, "T cannot be a function type");
			return pointer;
		}

		/// @brief Polyfill for std::to_address for iterator types.
		/// Extracts underlying pointer from contiguous iterator.
		/// @tparam It Iterator type
		/// @param iterator Contiguous iterator
		/// @return Underlying pointer obtained via std::addressof(*iterator)
		template<typename It>
		constexpr auto to_address(const It &iterator) noexcept
		  -> decltype(std::addressof(*iterator))
		{
			return std::addressof(*iterator);
		}

		/// @brief Helper to extract element type from iterator dereferencing.
		/// Per C++20 std::iter_reference_t [iterator.traits.types].
		/// For C++14 compatibility without concepts, uses decltype(*std::declval<T&>()).
		/// @note Per standard [span.cons]/8.1: returns iter_reference_t<T> which is the
		///       reference type obtained by dereferencing. When used in constraints,
		///       remove_reference_t is applied to get the element type U.
		template<typename T>
		struct iterator_reference_type
		{
			/// Type obtained from dereferencing T, equivalent to iter_reference_t<T> in C++20.
			/// This may be a reference type and needs remove_reference_t when used per standard.
			using type = decltype(*std::declval<T &>());
		};

		template<typename T>
		struct has_proper_to_address
		{
		private:
			using ref_type = typename iterator_reference_type<T>::type;
			using expected_ptr_type = typename std::add_pointer<ref_type>::type;

			template<typename U>
			static auto test(int) -> decltype(std::is_same<
			                                    decltype(detail::to_address(std::declval<const U &>())),
			                                    expected_ptr_type>::value,
			                                  std::true_type{});

			template<typename>
			static std::false_type test(...);

		public:
			static constexpr bool value = decltype(test<T>(0))::value;
		};

		template<typename I>
		struct has_contiguous_iterator_category
		{
		private:
			template<typename U>
			static auto test(int) -> decltype(std::is_same<
			                                    typename std::iterator_traits<U>::iterator_category,
			                                    std::random_access_iterator_tag>::value ||
			                                    std::is_pointer<U>::value ||
			                                    std::is_same<U, typename std::remove_pointer<U>::type *>::value,
			                                  std::true_type{});

			template<typename>
			static std::false_type test(...);

		public:
			static constexpr bool value = decltype(test<I>(0))::value;
		};

		template<typename T>
		struct is_random_access_iterator
		{
		private:
			template<typename U>
			static auto test(int) -> decltype(std::declval<U &>() + std::declval<std::ptrdiff_t>(),
			                                  std::declval<std::ptrdiff_t>() + std::declval<U &>(),
			                                  std::declval<U &>() - std::declval<std::ptrdiff_t>(),
			                                  std::declval<U &>() += std::declval<std::ptrdiff_t>(),
			                                  std::declval<U &>() -= std::declval<std::ptrdiff_t>(),
			                                  std::declval<const U &>()[std::declval<std::ptrdiff_t>()],
			                                  std::declval<U &>() - std::declval<U &>(),
			                                  std::true_type{});

			template<typename>
			static std::false_type test(...);

		public:
			static constexpr bool value = decltype(test<T>(0))::value;
		};

		/// @brief Type trait to check if type is a contiguous iterator (pointer-like).
		/// Used in constraints to validate iterator requirements per [span.cons].
		/// @note In C++14, we simply check main properties of contiguos iterators.
		template<typename I>
		struct is_contiguous_iterator
		{
			static constexpr bool value =
			  is_random_access_iterator<I>::value &&
			  std::is_lvalue_reference<typename iterator_reference_type<I>::type>::value &&
			  has_proper_to_address<I>::value &&
			  has_contiguous_iterator_category<I>::value;
		};

		/// @brief Helper for computing extent in subspan.
		/// Calculates the Extent template parameter for subspan return type per [span.sub]/10.
		/// @tparam Extent Original span extent
		/// @tparam Offset Offset parameter to subspan
		/// @tparam Count Count parameter to subspan
		template<std::size_t Extent, std::size_t Offset, std::size_t Count>
		struct subspan_extent
		{
			/// Computed extent: Count if specified, else Extent-Offset if known, else DYNAMIC_EXTENT.
			static constexpr std::size_t value =
			  (Count != DYNAMIC_EXTENT) ? Count
			                            : ((Extent != DYNAMIC_EXTENT) ? Extent - Offset : DYNAMIC_EXTENT);
		};
	} // namespace detail

	//================================================================================================
	/// @class span
	///
	/// @brief A non-owning view over a contiguous sequence of objects.
	/// Based on C++20 std::span [views.span] (ISO/IEC 14882:2020 [views.span]).
	/// Fully freestanding C++14 compatible.
	///
	/// @tparam T Element type (per [span.overview]/4: must be a complete object type,
	///           not abstract)
	/// @tparam Extent Fixed extent (number of elements), or DYNAMIC_EXTENT for runtime size
	///
	/// @note Per [span.overview]/3: span is trivially copyable.
	/// @note Per [span.overview]/5: operations that invalidate pointers in
	///       [data(), data() + size()) invalidate all span pointers, iterators, and references.
	///
	/// @note Constraint Strategy (static_assert vs enable_if):
	/// - **Mandates** [standard term]: compile-time requirements that cannot be violated at
	///   instantiation. Implemented via static_assert to provide clear, immediate compilation
	///   errors and prevent template instantiation with invalid parameters. Examples:
	///   - Default constructor: Extent == DYNAMIC_EXTENT || Extent == 0 [span.cons]/1
	///   - first<Count>(): Count <= Extent [span.sub]/1
	///   - C array constructors: N == Extent (if not dynamic) [span.cons]/13
	///
	/// - **Template Constraints** (SFINAE):
	///   - Iterator type checking: It satisfies contiguous_iterator [span.cons]/3.1
	///   - Element type conversion: is_convertible_v<U(*)[], T(*)[]> [span.cons]/3.2
	///
	/// - **Hardened Preconditions** (runtime): conditions on parameter values that cannot
	///   be checked at compile-time. In freestanding mode, these are not checked (no exceptions).
	///   Examples:
	///   - count <= size() for first(count) [span.sub]/11
	///   - idx < size() for operator[](idx) [span.elem]/1
	///
	/// Per C++14 limitations, explicit(condition) syntax is not available, so all template
	/// constructors are unconditionally explicit.
	//================================================================================================
	template<typename T, std::size_t Extent = DYNAMIC_EXTENT>
	class Span
	{
		static_assert(!std::is_abstract<T>::value,
		              "ElementType cannot be an abstract class type [span.overview]");
		static_assert(std::is_object<T>::value,
		              "ElementType must be a complete object type [span.overview]");

	public:
		// [span.overview] 23.7.2.2.1 Overview - constants and types
		/// @brief Element type of the span [span.overview]
		using element_type = T;
		/// @brief Value type (cv-unqualified element type) [span.overview]
		using value_type = typename std::remove_cv<T>::type;
		/// @brief Size type [span.overview]
		using size_type = std::size_t;
		/// @brief Difference type [span.overview]
		using difference_type = std::ptrdiff_t;
		/// @brief Pointer type [span.overview]
		using pointer = T *;
		/// @brief Const pointer type [span.overview]
		using const_pointer = const T *;
		/// @brief Reference type [span.overview]
		using reference = T &;
		/// @brief Const reference type [span.overview]
		using const_reference = const T &;
		/// @brief Iterator type - raw pointer (contiguous_iterator model) [span.iterators]/1
		using iterator = T *;
		/// @brief Const iterator type [span.iterators]/1
		using const_iterator = const T *;
		/// @brief Reverse iterator type [span.iterators]
		using reverse_iterator = std::reverse_iterator<iterator>;
		/// @brief Const reverse iterator type [span.iterators]
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		/// @brief Extent of the span (DYNAMIC_EXTENT for dynamic extent) [span.overview]
		static constexpr size_type extent = Extent;

		// [span.cons] 23.7.2.2.2 Constructors, copy, and assignment

		/// @brief Default constructor [span.cons]/1-2
		/// @constraints Extent == DYNAMIC_EXTENT || Extent == 0
		/// @postconditions size() == 0 && data() == nullptr
		/// @note Using static_assert for compile-time constraints verification per
		///       [span.cons]/1: these are compile-time Constraints that must be checked at
		///       instantiation. static_assert is preferred here as it prevents template
		///       instantiation entirely when violated, providing earlier and clearer errors
		///       compared to SFINAE-based enable_if.
		constexpr Span() noexcept :
		  data_(nullptr),
		  size_(0)
		{
			static_assert((Extent == DYNAMIC_EXTENT) || (0 == Extent),
			              "Default constructor only for dynamic extent or zero extent [span.cons]");
		}

		/// @brief Constructor from iterator and count [span.cons]/3-7
		/// @param[in] first Contiguous iterator to the first element
		/// @param[in] count Number of elements
		/// @constraints Let U be remove_reference_t<iter_reference_t<It>>.
		///   - It satisfies contiguous_iterator [span.cons]/3.1
		///   - is_convertible_v<U(*)[], element_type(*)[]> is true [span.cons]/3.2
		/// @preconditions [first, first + count) is a valid range [span.cons]/4.1
		/// @preconditions It models contiguous_iterator [span.cons]/4.2
		/// @postconditions data() == to_address(first) && size() == count
		/// @note Using enable_if for SFINAE-based constraint checking because:
		///   - enable_if allows partial template specialization matching during overload resolution
		///   - These are template constraints requiring compile-time type checking
		///   - Multiple enable_if conditions can be combined for complex constraints
		///   - Per C++14 limitations, we cannot use explicit(Extent != DYNAMIC_EXTENT) syntax
		template<typename It, typename = typename std::enable_if<detail::is_contiguous_iterator<It>::value && std::is_convertible<typename std::remove_reference<typename detail::iterator_reference_type<It>::type>::type (*)[], element_type (*)[]>::value>::type>
		constexpr explicit Span(It first, size_type count) noexcept :
		  data_(detail::to_address(first)),
		  size_(count)
		{
			// Precondition checks omitted for freestanding (no exceptions/debugging)
			// Clients must ensure [first, first + count) is valid
		}

		/// @brief Constructor from two iterators [span.cons]/8-12
		/// @param[in] first Contiguous iterator to the first element
		/// @param[in] last Sized sentinel for the past-the-end position
		/// @constraints Let U be remove_reference_t<iter_reference_t<It>>.
		///   - is_convertible_v<U(*)[], element_type(*)[]> is true [span.cons]/8.2
		///   - It satisfies contiguous_iterator [span.cons]/8.2
		///   - End satisfies sized_sentinel_for<It> [span.cons]/8.3
		///   - is_convertible_v<End, size_t> is false [span.cons]/8.4
		/// @preconditions [first, last) is a valid range [span.cons]/9.1
		/// @preconditions It models contiguous_iterator [span.cons]/9.2
		/// @preconditions End models sized_sentinel_for<It> [span.cons]/9.3
		/// @postconditions data() == to_address(first) && size() == (last - first)
		template<typename It, typename End, typename = typename std::enable_if<detail::is_contiguous_iterator<It>::value && !std::is_convertible<End, std::size_t>::value && std::is_convertible<typename std::remove_reference<typename detail::iterator_reference_type<It>::type>::type (*)[], element_type (*)[]>::value>::type>
		constexpr explicit Span(It first, End last) noexcept :
		  data_(detail::to_address(first)),
		  size_(static_cast<size_type>(last - first))
		{
			// Precondition checks omitted for freestanding (no exceptions/debugging)
		}

		/// @brief Constructor from C array [span.cons]/13-15
		/// @param[in] arr C-style array reference
		/// @constraints extent == DYNAMIC_EXTENT || N == extent
		/// @postconditions size() == N && data() == arr
		/// @note C++14 compatible - no type_identity_t available, using reference instead
		template<std::size_t N>
		constexpr Span(element_type (&arr)[N]) noexcept :
		  data_(arr),
		  size_(N)
		{
			static_assert((Extent == DYNAMIC_EXTENT) || (N == Extent),
			              "Array size must match extent [span.cons]");
		}

		/// @brief Constructor from std::array [span.cons]/13-15
		/// @param[in] arr std::array reference
		/// @tparam U Element type of array
		/// @tparam N Size of array
		/// @constraints extent == DYNAMIC_EXTENT || N == extent
		/// @constraints is_convertible_v<U(*)[], element_type(*)[]> is true
		/// @postconditions size() == N && data() == arr.data()
		template<typename U, std::size_t N>
		constexpr Span(std::array<U, N> &arr) noexcept :
		  data_(arr.data()),
		  size_(N)
		{
			static_assert((Extent == DYNAMIC_EXTENT) || (N == Extent),
			              "Array size must match extent [span.cons]");
			static_assert(std::is_convertible<U *, element_type *>::value,
			              "Array element type must be convertible to span element type [span.cons]");
		}

		/// @brief Constructor from const std::array [span.cons]/13-15
		/// @param[in] arr const std::array reference
		/// @tparam U Element type of array
		/// @tparam N Size of array
		/// @constraints extent == DYNAMIC_EXTENT || N == extent
		/// @constraints is_convertible_v<const U(*)[], element_type(*)[]> is true
		/// @postconditions size() == N && data() == arr.data()
		template<typename U, std::size_t N>
		constexpr Span(const std::array<U, N> &arr) noexcept :
		  data_(arr.data()),
		  size_(N)
		{
			static_assert((Extent == DYNAMIC_EXTENT) || (N == Extent),
			              "Array size must match extent [span.cons]");
			static_assert(std::is_convertible<const U *, element_type *>::value,
			              "Array element type must be convertible to span element type [span.cons]");
		}

		/// @brief Constructor from range (R&&) [span.cons]/16-20
		/// @note C++14 LIMITATION: This constructor requires concepts (contiguous_range,
		///       sized_range, borrowed_range) which are only available in C++20 with ranges library.
		///       This C++14 freestanding implementation cannot fully support the range constructor
		///       as specified in [span.cons]/16-20 without external range support.
		///       To use this, either:
		///       - Provide custom range types with constexpr data() and size() members
		///       - Use explicit construction with iterators instead
		///       - Use a C++20 compatible standard library implementation
		///
		/// @constraints [span.cons]/16: This constructor is NOT implemented in C++14 freestanding
		///              because it requires std::ranges support (C++20).
		///
		/// Intended constraints from [span.cons]/16:
		/// - R satisfies ranges::contiguous_range and ranges::sized_range
		/// - Either R satisfies ranges::borrowed_range or is_const_v<element_type> is true
		/// - remove_cvref_t<R> is not a specialization of span
		/// - remove_cvref_t<R> is not a specialization of array
		/// - is_array_v<remove_cvref_t<R>> is false
		/// - is_convertible_v<U(*)[], element_type(*)[]> is true
		///
		/// See [span.cons]/16-20 for full specification.

		/// @brief Constructor from initializer_list [span.cons]/21-23
		/// @param[in] il initializer_list of value_type
		/// @constraints is_const_v<element_type> is true [span.cons]/21
		/// @postconditions size() == il.size() && data() == il.data()
		constexpr explicit Span(std::initializer_list<value_type> il) noexcept :
		  data_(il.begin()),
		  size_(il.size())
		{
			static_assert(std::is_const<element_type>::value,
			              "initializer_list constructor requires const element_type [span.cons]");
		}

		/// @brief Copy constructor [span.cons]/24
		/// @postconditions size() == other.size() && data() == other.data()
		// constexpr Span(const Span &) noexcept = default;

		/// @brief Converting constructor from other span [span.cons]/25-29
		/// @param[in] s Other span to convert from
		/// @tparam OtherElementType Element type of source span
		/// @tparam OtherExtent Extent of source span
		/// @constraints (Extent == DYNAMIC_EXTENT) || (OtherExtent == DYNAMIC_EXTENT) ||
		///              (Extent == OtherExtent) is true [span.cons]/25.1
		/// @constraints is_convertible_v<OtherElementType(*)[], element_type(*)[]> is true
		///              [span.cons]/25.2
		/// @postconditions size() == s.size() && data() == s.data()
		/// @remarks Explicit if extent != dynamic_extent && OtherExtent == dynamic_extent
		///          [span.cons]/29. Per C++14 limitation, always explicit.
		template<typename OtherElementType, std::size_t OtherExtent, typename = typename std::enable_if<((Extent == DYNAMIC_EXTENT) || (OtherExtent == DYNAMIC_EXTENT) || (Extent == OtherExtent)) && std::is_convertible<OtherElementType *, element_type *>::value>::type>
		constexpr explicit Span(const Span<OtherElementType, OtherExtent> &s) noexcept :
		  data_(s.data()),
		  size_(s.size())
		{
		}

		/// @brief Copy assignment [span.cons]/30
		/// @postconditions size() == other.size() && data() == other.data()
		// constexpr Span &operator=(const Span &) noexcept = default;

		// [span.sub] 23.7.2.2.4 Subviews

		/// @brief Get first Count elements as a fixed-size span [span.sub]/1-3
		/// @tparam Count Number of elements to take from the beginning
		/// @mandates Count <= Extent is true [span.sub]/1
		/// @hardened_preconditions Count <= size() is true [span.sub]/2
		/// @returns span containing the first Count elements
		/// @effects Equivalent to: return R(data(), Count); where R is the return type
		/// @note Using static_assert for Mandate checking: per [span.sub]/1, Mandates are
		///       compile-time requirements checked with static_assert, while hardened
		///       preconditions are runtime checks omitted in freestanding.
		template<std::size_t Count>
		constexpr Span<element_type, Count> first() const noexcept
		{
			static_assert(Count <= Extent,
			              "Count must not exceed Extent [span.sub]");
			return Span<element_type, Count>(data_, Count);
		}

		/// @brief Get first count elements as a dynamic-size span [span.sub]/11-12
		/// @param[in] count Number of elements to take from the beginning
		/// @hardened_preconditions count <= size() is true [span.sub]/11
		/// @returns span containing the first count elements
		/// @effects Equivalent to: return R(data(), count); where R is the return type
		constexpr Span<element_type, DYNAMIC_EXTENT> first(size_type count) const noexcept
		{
			return Span<element_type, DYNAMIC_EXTENT>(data_, count);
		}

		/// @brief Get last Count elements as a fixed-size span [span.sub]/4-6
		/// @tparam Count Number of elements to take from the end
		/// @mandates Count <= Extent is true [span.sub]/4
		/// @hardened_preconditions Count <= size() is true [span.sub]/5
		/// @returns span containing the last Count elements
		/// @effects Equivalent to: return R(data() + (size() - Count), Count); where R is return type
		template<std::size_t Count>
		constexpr Span<element_type, Count> last() const noexcept
		{
			static_assert(Count <= Extent,
			              "Count must not exceed Extent [span.sub]");
			return Span<element_type, Count>(data_ + size_ - Count, Count);
		}

		/// @brief Get last count elements as a dynamic-size span [span.sub]/13-14
		/// @param[in] count Number of elements to take from the end
		/// @hardened_preconditions count <= size() is true [span.sub]/13
		/// @returns span containing the last count elements
		/// @effects Equivalent to: return R(data() + (size() - count), count); where R is return type
		constexpr Span<element_type, DYNAMIC_EXTENT> last(size_type count) const noexcept
		{
			return Span<element_type, DYNAMIC_EXTENT>(data_ + size_ - count, count);
		}

		/// @brief Get subspan starting at Offset with Count elements (or to end) [span.sub]/7-10
		/// @tparam Offset Starting offset from beginning
		/// @tparam Count Number of elements to take (default: DYNAMIC_EXTENT for remainder)
		/// @mandates Offset <= Extent && (Count == DYNAMIC_EXTENT || Count <= Extent - Offset)
		///           is true [span.sub]/7
		/// @hardened_preconditions Offset <= size() && (Count == DYNAMIC_EXTENT ||
		///                         Count <= size() - Offset) is true [span.sub]/8
		/// @returns Subspan view with element_type and computed extent per [span.sub]/10
		/// @effects Equivalent to:
		///          return R(data() + Offset, Count != DYNAMIC_EXTENT ? Count : size() - Offset);
		///          where R is the return type
		/// @remarks Returned extent is: Count != DYNAMIC_EXTENT ? Count :
		///          (Extent != DYNAMIC_EXTENT ? Extent - Offset : DYNAMIC_EXTENT)
		template<std::size_t Offset, std::size_t Count = DYNAMIC_EXTENT>
		constexpr Span<element_type, detail::subspan_extent<Extent, Offset, Count>::value>
		subspan() const noexcept
		{
			static_assert(
			  Offset <= Extent && ((Count == DYNAMIC_EXTENT) || (Count <= Extent - Offset)),
			  "Offset and Count must be within Extent [span.sub]");

			return Span<element_type, detail::subspan_extent<Extent, Offset, Count>::value>(
			  data_ + Offset,
			  (Count != DYNAMIC_EXTENT) ? Count : size_ - Offset);
		}

		/// @brief Get dynamic subspan starting at offset with count elements (or to end)
		///        [span.sub]/15-16
		/// @param[in] offset Starting offset from beginning
		/// @param[in] count Number of elements to take (default: DYNAMIC_EXTENT for remainder)
		/// @hardened_preconditions offset <= size() && (count == DYNAMIC_EXTENT ||
		///                         count <= size() - offset) is true [span.sub]/15
		/// @returns Dynamic subspan view
		/// @effects Equivalent to:
		///          return R(data() + offset, count == DYNAMIC_EXTENT ? size() - offset : count);
		///          where R is the return type
		constexpr Span<element_type, DYNAMIC_EXTENT> subspan(
		  size_type offset,
		  size_type count = DYNAMIC_EXTENT) const noexcept
		{
			return Span<element_type, DYNAMIC_EXTENT>(
			  data_ + offset,
			  (count != DYNAMIC_EXTENT) ? count : size_ - offset);
		}

		// [span.obs] 23.7.2.2.5 Observers

		/// @brief Returns the number of elements in the span [span.obs]/1
		/// @returns Number of elements, equivalent to size_
		/// @note Per [span.overview]/2: O(1) complexity
		constexpr size_type size() const noexcept
		{
			return size_;
		}

		/// @brief Returns the size of the span in bytes [span.obs]/2
		/// @returns Size in bytes, equivalent to size() * sizeof(element_type)
		/// @note Per [span.overview]/2: O(1) complexity
		constexpr size_type size_bytes() const noexcept
		{
			return size_ * sizeof(element_type);
		}

		/// @brief Checks if the span is empty [span.obs]/3
		/// @returns true if size() == 0, false otherwise
		/// @note Per [span.overview]/2: O(1) complexity
		constexpr bool empty() const noexcept
		{
			return (0 == size_);
		}

		// [span.elem] 23.7.2.2.6 Element access

		/// @brief Accesses an element by index [span.elem]/1-3
		/// @param[in] idx Index of the element
		/// @hardened_preconditions idx < size() is true [span.elem]/1
		/// @returns Reference to element at index idx, equivalent to *(data() + idx)
		/// @throws Nothing [span.elem]/3
		constexpr reference operator[](size_type idx) const noexcept
		{
			return data_[idx];
		}

		/// @brief Accesses an element by index with bounds checking [span.elem]/4-5
		/// @param[in] idx Index of the element
		/// @returns Reference to element at index idx, equivalent to *(data() + idx)
		/// @throws out_of_range if idx >= size() is true [span.elem]/5
		/// @note This method is marked as freestanding-deleted in the standard.
		///       For freestanding C++14 compatibility, exceptions are not available.
		///       This implementation provides minimal implementation without bounds checking.
		///       Clients should use operator[] with their own bounds checking.
		constexpr reference at(size_type idx) const noexcept
		{
			// Freestanding compatibility: cannot throw exceptions
			// Clients must ensure idx < size() themselves
			return data_[idx];
		}

		/// @brief Returns reference to the first element [span.elem]/6-8
		/// @hardened_preconditions empty() is false [span.elem]/6
		/// @returns Reference to first element, equivalent to *data()
		/// @throws Nothing [span.elem]/8
		constexpr reference front() const noexcept
		{
			return data_[0];
		}

		/// @brief Returns reference to the last element [span.elem]/9-11
		/// @hardened_preconditions empty() is false [span.elem]/9
		/// @returns Reference to last element, equivalent to *(data() + (size() - 1))
		/// @throws Nothing [span.elem]/11
		constexpr reference back() const noexcept
		{
			return data_[size_ - 1];
		}

		/// @brief Returns pointer to the beginning of the span [span.elem]/12
		/// @returns Pointer to the data, equivalent to data_
		/// @note Per [span.overview]/2: O(1) complexity
		constexpr pointer data() const noexcept
		{
			return data_;
		}

		// [span.iterators] 23.7.2.2.7 Iterator support

		/// @brief Returns iterator to the first element [span.iterators]/3
		/// @returns Iterator to the first element, equivalent to data_
		/// @effects If empty() is true, returns same value as end()
		/// @note Per [span.overview]/2: O(1) complexity
		constexpr iterator begin() const noexcept
		{
			return data_;
		}

		/// @brief Returns iterator past the last element [span.iterators]/4
		/// @returns Past-the-end iterator, equivalent to data_ + size_
		/// @note Per [span.overview]/2: O(1) complexity
		constexpr iterator end() const noexcept
		{
			return data_ + size_;
		}

		/// @brief Returns const iterator to the first element
		/// @returns Const iterator to the first element
		/// @effects Equivalent to: return begin()
		constexpr const_iterator cbegin() const noexcept
		{
			return data_;
		}

		/// @brief Returns const iterator past the last element
		/// @returns Const iterator past the last element
		/// @effects Equivalent to: return end()
		constexpr const_iterator cend() const noexcept
		{
			return data_ + size_;
		}

		/// @brief Returns reverse iterator to the first element (from end) [span.iterators]/5
		/// @returns Reverse iterator, equivalent to reverse_iterator(end())
		/// @effects Equivalent to: return reverse_iterator(end())
		constexpr reverse_iterator rbegin() const noexcept
		{
			return reverse_iterator(end());
		}

		/// @brief Returns reverse iterator past the last element (from beginning)
		///        [span.iterators]/6
		/// @returns Reverse iterator, equivalent to reverse_iterator(begin())
		/// @effects Equivalent to: return reverse_iterator(begin())
		constexpr reverse_iterator rend() const noexcept
		{
			return reverse_iterator(begin());
		}

		/// @brief Returns const reverse iterator to the first element (from end)
		/// @returns Const reverse iterator
		/// @effects Equivalent to: return const_reverse_iterator(end())
		constexpr const_reverse_iterator crbegin() const noexcept
		{
			return const_reverse_iterator(end());
		}

		/// @brief Returns const reverse iterator past the last element (from beginning)
		/// @returns Const reverse iterator
		/// @effects Equivalent to: return const_reverse_iterator(begin())
		constexpr const_reverse_iterator crend() const noexcept
		{
			return const_reverse_iterator(begin());
		}

	private:
		/// @brief Pointer to the beginning of the span [span.overview]
		/// Per [span.overview]: exposition only, data from owned storage
		pointer data_;
		/// @brief Number of elements in the span [span.overview]
		/// Per [span.overview]: exposition only, maintained by constructors
		size_type size_;
	};

	// [span.objectrep] 23.7.2.3 Views of object representation

	/// @brief Create a span view of the bytes of a span [span.objectrep]/1-2
	/// @tparam ElementType Element type of the source span
	/// @tparam Extent Extent of the source span
	/// @param[in] s Source span
	/// @constraints is_volatile_v<ElementType> is false [span.objectrep]/1
	/// @returns span view of object bytes
	/// @effects Equivalent to:
	///          return R{reinterpret_cast<const byte*>(s.data()), s.size_bytes()};
	///          where R is the return type
	/// @throws Nothing
	/// @note Per [span.objectrep]/2: allows examining object representation as bytes.
	///       Return extent is: Extent == DYNAMIC_EXTENT ? DYNAMIC_EXTENT
	///                         : sizeof(ElementType) * Extent
	template<typename ElementType, std::size_t Extent>
	constexpr Span<const detail::byte,
	               (Extent == DYNAMIC_EXTENT ? DYNAMIC_EXTENT : sizeof(ElementType) * Extent)>
	as_bytes(Span<ElementType, Extent> s) noexcept
	{
		static_assert(!std::is_volatile<ElementType>::value,
		              "Cannot create byte view of volatile-qualified element type");

		return Span<const detail::byte,
		            (Extent == DYNAMIC_EXTENT ? DYNAMIC_EXTENT : sizeof(ElementType) * Extent)>(
		  reinterpret_cast<const detail::byte *>(s.data()),
		  s.size_bytes());
	}

	/// @brief Create a writable span view of the bytes of a span [span.objectrep]/3-4
	/// @tparam ElementType Element type of the source span
	/// @tparam Extent Extent of the source span
	/// @param[in] s Source span
	/// @constraints is_const_v<ElementType> is false [span.objectrep]/3
	/// @constraints is_volatile_v<ElementType> is false [span.objectrep]/3
	/// @returns Writable span view of object bytes
	/// @effects Equivalent to:
	///          return R{reinterpret_cast<byte*>(s.data()), s.size_bytes()};
	///          where R is the return type
	/// @throws Nothing
	/// @note Per [span.objectrep]/4: allows modifying object representation as bytes.
	///       Return extent is: Extent == DYNAMIC_EXTENT ? DYNAMIC_EXTENT
	///                         : sizeof(ElementType) * Extent
	template<typename ElementType, std::size_t Extent>
	constexpr Span<detail::byte,
	               (Extent == DYNAMIC_EXTENT ? DYNAMIC_EXTENT : sizeof(ElementType) * Extent)>
	as_writable_bytes(Span<ElementType, Extent> s) noexcept
	{
		static_assert(!std::is_const<ElementType>::value && !std::is_volatile<ElementType>::value,
		              "Cannot create writable byte view of const or volatile-qualified element type");

		return Span<detail::byte,
		            (Extent == DYNAMIC_EXTENT ? DYNAMIC_EXTENT : sizeof(ElementType) * Extent)>(
		  reinterpret_cast<detail::byte *>(s.data()),
		  s.size_bytes());
	}

	// Utility functions (C++14 alternatives to deduction guides)

	/// @brief Create a span from pointer and size
	/// @tparam T Element type
	/// @param[in] ptr Pointer to data
	/// @param[in] size Number of elements
	/// @returns Span<T> object with dynamic extent
	/// @postconditions result.data() == ptr && result.size() == size
	/// @note C++14 compatibility: deduction guides not available, using utility function instead
	template<typename T>
	constexpr Span<T> make_span(T *ptr, std::size_t size) noexcept
	{
		return Span<T>(ptr, size);
	}

	/// @brief Create a span from C array
	/// @tparam T Element type
	/// @tparam N Array size
	/// @param[in] arr C-style array reference
	/// @returns Span<T, N> object with fixed extent N
	/// @postconditions result.data() == arr && result.size() == N
	/// @note C++14 compatibility: deduction guides not available, using utility function instead
	template<typename T, std::size_t N>
	constexpr Span<T, N> make_span(T (&arr)[N]) noexcept
	{
		return Span<T, N>(arr);
	}

	/// @brief Create a span from std::array
	/// @tparam T Element type
	/// @tparam N Array size
	/// @param[in] arr std::array reference
	/// @returns Span<T, N> object with fixed extent N
	/// @postconditions result.data() == arr.data() && result.size() == N
	/// @note C++14 compatibility: deduction guides not available, using utility function instead
	template<typename T, std::size_t N>
	constexpr Span<T, N> make_span(std::array<T, N> &arr) noexcept
	{
		return Span<T, N>(arr);
	}

	/// @brief Create a const span from const std::array
	/// @tparam T Element type
	/// @tparam N Array size
	/// @param[in] arr const std::array reference
	/// @returns Span<const T, N> object with fixed extent N and const element type
	/// @postconditions result.data() == arr.data() && result.size() == N
	/// @note C++14 compatibility: deduction guides not available, using utility function instead
	template<typename T, std::size_t N>
	constexpr Span<const T, N> make_span(const std::array<T, N> &arr) noexcept
	{
		return Span<const T, N>(arr);
	}

} // namespace isobus

#endif // SPAN_HPP
