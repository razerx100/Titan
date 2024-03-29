#include <gtest/gtest.h>
#include <TitanDispatcher.hpp>
#include <iostream>

enum class TestEventType
{
	A,
	B,
	C,
	None
};

using TestEventBaseType = TitanEvent<TestEventType>;

class EventA : public TestEventBaseType
{
public:
	TestEventType GetType() const noexcept override
	{ return TestEventType::A; }
};

class EventB : public TestEventBaseType
{
public:
	TestEventType GetType() const noexcept override
	{ return TestEventType::B; }

	[[nodiscard]]
	std::string GetString() const noexcept
	{
		return "Type B\n";
	}
};

using TestFeedbackEvent = FeedbackEvent<TestEventType>;

class EventC : public TestFeedbackEvent
{
public:
	[[nodiscard]]
	TestEventType GetType() const noexcept override
	{
		return TestEventType::C;
	}
};

using TitanTestDispatcher = TitanDispatcher<TestEventType>;

class User
{
public:
	class Dispatchable : public ITitanDispatchable<TestEventType>
	{
		std::string_view m_name;
	public:
		Dispatchable(std::string_view name) : m_name{ std::move(name) } {}

		void ProcessEvent(TestEventBaseType& eventt) override
		{
			if (eventt.GetType() == TestEventType::A)
				std::cout << m_name << " " << static_cast<size_t>(eventt.GetType()) << "\n";
			else if (eventt.GetType() == TestEventType::B)
			{
				const EventB& eventB = eventt.CastType<EventB>();
				std::cout << m_name << " " << eventB.GetString() << "\n";
			}
			else if (eventt.GetType() == TestEventType::C)
			{
				EventC& eventC = eventt.CastType<EventC>();
				// async
				auto waitVar = std::async(std::launch::async,
					[](EventC::Subscription sub)
					{
						std::cout << "Paused Event C's owner, doing stuff.\n";
					}
				, eventC.GetSubscription());
			}
		}
	};

	User(std::string_view name) : m_eventReceiver{ std::make_shared<Dispatchable>( name ) } {}

	void SubscribeToA(TitanTestDispatcher& dispatcher)
	{
		dispatcher.Subscribe(TestEventType::A, m_eventReceiver);
	}
	void SubscribeToB(TitanTestDispatcher& dispatcher)
	{
		dispatcher.Subscribe(TestEventType::B, m_eventReceiver);
	}
	void SubscribeToC(TitanTestDispatcher& dispatcher)
	{
		dispatcher.Subscribe(TestEventType::C, m_eventReceiver);
	}
	void UnsubscribeToA(TitanTestDispatcher& dispatcher)
	{
		dispatcher.Unsubscribe(TestEventType::A, m_eventReceiver);
	}
	void UnsubscribeToB(TitanTestDispatcher& dispatcher)
	{
		dispatcher.Unsubscribe(TestEventType::B, m_eventReceiver);
	}
	void UnsubscribeToC(TitanTestDispatcher& dispatcher)
	{
		dispatcher.Unsubscribe(TestEventType::C, m_eventReceiver);
	}

private:
	std::shared_ptr<Dispatchable> m_eventReceiver;
};

TEST(TitanEventTest, EventSubscribeTest)
{
	TitanTestDispatcher testDispatcher{};

	User user{ "User" };
	user.SubscribeToA(testDispatcher);

	{
		User user1{ "TempUser1" };
		user1.SubscribeToA(testDispatcher);
		user1.SubscribeToB(testDispatcher);
	}

	User user1{ "User1" };
	user1.SubscribeToA(testDispatcher);
	user1.SubscribeToB(testDispatcher);

	EventA eventA{};
	testDispatcher.Dispatch(eventA);

	EventB eventB{};
	testDispatcher.Dispatch(eventB);
}

TEST(TitanEventTest, EventUnsubscribeTest)
{
	TitanTestDispatcher testDispatcher{};

	User user{ "User" };
	user.SubscribeToA(testDispatcher);

	User user1{ "User1" };
	user1.SubscribeToA(testDispatcher);

	EventA eventA{};
	testDispatcher.Dispatch(eventA);

	user1.UnsubscribeToA(testDispatcher);

	testDispatcher.Dispatch(eventA);
}

TEST(TitanEventTest, FeedbackEventTest)
{
	TitanTestDispatcher testDispatcher{};

	User user{ "User" };
	user.SubscribeToC(testDispatcher);

	{
		EventC eventC{};
		std::future<void> waitObj = testDispatcher.DispatchFeedback(eventC);
		waitObj.wait();
	}

	user.UnsubscribeToC(testDispatcher);

	{
		EventC eventC1{};
		std::future<void> waitObj1 = testDispatcher.DispatchFeedback(eventC1);
		waitObj1.wait();
	}
}
