//================================================================================================
/// @file isobus_device_descriptor_object_pool.hpp
///
/// @brief Defines an interface for creating a Task Controller DDOP.
/// @author Adrian Del Grosso
///
/// @copyright 2023 The Open-Agriculture Developers
//================================================================================================

#ifndef ISOBUS_DEVICE_DESCRIPTOR_OBJECT_POOL_HPP
#define ISOBUS_DEVICE_DESCRIPTOR_OBJECT_POOL_HPP

#include "isobus/isobus/can_NAME.hpp"
#include "isobus/isobus/isobus_task_controller_client_objects.hpp"

#include <functional>
#include <memory>

namespace isobus
{
	/// @brief Defines a device descriptor object pool
	/// @details This class can be used to build up a task controller DDOP by adding objects to it
	/// in a hierarchy, then calling generate_binary_object_pool to get the object pool in
	/// binary form.
	/// @note To ensure maximum compatibility with task controllers, it may be best to stick to
	/// limits that were defined for TC 3 and older when providing things like labels for
	/// device element designators.
	class DeviceDescriptorObjectPool
	{
	public:
		/// @brief Default constructor for a DDOP. Sets TC compatibility to version 4.
		DeviceDescriptorObjectPool() = default;

		/// @brief This constructor allows customization of the TC compatibility level
		/// @param[in] taskControllerServerVersion The version of TC server to support with this DDOP
		explicit DeviceDescriptorObjectPool(std::uint8_t taskControllerServerVersion);

		/// @brief Adds a device object to the DDOP
		/// @note There can only be one of these per DDOP
		/// @attention Pay close attention to which values are UTF-8 and which are byte arrays
		/// @param[in] deviceDesignator Descriptive text for the object, UTF-8, 32 chars max
		/// @param[in] deviceSoftwareVersion Software version indicating text
		/// @param[in] deviceSerialNumber Device and manufacturer-specific serial number of the Device (UTF-8)
		/// @param[in] deviceStructureLabel This label allows the device to identify the current version of the device descriptor object pool
		/// @param[in] deviceLocalizationLabel Defined by the language command PGN
		/// @param[in] deviceExtendedStructureLabel Continuation of the Label given by Device to identify the Device descriptor Structure
		/// @param[in] clientIsoNAME NAME of client device as defined in ISO 11783-5
		/// @returns `true` if the object was added to the DDOP, `false` if the object cannot be added (duplicate or some other error)
		bool add_device(std::string deviceDesignator,
		                std::string deviceSoftwareVersion,
		                std::string deviceSerialNumber,
		                std::string deviceStructureLabel,
		                std::array<std::uint8_t, task_controller_object::DeviceObject::MAX_STRUCTURE_AND_LOCALIZATION_LABEL_LENGTH> deviceLocalizationLabel,
		                std::vector<std::uint8_t> deviceExtendedStructureLabel,
		                std::uint64_t clientIsoNAME);

		/// @brief Adds a device element object to the DDOP
		/// @param[in] deviceElementDesignator Descriptive text for the object, UTF-8, 32-128 chars max depending on TC version
		/// @param[in] deviceElementNumber The Element number for process data variable	addressing
		/// @param[in] parentObjectID Object ID of parent DeviceElementObject or DeviceObject in order to establish a hierarchical order of DeviceElements
		/// @param[in] deviceElementType The type of element, such as "device" or "bin"
		/// @param[in] uniqueID The object ID of the object. Must be unique in the DDOP.
		/// @returns `true` if the object was added to the DDOP, `false` if the object cannot be added (duplicate or some other error)
		bool add_device_element(std::string deviceElementDesignator,
		                        std::uint16_t deviceElementNumber,
		                        std::uint16_t parentObjectID,
		                        task_controller_object::DeviceElementObject::Type deviceElementType,
		                        std::uint16_t uniqueID);

		/// @brief Adds a device process data object to the DDOP
		/// @param[in] processDataDesignator Descriptive text for the object, UTF-8, 32-128 chars max
		/// @param[in] processDataDDI Identifier of process data variable (DDI) according to definitions in Annex B and ISO 11783 - 11
		/// @param[in] deviceValuePresentationObjectID Object identifier of a DeviceValuePresentationObject, or the null ID
		/// @param[in] processDataProperties A bitset of properties associated to this object. Some combination of `PropertiesBit`
		/// @param[in] processDataTriggerMethods A bitset of available trigger methods, built from some combination of `AvailableTriggerMethods`
		/// @param[in] uniqueID The object ID of the object. Must be unique in the DDOP.
		/// @returns `true` if the object was added to the DDOP, `false` if the object cannot be added (duplicate or some other error)
		bool add_device_process_data(std::string processDataDesignator,
		                             std::uint16_t processDataDDI,
		                             std::uint16_t deviceValuePresentationObjectID,
		                             std::uint8_t processDataProperties,
		                             std::uint8_t processDataTriggerMethods,
		                             std::uint16_t uniqueID);

		/// @brief Adds a device property object to the DDOP
		/// @param[in] propertyDesignator Descriptive text for the object, UTF-8, 32-128 chars max
		/// @param[in] propertyValue The value of the property
		/// @param[in] propertyDDI Identifier of property (DDI) according to definitions in Annex B and ISO 11783 - 11.
		/// @param[in] valuePresentationObject Object identifier of DeviceValuePresentationObject, or NULL object ID
		/// @param[in] uniqueID The object ID of the object. Must be unique in the DDOP.
		/// @returns `true` if the object was added to the DDOP, `false` if the object cannot be added (duplicate or some other error)
		bool add_device_property(std::string propertyDesignator,
		                         std::int32_t propertyValue,
		                         std::uint16_t propertyDDI,
		                         std::uint16_t valuePresentationObject,
		                         std::uint16_t uniqueID);

		/// @brief Adds a device value presentation object to the DDOP
		/// @param[in] unitDesignator Unit designator for this value presentation
		/// @param[in] offsetValue Offset to be applied to the value for presentation.
		/// @param[in] scaleFactor Scale to be applied to the value for presentation.
		/// @param[in] numberDecimals Specifies the number of decimals to display after the decimal point.
		/// @param[in] uniqueID The object ID of the object. Must be unique in the DDOP.
		/// @returns `true` if the object was added to the DDOP, `false` if the object cannot be added (duplicate or some other error)
		bool add_device_value_presentation(std::string unitDesignator,
		                                   std::int32_t offsetValue,
		                                   float scaleFactor,
		                                   std::uint8_t numberDecimals,
		                                   std::uint16_t uniqueID);

		/// @brief Removes all objects from the DDOP that have a certain type
		/// @param objectType The type of object to remove
		/// @returns `true` if any objects were removed, `false` if no objects were removed
		bool remove_objects_with_type(task_controller_object::ObjectTypes objectType);

		/// @brief Removes all objects from the DDOP that have a certain object ID
		/// @param objectID The object ID to remove
		/// @returns `true` if any objects were removed, `false` if no objects were removed
		bool remove_object_with_id(std::uint16_t objectID);

		/// @brief Removes all objects from the DDOP that match a certain predicate
		/// @param predicate The predicate to match against
		/// @returns `true` if any objects were removed, `false` if no objects were removed
		bool remove_where(std::function<bool(const task_controller_object::Object &)> predicate);

		/// @brief Attempts to take a binary object pool and convert it back into
		/// C++ objects. The object's will be added to the list of objects,
		/// or replaced if an object already exists with the same identifier.
		/// of a DDOP captured in a CAN log, for example.
		/// @param binaryPool The binary object pool, as an array of bytes.
		/// @param clientNAME The ISO NAME of the source ECU for this DDOP, or NAME(0) to ignore checking against actual ECU NAME
		/// @returns True if the object pool was successfully deserialized, otherwise false.
		/// @note This only means that the pool was deserialized. It does not mean that the
		/// relationship between objects is valid. You may have to do additional
		/// checking on the pool before using it.
		bool deserialize_binary_object_pool(std::vector<std::uint8_t> &binaryPool, NAME clientNAME = NAME(0));

		/// @brief Attempts to take a binary object pool and convert it back into
		/// C++ objects. The object's will be added to the list of objects,
		/// or replaced if an object already exists with the same identifier.
		/// Useful for a task controller server or to view the content
		/// of a DDOP captured in a CAN log, for example.
		/// @param binaryPool The binary object pool, as an array of bytes.
		/// @param binaryPoolSizeBytes The size of the DDOP to process in bytes.
		/// @param clientNAME The ISO NAME of the source ECU for this DDOP, or NAME(0) to ignore checking against actual ECU NAME
		/// @returns True if the object pool was successfully deserialized, otherwise false.
		/// @note This only means that the pool was deserialized. It does not mean that the
		/// relationship between objects is valid. You may have to do additional
		/// checking on the pool before using it.
		bool deserialize_binary_object_pool(const std::uint8_t *binaryPool, std::uint32_t binaryPoolSizeBytes, NAME clientNAME = NAME(0));

		/// Constructs a binary DDOP using the objects that were previously added
		/// @param[in,out] resultantPool The binary representation of the DDOP, or an empty vector if this function returns false
		/// @returns `true` if the object pool was generated and is valid, otherwise `false`.
		bool generate_binary_object_pool(std::vector<std::uint8_t> &resultantPool);

		/// Constructs a ISOXML formatted TASKDATA.xml file inside a string using the objects that were previously added.
		/// @param[in,out] resultantString The XML representation of the DDOP, or an empty string if this function returns false
		/// @returns `true` if the object pool was generated and is valid, otherwise `false`.
		bool generate_task_data_iso_xml(std::string &resultantString);

		/// @brief Gets an object from the DDOP that corresponds to a certain object ID
		/// @param[in] objectID The ID of the object to get
		/// @returns Pointer to the object matching the provided ID, or nullptr if no match was found
		std::shared_ptr<task_controller_object::Object> get_object_by_id(std::uint16_t objectID);

		/// @brief Gets an object from the DDOP by index based on object creation
		/// @param[in] index The index of the object to get
		/// @returns Pointer to the object matching the index, or nullptr if no match was found
		std::shared_ptr<task_controller_object::Object> get_object_by_index(std::uint16_t index);

		/// @brief Removes an object from the DDOP using its object ID.
		/// @note This will not fix orphaned parent/child relationships.
		/// Also, if two or more objects were created with the same ID, only one match will be removed.
		/// You should consider the case where 2 objects have the same ID undefined behavior.
		/// @param[in] objectID The ID of the object to remove
		/// @returns true if the object with the specified ID was removed, otherwise false
		bool remove_object_by_id(std::uint16_t objectID);

		/// @brief Sets the TC version to use when generating a binary DDOP.
		/// @note If you do not call this, TC version 4 is used by default
		/// @param[in] tcVersion The version of TC you are targeting for this DDOP
		void set_task_controller_compatibility_level(std::uint8_t tcVersion);

		/// @brief Returns the current TC version used when generating a binary DDOP.
		/// @returns the current TC version used when generating a binary DDOP.
		std::uint8_t get_task_controller_compatibility_level() const;

		/// @brief Returns The maximum TC version supported by the CAN stack's DDOP generator
		/// @returns The maximum TC version supported by the CAN stack's DDOP generator
		static std::uint8_t get_max_supported_task_controller_version();

		/// @brief Clears the DDOP back to an empty state
		void clear();

		/// @brief Returns the number of objects in the DDOP.
		/// @note The number of objects in the DDOP is limited to 65535.
		/// @returns The number of objects in the DDOP
		std::uint16_t size() const;

	private:
		/// @brief Checks to see that all parent object IDs correspond to an object in this DDOP
		/// @returns `true` if all object IDs were validated, otherwise `false`
		bool resolve_parent_ids_to_objects();

		/// @brief Checks the DDOP to see if an object ID has already been used
		/// @param[in] uniqueID The ID to check against in the DDOP for uniqueness
		/// @returns true if the object ID parameter is unique in the DDOP, otherwise false
		bool check_object_id_unique(std::uint16_t uniqueID) const;

		static constexpr std::uint8_t MAX_TC_VERSION_SUPPORTED = 4; ///< The max TC version a DDOP object can support as of today

		std::vector<std::shared_ptr<task_controller_object::Object>> objectList; ///< Maintains a list of all added objects
		std::uint8_t taskControllerCompatibilityLevel = MAX_TC_VERSION_SUPPORTED; ///< Stores the max TC version
	};
} // namespace isobus

#endif // ISOBUS_DEVICE_DESCRIPTOR_OBJECT_POOL_HPP
