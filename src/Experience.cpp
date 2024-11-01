#include "Experience.h"

#include "Settings.h"
#include "Translation.h"
#include "Skyrim/HUDMenu.h"

#include "Events/ActorKill.h"
#include "Events/BooksRead.h"
#include "Events/LocationCleared.h"
#include "Events/LocationDiscovery.h"
#include "Events/ObjectiveState.h"
#include "Events/QuestStatus.h"
#include "Events/SkillIncrease.h"

using namespace RE;

ExperienceManager::ExperienceManager()
{
	worker = std::thread(&ExperienceManager::Loop, this);

	auto player = PlayerCharacter::GetSingleton();
	data = player->skills->data;		
}

ExperienceManager::~ExperienceManager()
{
	running.store(false);

	wakeup.notify_one();
	worker.join();
}

void ExperienceManager::Init()
{
	events.emplace_back(new QuestStatusEventHandler(this));
	events.emplace_back(new LocationDiscoveryEventHandler(this));
	events.emplace_back(new LocationClearedEventHandler(this));
	events.emplace_back(new ObjectiveStateEventHandler(this));

	auto settings = Settings::GetSingleton();

	if (settings->GetValue<bool>("bEnableKilling")) {
		events.emplace_back(new ActorKillEventHandler(this));
	}
	if (settings->GetValue<bool>("bEnableReading")) {
		events.emplace_back(new BooksReadEventHandler(this));
	}
	if (settings->GetValue<bool>("bEnableSkillXP")) {
		events.emplace_back(new SkillIncreaseEventHandler(this));
	}

	logger::info("Experience sources initialized");
}

void ExperienceManager::Loop()
{
	std::unique_lock<std::mutex> lock(mtx);

	do {
		wakeup.wait(lock); // mutex unlocked until CV is notified

		if (!queue.empty()) {
			Process(lock);
		}

	} while (running.load());
}

void ExperienceManager::Process(std::unique_lock<std::mutex>& lock)
{
	wakeup.wait_until(lock, timeout, [this] {  // mutex unlocked
		return !running.load();
	});

	int points = 0; 
	int	widget = 0;

	while (!queue.empty()) {
		Experience& e = queue.front();

		points += e.points;
		widget |= e.widget;

		queue.pop();
	}

	AddExperience(points, widget);
}

void ExperienceManager::AddExperience(const Experience& xp)
{
	std::unique_lock<std::mutex> lock(mtx);

	timeout = clock_t::now() + std::chrono::seconds(1);  // reset timer

	queue.push(xp);

	wakeup.notify_one();
}

void ExperienceManager::AddExperience(int points, bool meter)
{
	float xp_old = data->xp;
	float xp_new = xp_old + points;
	float xp_max = data->levelThreshold;

	data->xp = xp_new;

	logger::trace("Progress: {0}/{2} => {1}/{2}", 
		xp_old, xp_new, xp_max);

	if (ShouldDisplayLevelMeter(meter)) {
		ShowLevelMeter(xp_old / xp_max, xp_new / xp_max);
	}
	if (xp_new >= xp_max && xp_old < xp_max) {
		ShowLevelUpNotification();
	}
	if (Settings::GetSingleton()->GetValue<bool>("bShowMessages")) {
		ShowRewardMessage(points);
	}
}

float ExperienceManager::GetExperience()
{
	return data->xp;
}

void ExperienceManager::Source::AddExperience(int points, bool meter)
{
	if (points != 0) {

		Experience exp { .source = this, .points = points, .widget = meter };

		logger::trace("{0:+d} XP", points);

		manager->AddExperience(exp);
	}
}

bool ExperienceManager::ShouldDisplayLevelMeter(bool display)
{
	int mode = Settings::GetSingleton()->GetValue<int>("iMeterMode");
	switch (mode) {
	case MeterMode::kHidden:
		return false;
	case MeterMode::kDynamic:
		return display;
	case MeterMode::kForced:
		return true;
	default:
		return true;
	}
}

void ExperienceManager::ShowLevelMeter(float start, float end)
{
	PlayerCharacter* player = PlayerCharacter::GetSingleton();
	uint16_t level = player->GetLevel();

	HUDMenuEx::ShowLevelMeter(level, start, end);
}

void ExperienceManager::ShowLevelUpNotification()
{
	std::string message = Translation::Translate("$LEVEL UP");

	HUDMenuEx::ShowNotification(message.c_str(), "", "UILevelUp");
}

void ExperienceManager::ShowRewardMessage(int points)
{
	std::string format = Settings::GetSingleton()->GetValue<std::string>("sMessageFormat");
	std::string result = std::vformat(format, std::make_format_args(points));

	DebugNotification(result.c_str());
}
