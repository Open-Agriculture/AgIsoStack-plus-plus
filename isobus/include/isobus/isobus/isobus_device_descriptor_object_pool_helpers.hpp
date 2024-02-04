//================================================================================================
/// @file isobus_device_descriptor_object_pool_helpers.hpp
///
/// @brief Defines helpers for getting commonly needed information out of a DDOP.
/// These are provided so that you don't have to do quite as much manual parsing of
/// the DDOP.
/// @author Adrian Del Grosso
///
/// @copyright 2024 The Open-Agriculture Developers
//================================================================================================

#ifndef ISOBUS_DEVICE_DESCRIPTOR_OBJECT_POOL_HELPERS_HPP
#define ISOBUS_DEVICE_DESCRIPTOR_OBJECT_POOL_HELPERS_HPP

#include "isobus/isobus/isobus_device_descriptor_object_pool.hpp"
#include "isobus_standard_data_description_indices.hpp"

namespace isobus
{
	/// @brief Helper object for parsing DDOPs
	/// @attention Getting this data from the DDOP requires traversing the
	/// entire DDOP several times, so you should treat these as O(n^2) and try
	/// not to call them too many times.
	class DeviceDescriptorObjectPoolHelper
	{
	public:
		/// @brief A wrapper for a DDOP value which tells you if the value
		/// was actually supplied by the DDOP.
		class ObjectPoolValue
		{
		public:
			/// @brief Default constructor for ObjectPoolValue which
			/// defaults the value to being non-existant and not settable
			ObjectPoolValue() = default;

			/// @brief overloads the bool operator so that you can check for
			/// this value's existence by doing if(thisObject)
			/// @returns true if the value was in the DDOP or has been manually set, otherwise false
			explicit operator bool() const;

			/// @brief Returns if this variable exists.
			/// A variable exists if it was either provided in the DDOP, or has been set manually
			/// as part of a DPD value command.
			/// @returns true if the value has ever been set, otherwise false
			bool exists() const;

			/// @brief Returns if this value is editable. DPDs are editable. DPTs are not.
			/// @returns true if the value can be set/edited
			bool editable() const;

			/// @brief Returns the value. If the value doesn't exist this will return 0.
			/// @returns The value if it exists, otherwise 0.
			std::int32_t get() const;

		private:
			friend class DeviceDescriptorObjectPoolHelper; ///< Allow our helper to change the values

			std::int32_t value = 0; ///< The value being wrapped by this object
			bool isValuePresent = false; ///< Stores if the value has ever been set
			bool isSettable = false; ///< Stores if the value can be set, such as on a DPD's value
		};

		/// @brief A helper class that describes an individual section of a boom.
		/// This is used to describe the sections of a boom. Units are defined in mm as specified
		/// in the ISO 11783-10 standard. X offsets are fore/aft. Y offsets are left/right again as
		/// defined in the ISO 11783-10 standard.
		class Section
		{
		public:
			/// @brief Default constructor for a helper class that describes an individual section of a boom
			Section();

			ObjectPoolValue xOffset_mm; ///< The x offset of the section in mm. X offsets are fore+/aft-.
			ObjectPoolValue yOffset_mm; ///< The y offset of the section in mm. Y offsets are left-/right+.
			ObjectPoolValue zOffset_mm; ///< The z offset of the section in mm. Z offsets are up+/down-.
			ObjectPoolValue width_mm; ///< The width of the section in mm.
		};

		/// @brief A helper class that describes a sub boom (not all devices support this)
		class SubBoom
		{
		public:
			/// @brief Default constructor for a helper class that describes a sub boom
			SubBoom();

			std::vector<Section> sections; ///< The sections of the sub boom
			ObjectPoolValue xOffset_mm; ///< The x offset of the sub boom in mm. X offsets are fore+/aft-.
			ObjectPoolValue yOffset_mm; ///< The y offset of the sub boom in mm. Y offsets are left-/right+.
			ObjectPoolValue zOffset_mm; ///< The z offset of the sub boom in mm. Z offsets are up+/down-.
			ObjectPoolValue width_mm; ///< The width of the sub boom in mm
		};

		/// @brief A helper class that describes a boom
		/// This is used to describe a boom, or more generally, an ISO11783-10 function element.
		class Boom
		{
		public:
			std::vector<Section> sections; ///< The sections of the boom
			std::vector<SubBoom> subBooms; ///< The sub booms of the boom
			ObjectPoolValue xOffset_mm; ///< The x offset of the sub boom in mm. X offsets are fore+/aft-.
			ObjectPoolValue yOffset_mm; ///< The y offset of the sub boom in mm. Y offsets are left-/right+.
			ObjectPoolValue zOffset_mm; ///< The z offset of the sub boom in mm. Z offsets are up+/down-.
		};

		/// @brief A helper class that describes an implement based on its DDOP.
		class Implement
		{
		public:
			std::vector<Boom> booms; ///< The booms of the implement
		};

		/// @brief Get the implement description from the DDOP
		/// @param[in] ddop The DDOP to get the implement geometry and info from
		/// @returns The implement geometry and info
		static Implement get_implement_geometry(DeviceDescriptorObjectPool &ddop);

	private:
		/// @brief Parse an element of the DDOP
		/// @param[in] ddop The DDOP to get the implement geometry and info from
		/// @param[in] elementObject The object to parse
		/// @param[out] implementToPopulate The implement to populate with the parsed data
		static void parse_element(DeviceDescriptorObjectPool &ddop,
		                          std::shared_ptr<task_controller_object::DeviceElementObject> elementObject,
		                          Implement &implementToPopulate);

		/// @brief Parse a section element of the DDOP
		/// @param[in] ddop The DDOP to get the implement geometry and info from
		/// @param[in] elementObject The element to parse
		/// @returns The parsed section, or a default section if the elementObject is invalid
		static Section parse_section(DeviceDescriptorObjectPool &ddop,
		                             std::shared_ptr<task_controller_object::DeviceElementObject> elementObject);

		/// @brief Parse a sub boom element of the DDOP
		/// @param[in] ddop The DDOP to get the implement geometry and info from
		/// @param[in] elementObject The element to parse
		/// @returns The parsed sub boom, or a default sub boom if the elementObject is invalid
		static SubBoom parse_sub_boom(DeviceDescriptorObjectPool &ddop,
		                              std::shared_ptr<task_controller_object::DeviceElementObject> elementObject);

		/// @brief Sets the value and presence based on a DDI match.
		/// @param[in,out] objectPoolValue The object pool value to set.
		/// @param[in] property The device property object.
		/// @param[in] ddi The DDI to check against.
		static void setValueFromProperty(ObjectPoolValue &objectPoolValue,
		                                 const std::shared_ptr<task_controller_object::DevicePropertyObject> &property,
		                                 DataDescriptionIndex ddi);

		/// @brief Sets the settable flag based on a DDI match for process data.
		/// @param[in,out] objectPoolValue The object pool value to update.
		/// @param[in] processData The device process data object.
		/// @param[in] ddi The DDI to check against.
		static void setEditableFromProcessData(ObjectPoolValue &objectPoolValue,
		                                       const std::shared_ptr<task_controller_object::DeviceProcessDataObject> &processData,
		                                       DataDescriptionIndex ddi);
	};
} // namespace isobus

#endif // ISOBUS_DEVICE_DESCRIPTOR_OBJECT_POOL_HELPERS_HPP
