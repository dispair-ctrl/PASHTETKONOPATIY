#include "Effects.h"
#include <thread>
#include <chrono>

void PlayLongSound(HINSTANCE hInstance, std::mt19937& gen) {
    if (LONG_SOUNDS.empty()) return;
    std::uniform_int_distribution<size_t> sndDist(0, LONG_SOUNDS.size() - 1);
    WORD soundId = LONG_SOUNDS[sndDist(gen)];

    PlaySoundW(MAKEINTRESOURCE(soundId), hInstance, SND_RESOURCE | SND_ASYNC);
    
    for (int i = 0; i < 30; ++i) {
        if (g_cancelEffects.load()) {
            PlaySoundW(NULL, NULL, 0);
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void PlayRandomScreamerSoundOnly(HINSTANCE hInstance, std::mt19937& gen) {
    if (SCREAMER_SOUNDS.empty()) return;
    std::uniform_int_distribution<size_t> sndDist(0, SCREAMER_SOUNDS.size() - 1);
    WORD soundId = SCREAMER_SOUNDS[sndDist(gen)];

    PlaySoundW(MAKEINTRESOURCE(soundId), hInstance, SND_RESOURCE | SND_ASYNC);

    for (int i = 0; i < 25; ++i) {
        if (g_cancelEffects.load()) {
            PlaySoundW(NULL, NULL, 0);
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void PlayFlashbangSoundOnly(HINSTANCE hInstance, std::mt19937& gen) {
    if (FLASHBANG_SOUNDS.empty()) return;
    std::uniform_int_distribution<size_t> sndDist(0, FLASHBANG_SOUNDS.size() - 1);
    WORD soundId = FLASHBANG_SOUNDS[sndDist(gen)];

    PlaySoundW(MAKEINTRESOURCE(soundId), hInstance, SND_RESOURCE | SND_ASYNC);

    for (int i = 0; i < 20; ++i) {
        if (g_cancelEffects.load()) {
            PlaySoundW(NULL, NULL, 0);
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void TriggerRandomSoundEffect(HINSTANCE hInstance, std::mt19937& gen) {
    std::uniform_int_distribution<int> dist(0, 2);
    int choice = dist(gen);
    switch (choice) {
    case 0:
        PlayLongSound(hInstance, gen);
        break;
    case 1:
        PlayRandomScreamerSoundOnly(hInstance, gen);
        break;
    case 2:
        PlayFlashbangSoundOnly(hInstance, gen);
        break;
    }
}
