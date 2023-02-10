#pragma once

#include <map>
#include <chrono>
#include <shared_mutex>
#include "Domain/Repositories/EntityRepositoryInterface.h"
#include "../Factories/SkillFactory.h"
#include "../../../Events/SkillCreatedEvent.h"
#include "../../../Events/SkillUsedEvent.h"
#include "../../../Events/SkillCancelledEvent.h"
#include "../../../Events/AbnormalEffectChangedEvent.h"
#include "../../../Events/HeroDeletedEvent.h"
#include "../../../Events/GameEngineTickedEvent.h"
#include "../../../Events/EventDispatcher.h"
#include "../GameStructs/NetworkHandlerWrapper.h"
#include "../../../Common/TimerMap.h"
#include "../../../Services/EntityFinder.h"

using namespace L2Bot::Domain;

namespace Interlude
{
	class SkillRepository : public Repositories::EntityRepositoryInterface
	{
	public:
		const std::vector<std::shared_ptr<DTO::EntityState>> GetEntities() override
		{
			std::unique_lock<std::shared_timed_mutex>(m_Mutex);

			const auto objects = m_EntityFinder.FindEntities<std::shared_ptr<Entities::Skill>>(m_Skills, [this](std::shared_ptr<Entities::Skill> item) {
				return std::make_shared<Entities::Skill>(item.get());
			});

			auto result = std::vector<std::shared_ptr<DTO::EntityState>>();

			for (const auto kvp : objects)
			{
				result.push_back(kvp.second);
			}

			return result;
		}

		SkillRepository(const NetworkHandlerWrapper& networkHandler, const SkillFactory& factory, EntityFinder& finder) :
			m_NetworkHandler(networkHandler),
			m_Factory(factory),
			m_EntityFinder(finder)
		{
			EventDispatcher::GetInstance().Subscribe(SkillCreatedEvent::name, [this](const Event& evt) {
				OnSkillCreated(evt);
			});
			EventDispatcher::GetInstance().Subscribe(SkillUsedEvent::name, [this](const Event& evt) {
				OnSkillUsed(evt);
			});
			EventDispatcher::GetInstance().Subscribe(SkillCancelledEvent::name, [this](const Event& evt) {
				OnSkillCancelled(evt);
			});
			EventDispatcher::GetInstance().Subscribe(AbnormalEffectChangedEvent::name, [this](const Event& evt) {
				OnSkillToggled(evt);
			});
			EventDispatcher::GetInstance().Subscribe(HeroDeletedEvent::name, [this](const Event& evt) {
				OnHeroDeleted(evt);
			});
			EventDispatcher::GetInstance().Subscribe(GameEngineTickedEvent::name, [this](const Event& evt) {
				OnGameEngineTicked(evt);
			});
		}

		SkillRepository() = delete;
		virtual ~SkillRepository()
		{
			Reset();
		}

		void Reset() override
		{
			std::shared_lock<std::shared_timed_mutex>(m_Mutex);
			m_Skills.clear();
			m_IsNewCycle = false;
			m_NewSkills.clear();
		}

		void OnGameEngineTicked(const Event& evt)
		{
			std::shared_lock<std::shared_timed_mutex>(m_Mutex);
			if (evt.GetName() == GameEngineTickedEvent::name)
			{
				for (auto it = m_Skills.begin(); it != m_Skills.end();)
				{
					if (m_NewSkills.find(it->first) == m_NewSkills.end())
					{
						it = m_Skills.erase(it);
					}
					else
					{
						++it;
					}
				}

				m_IsNewCycle = true;
			}
		}

		void OnHeroDeleted(const Event& evt)
		{
			std::shared_lock<std::shared_timed_mutex>(m_Mutex);
			if (evt.GetName() == HeroDeletedEvent::name)
			{
				Reset();
				m_CastingTimers.StopAll();
				m_ReloadingTimers.StopAll();
			}
		}

		//todo need to delete skills if they are not exists in create "queue"
		void OnSkillCreated(const Event& evt)
		{
			std::shared_lock<std::shared_timed_mutex>(m_Mutex);
			if (evt.GetName() == SkillCreatedEvent::name)
			{
				if (m_IsNewCycle)
				{
					m_IsNewCycle = false;
					m_NewSkills.clear();
				}

				const auto casted = static_cast<const SkillCreatedEvent&>(evt);
				const auto skillInfo = casted.GetSkillInfo();
				const auto skillId = skillInfo[2];

				auto skill = m_Factory.Create(
					skillInfo[2],
					skillInfo[1],
					skillInfo[0]
				);
				if (m_Skills.find(skillId) == m_Skills.end())
				{
					m_Skills.emplace(skill->GetId(), skill);
				}
				else
				{
					m_Skills[skillId]->Update(skill.get());
				}
				m_NewSkills[skillId] = skillId;
			}
		}
		void OnSkillUsed(const Event& evt)
		{
			std::shared_lock<std::shared_timed_mutex>(m_Mutex);
			if (evt.GetName() == SkillUsedEvent::name)
			{
				const auto casted = static_cast<const SkillUsedEvent&>(evt);
				const auto skillInfo = casted.GetSkillInfo();
				const auto skillId = skillInfo[0];

				if (m_Skills.find(skillId) == m_Skills.end())
				{
					//todo exception?
					return;
				}

				const auto& skill = m_Skills[skillId];

				skill->UpdateReloadingState(true);
				skill->UpdateCastingState(true);

				m_UsedSkillId = skill->GetId();

				m_ReloadingTimers.StartTimer(skill->GetId(), skillInfo[3], [this] (uint32_t skillId) {
					std::shared_lock<std::shared_timed_mutex>(m_Mutex);
					m_Skills[skillId]->UpdateReloadingState(false);
				});
				m_CastingTimers.StartTimer(skill->GetId(), skillInfo[2], [this] (uint32_t skillId) {
					std::shared_lock<std::shared_timed_mutex>(m_Mutex);
					m_Skills[skillId]->UpdateCastingState(false);
				});
			}
		}
		void OnSkillCancelled(const Event& evt)
		{
			std::shared_lock<std::shared_timed_mutex>(m_Mutex);
			if (evt.GetName() == SkillCancelledEvent::name)
			{
				const auto casted = static_cast<const SkillCancelledEvent&>(evt);

				const auto hero = m_NetworkHandler.GetHero();

				if (hero && hero->objectId == casted.GetInitiatorId())
				{
					if (m_Skills.find(m_UsedSkillId) == m_Skills.end())
					{
						//todo exception?
						return;
					}

					const auto& skill = m_Skills[m_UsedSkillId];

					skill->UpdateCastingState(false);
					m_CastingTimers.StopTimer(skill->GetId());

					m_UsedSkillId = 0;
				}
			}
		}
		void OnSkillToggled(const Event& evt)
		{
			std::shared_lock<std::shared_timed_mutex>(m_Mutex);
			if (evt.GetName() == AbnormalEffectChangedEvent::name)
			{
				const auto casted = static_cast<const AbnormalEffectChangedEvent&>(evt);
				const auto skillInfo = casted.GetSkillInfo();

				std::map<uint32_t, int32_t> ids;

				for (size_t i = 0; i < skillInfo.size(); i += 3)
				{
					ids[skillInfo[i]] = skillInfo[i + 2];
				}

				for (const auto& kvp : m_Skills)
				{
					const auto& skill = kvp.second;

					const auto needToToggle = ids.find(skill->GetId()) != ids.end();
					// buff time less than zero means this is a aura
					const auto isAura = needToToggle ? ids[skill->GetId()] < 0 : false;

					if (skill->IsToggled() && !needToToggle)
					{
						skill->UpdateToggle(false);
					}
					else if (!skill->IsToggled() && needToToggle && isAura)
					{
						skill->UpdateToggle(true);
					}
				}
			}
		}

	private:
		const SkillFactory& m_Factory;
		std::map<uint32_t, std::shared_ptr<Entities::Skill>> m_Skills;
		std::map<uint32_t, uint32_t> m_NewSkills;
		bool m_IsNewCycle = true;
		uint32_t m_UsedSkillId = 0;
		const NetworkHandlerWrapper& m_NetworkHandler;
		TimerMap m_ReloadingTimers;
		TimerMap m_CastingTimers;
		std::shared_timed_mutex m_Mutex;
		EntityFinder& m_EntityFinder;
	};
}