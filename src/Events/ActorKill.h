#pragma once

#include "Experience.h"

namespace fs = std::filesystem;

class ActorKillEventHandler : public ExperienceManager::Source, 
	public RE::BSTEventSink<RE::TESDeathEvent>
{
public:
	using ActorKillEventSource = RE::BSTEventSource<RE::ActorKill::Event>;

	ActorKillEventHandler(ExperienceManager* manager);
	~ActorKillEventHandler();

	RE::BSEventNotifyControl ProcessEvent(const RE::TESDeathEvent* event, RE::BSTEventSource<RE::TESDeathEvent>* source) override;

	void HandleKill(RE::Actor* victim, RE::Actor* killer);

	static RE::TESNPC* GetTemplateBase(RE::Actor* actor);

	float GetBaseReward(RE::Actor* victim);
	static float GetLevelMult(const RE::Actor* victim, const RE::Actor* killer);
	static float GetGroupMult(const RE::PlayerCharacter* player);

	static bool IsValidKill(RE::Actor* victim, RE::Actor* killer);

private:
	void ParseDirectory(const fs::path& root, std::unordered_map<RE::TESForm*, int>& data);
	void ParseFile(const fs::path& file, std::unordered_map<RE::TESForm*, int>& data);

	std::unordered_map<RE::TESForm*, int> races;
	std::unordered_map<RE::TESForm*, int> npcs;
};
