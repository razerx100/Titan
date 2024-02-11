#ifndef TITAN_DISPATCHER_HPP_
#define TITAN_DISPATCHER_HPP_
#include <array>
#include <vector>
#include <functional>
#include <memory>
#include <algorithm>
#include <TitanEvent.hpp>

template<typename EnumType>
class ITitanDispatchable
{
public:
	virtual ~ITitanDispatchable() = default;

	virtual void ProcessEvent(const TitanEvent<EnumType>& event) = 0u;
};

// The intended use would be to define an Enum class for a program,
// then pass it to EnumType and have all of the events of that program inherit from that templated
// version of TitanEvent. That enum should have all of the Event types of that program. And the
// last one should be called None, to use that one to count the the total number of the event types.
template<typename EnumType>
class TitanDispatcher
{
public:
	using BaseEvent    = TitanEvent<EnumType>;
	using CallbackType = std::shared_ptr<ITitanDispatchable<EnumType>>;

private:
	using SubscribedCallbacks = std::vector<CallbackType>;
	using SubscribersType     = std::array<SubscribedCallbacks, static_cast<size_t>(EnumType::None)>;

public:
	// The registered callback should be threadsafe.
	void Subscribe(EnumType type, CallbackType callback) noexcept
	{
		GetSubscribers(type).emplace_back(std::move(callback));
	}

	void Dispatch(const BaseEvent& eventObj)
	{
		EnumType eventType               = eventObj.GetType();
		SubscribedCallbacks& subscribers = GetSubscribers(eventType);

		std::erase_if(subscribers, [&eventObj](const CallbackType& dispatchable)
			{
				if (dispatchable.use_count() == 1u)
					return true;
				else
				{
					dispatchable->ProcessEvent(eventObj);
					return false;
				}
			});
	}

private:
	[[nodiscard]]
	SubscribedCallbacks& GetSubscribers(EnumType type) noexcept
	{
		return m_subscribers.at(static_cast<size_t>(type));
	}

private:
	SubscribersType m_subscribers;
};
#endif
