#ifndef TITAN_DISPATCHER_HPP_
#define TITAN_DISPATCHER_HPP_
#include <array>
#include <vector>
#include <functional>
#include <memory>
#include <ranges>
#include <algorithm>
#include <mutex>
#include <TitanEvent.hpp>

template<typename EnumType>
class ITitanDispatchable
{
public:
	virtual ~ITitanDispatchable() = default;

	virtual void ProcessEvent(TitanEvent<EnumType>& event) = 0u;
};

// The intended use would be to define an Enum class for a program,
// then pass it to EnumType and have all of the events of that program inherit from that templated
// version of TitanEvent. That enum should have all of the Event types of that program. And the
// last one should be called None, to use that one to count the the total number of the event types.
template<typename EnumType>
class TitanDispatcher
{
public:
	using BaseEvent         = TitanEvent<EnumType>;
	using BaseFeedbackEvent = FeedbackEvent<EnumType>;
	using CallbackRefType   = std::weak_ptr<ITitanDispatchable<EnumType>>;

private:
	using SubscribedCallbacks = std::vector<CallbackRefType>;
	using SubscribersType     = std::array<SubscribedCallbacks, static_cast<size_t>(EnumType::None)>;

public:
	TitanDispatcher() : m_subscribers{}, m_subscriberMutex{} {}

	// The registered callback should be threadsafe.
	void Subscribe(EnumType type, CallbackRefType callback) noexcept
	{
		std::lock_guard lock{ m_subscriberMutex };

		SubscribedCallbacks& subscribedCallbacks = GetSubscribers(type);

		EraseExpiredCallbacks(subscribedCallbacks);

		auto result = std::ranges::find_if(subscribedCallbacks,
			[&callback](const CallbackRefType& other)
			{
				return callback.lock() == other.lock();
			}
		);

		// If there is a result, it can be a nullptr or we already have a callback of that type.
		// So, only add the callback if it doesn't exist.
		if (result == std::end(subscribedCallbacks))
			if (!callback.expired())
				subscribedCallbacks.emplace_back(std::move(callback));
	}

	void Unsubscribe(EnumType type, CallbackRefType callback) noexcept
	{
		std::lock_guard lock{ m_subscriberMutex };

		SubscribedCallbacks& subscribedCallbacks = GetSubscribers(type);

		// I am using find_if here instead of erase_if because there shouldn't be any duplicates,
		// so I won't need to look anymore once I have found a result and erase_if will search the
		// whole container even if a match is found.
		auto result = std::ranges::find_if(subscribedCallbacks,
			[&callback](const CallbackRefType& other)
			{
				return callback.lock() == other.lock();
			}
		);

		subscribedCallbacks.erase(result);
	}

	// Subscribe to all types of events.
	void Subscribe(CallbackRefType callback) noexcept
	{
		const auto typeCount = static_cast<size_t>(EnumType::None);
		for (size_t typeIndex = 0u; typeIndex < typeCount; ++typeIndex)
			Subscribe(static_cast<EnumType>(typeIndex), callback);
	}

	// Unsubscribe from all types of events.
	void Unsubscribe(CallbackRefType callback) noexcept
	{
		const auto typeCount = static_cast<size_t>(EnumType::None);
		for (size_t typeIndex = 0u; typeIndex < typeCount; ++typeIndex)
			Unsubscribe(static_cast<EnumType>(typeIndex), callback);
	}

	void Dispatch(BaseEvent& eventObj)
	{
		EnumType eventType = eventObj.GetType();

		{
			// Only locking this part, since the callback might open Pandora's Box. But it will
			// be the consuming function's responsibility to make those threadsafe.
			std::lock_guard lock{ m_subscriberMutex };
			SubscribedCallbacks& subscribers = GetSubscribers(eventType);

			EraseExpiredCallbacks(subscribers);
		}

		const SubscribedCallbacks& subscribers = GetSubscribers(eventType);

		for (const CallbackRefType& callback : subscribers)
			// I am checking if the generated shared_ptr is null instead of if the weak_ptr has expired
			// or not because there is a slight window for the weak_ptr to expire after we have done the
			// checking and before the shared_ptr is created, if the owning pointer is destroyed in a
			// different thread. But in this way, we might send a useless event but at least it the
			// program wouldn't crash because of nullptr dereferencing. The chance of this happening is
			// extremely low but why take risks?
			if (auto owningCallback = callback.lock(); owningCallback)
				owningCallback->ProcessEvent(eventObj);
	}
	void Dispatch(BaseEvent&& eventObj)
	{
		Dispatch(eventObj);
	}

	// Don't dispatch the same event twice. Or the returned future will be invalid.
	[[nodiscard]]
	std::future<void> DispatchFeedback(BaseFeedbackEvent& eventObj)
	{
		Dispatch(eventObj);

		eventObj.SetPromiseIfNoSubscribers();

		return eventObj.GetEventFuture();
	}

private:
	void EraseExpiredCallbacks(SubscribedCallbacks& callbacks) noexcept
	{
		std::erase_if(callbacks, [](const CallbackRefType& callback)
			{
				return callback.expired();
			}
		);
	}

	[[nodiscard]]
	SubscribedCallbacks& GetSubscribers(EnumType eventType) noexcept
	{
		return m_subscribers.at(static_cast<size_t>(eventType));
	}

private:
	SubscribersType m_subscribers;
	std::mutex      m_subscriberMutex;

public:
	TitanDispatcher(const TitanDispatcher&) = delete;
	TitanDispatcher& operator=(const TitanDispatcher&) = delete;

	// We can't really move an atomic, but since it's a constructor, creating a new mutex should
	// be fine. But might be a problem during assignment, so not defining that one.
	TitanDispatcher(TitanDispatcher&& other) noexcept
		: m_subscribers{ other.m_subscribers }, m_subscriberMutex{}
	{}
};
#endif
