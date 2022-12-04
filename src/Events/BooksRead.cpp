#include "Events/BooksRead.h"

#include "Settings.h"

#include <math.h> // round, sqrt
#include <bitset>

using namespace RE;

BooksReadEventHandler::BooksReadEventHandler(ExperienceManager* manager) :
	ExperienceManager::Source(manager, MeterState::kInactive)
{
	BooksRead::GetEventSource()->AddEventSink(this);
}

BooksReadEventHandler::~BooksReadEventHandler(void)
{
	BooksRead::GetEventSource()->RemoveEventSink(this);
}

// kType_Skill flag doesn't seem to be used, using event->skillBook instead
BSEventNotifyControl BooksReadEventHandler::ProcessEvent(const BooksRead::Event* event, BooksReadEventSource*)
{
	TESObjectBOOK* book = event->book;
	if (!event->unk08 && !book->TeachesSpell()) // skillBook
	{
		logger::info("BookRead: {0} ({1})", book->GetName(), book->value);

		auto reward = GetReward(book->value);
		AddExperience(reward);
		
	}
	return BSEventNotifyControl::kContinue;
}

int BooksReadEventHandler::GetReward(float value)
{
	float mult = Settings::GetSingleton().GetSettingFloat("fXPReadingMult");
	float sqrt = std::sqrtf(value * mult);

	return std::round(sqrt);
}
