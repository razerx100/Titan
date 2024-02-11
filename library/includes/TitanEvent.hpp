#ifndef TITAN_EVENT_HPP_
#define TITAN_EVENT_HPP_

// The intended use would be to define an Enum class for a program,
// then pass it to EnumType and have all of the events of that program inherit from that templated
// version of TitanEvent. That enum should have all of the Event types of that program. And the
// last one should be called None, to use that one to count the the total number of the event types.
template<typename EnumType>
class TitanEvent
{
public:
	virtual ~TitanEvent() = default;

	[[nodiscard]]
	virtual EnumType GetType() const noexcept = 0u;

	// Use this function with your own discretion.
	template<typename DerivedEvent>
	[[nodiscard]]
	const DerivedEvent& CastType() const noexcept
	{
		return *reinterpret_cast<DerivedEvent const*>(this);
	}
};
#endif
