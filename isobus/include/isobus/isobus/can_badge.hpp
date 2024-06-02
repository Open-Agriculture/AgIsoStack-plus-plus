//================================================================================================
/// @file can_badge.hpp
///
/// @brief A way to only allow certain object types to access certain functions that is enforced
/// at compile time. A neat trick from Serenity OS :^)
/// @author Adrian Del Grosso
///
/// @copyright 2022 The Open-Agriculture Developers
//================================================================================================

#ifndef CAN_BADGE_HPP
#define CAN_BADGE_HPP

namespace isobus
{
	//================================================================================================
	/// @class CANLibBadge
	///
	/// @brief This is a way to protect functions on public interfaces from being
	/// accessed by objects that shouldn't access them.
	///
	/// @details This class was inspired from SerenityOS. It's a way to avoid
	/// friends. It protects functions on a class's public interface from being
	/// called by types that were not explicitly allowed in the function signature.
	//================================================================================================

	//! @cond DoNotRaiseWarning
	template<typename T>
	class CANLibBadge
	{
	private:
		friend T; ///< Our best friend, T
		//! @brief An empty function for our best friend <T>
		CANLibBadge() = default;
	};
	//! @endcond

} // namespace isobus

#endif // CAN_BADGE_HPP
