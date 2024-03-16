#ifndef TITAN_EVENT_HPP_
#define TITAN_EVENT_HPP_
#include <atomic>
#include <future>

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
	DerivedEvent& CastType() noexcept
	{
		return *reinterpret_cast<DerivedEvent*>(this);
	}
};

template<typename EnumType>
class TitanDispatcher;

template<typename EnumType>
class FeedbackEvent : public TitanEvent<EnumType>
{
	friend class TitanDispatcher<EnumType>;
public:
	class Subscription
	{
		friend class FeedbackEvent<EnumType>;

		Subscription(FeedbackEvent<EnumType>& feedbackEvent) : m_feedbackEvent{ feedbackEvent }
		{
			m_feedbackEvent.AddSubscriber();
		}

	public:
		~Subscription()
		{
			m_feedbackEvent.RemoveSubscriber();
		}

	private:
		FeedbackEvent<EnumType>& m_feedbackEvent;
	};

public:
	FeedbackEvent() : m_syncObj{ 0u }, m_eventPromise{} {}

	// The Subscribed callback functions must increase the value of the syncObj
	// and then decrease it again when they are finished. The broadcasting thread
	// will wait until the value is 0.
	void AddSubscriber()
	{
		m_syncObj.fetch_add(1u);
	}
	void RemoveSubscriber()
	{
		m_syncObj.fetch_sub(1u);

		SetPromiseIfNoSubscribers();
	}

	[[nodiscard]]
	Subscription GetSubscription() { return Subscription{ *this }; }

protected:
	[[nodiscard]]
	std::future<void> GetEventFuture() { return m_eventPromise.get_future(); }

	void SetPromiseIfNoSubscribers()
	{
		if (m_syncObj == 0u)
			m_eventPromise.set_value();
	}

protected:
	std::atomic_uint32_t m_syncObj;
	std::promise<void>   m_eventPromise;
};
#endif
