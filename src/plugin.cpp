#include "log.h"

namespace {
    constexpr std::uint32_t kArvakFormID = 0x00BDD0;
    constexpr const char* kArvakPlugin = "Dawnguard.esm";
    constexpr std::uint32_t kHotkeyDIK = 0x23; // DIK_H

    RE::TESObjectREFR* g_arvakRef = nullptr;

    RE::TESNPC* GetArvakBase() {
        auto* handler = RE::TESDataHandler::GetSingleton();
        if (!handler) return nullptr;
        auto* form = handler->LookupForm(kArvakFormID, kArvakPlugin);
        return form ? form->As<RE::TESNPC>() : nullptr;
    }

    RE::NiPoint3 GetPositionInFrontOfPlayer(RE::PlayerCharacter* player, float distance) {
        float angle = player->GetAngleZ();
        RE::NiPoint3 pos = player->GetPosition();

        pos.x += distance * std::sin(angle);
        pos.y += distance * std::cos(angle);

        return pos;
    }

    void SpawnArvak() {
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player) return;

        if (!g_arvakRef) {
            auto* arvakBase = GetArvakBase();
            if (!arvakBase) {
                SKSE::log::error("Could not find Arvak base form - is Dawnguard.esm loaded?");
                return;
            }

            auto newRef = player->PlaceObjectAtMe(arvakBase, false);
            if (!newRef) {
                SKSE::log::error("Failed to spawn Arvak");
                return;
            }

            g_arvakRef = newRef.get();
        }

        RE::NiPoint3 spawnPos = GetPositionInFrontOfPlayer(player, 150.0f);
        g_arvakRef->SetPosition(spawnPos);
        g_arvakRef->data.angle.z = player->GetAngleZ();

        // MOUNT
    }

    class InputHandler : public RE::BSTEventSink<RE::InputEvent*> {
    public:
        static InputHandler* GetSingleton() {
            static InputHandler singleton;
            return &singleton;
        }

        RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* a_events,
            RE::BSTEventSource<RE::InputEvent*>*) override {
            if (!a_events) return RE::BSEventNotifyControl::kContinue;

            for (auto* e = *a_events; e; e = e->next) {
                if (e->eventType != RE::INPUT_EVENT_TYPE::kButton) continue;

                auto* button = static_cast<RE::ButtonEvent*>(e);
                if (button->IsDown() && button->GetIDCode() == kHotkeyDIK &&
                    button->GetDevice() == RE::INPUT_DEVICE::kKeyboard) {
                    SpawnArvak();
                }
            }
            return RE::BSEventNotifyControl::kContinue;
        }
    };
}

void OnDataLoaded() {}

void MessageHandler(SKSE::MessagingInterface::Message* a_msg) {
    switch (a_msg->type) {
    case SKSE::MessagingInterface::kDataLoaded:
        OnDataLoaded();
        break;
    case SKSE::MessagingInterface::kInputLoaded:
        RE::BSInputDeviceManager::GetSingleton()->AddEventSink(InputHandler::GetSingleton());
        break;
    case SKSE::MessagingInterface::kPostLoad:
        break;
    case SKSE::MessagingInterface::kPreLoadGame:
        break;
    case SKSE::MessagingInterface::kPostLoadGame:
        break;
    case SKSE::MessagingInterface::kNewGame:
        break;
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SKSE::Init(skse);
    SetupLog();

    auto messaging = SKSE::GetMessagingInterface();
    if (!messaging->RegisterListener("SKSE", MessageHandler)) {
        return false;
    }

    return true;
}