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

using TitanTestDispatcher = TitanDispatcher<TestEventType>;

class User
{
public:
	class Dispatchable : public ITitanDispatchable<TestEventType>
	{
	public:
		void ProcessEvent(const TestEventBaseType& eventt) override
		{
			if (eventt.GetType() == TestEventType::A)
				std::cout << static_cast<size_t>(eventt.GetType()) << "\n";
			else if (eventt.GetType() == TestEventType::B)
			{
				const EventB& eventB = eventt.CastType<EventB>();
				std::cout << eventB.GetString() << "\n";
			}
		}
	};

	User() : m_eventReceiver{ std::make_shared<Dispatchable>() } {}

	void SubscribeToA(TitanTestDispatcher& dispatcher)
	{
		dispatcher.Subscribe(TestEventType::A, m_eventReceiver);
	}
	void SubscribeToB(TitanTestDispatcher& dispatcher)
	{
		dispatcher.Subscribe(TestEventType::B, m_eventReceiver);
	}

private:
	std::shared_ptr<Dispatchable> m_eventReceiver;
};

TEST(TitanEventTest, EventTest)
{
	TitanTestDispatcher testDispatcher{};

	User user{};
	user.SubscribeToA(testDispatcher);

	{
		User user1{};
		user1.SubscribeToA(testDispatcher);
		user1.SubscribeToB(testDispatcher);
	}

	User user1{};
	user1.SubscribeToA(testDispatcher);
	user1.SubscribeToB(testDispatcher);

	EventA eventA{};
	testDispatcher.Dispatch(eventA);

	EventB eventB{};
	testDispatcher.Dispatch(eventB);
}
