#pragma once

#include "Globals.h"
#include "Utils.h"

enum EffectCategory {
    EFFECT_CAT_SOUND = 0,
    EFFECT_CAT_SCREEN = 1,
    EFFECT_CAT_OTHER = 2
};

// Функция мгновенной отмены всех эффектов
void CancelAllEffects();

// --- Звуковые эффекты ---
void PlayLongSound(HINSTANCE hInstance, std::mt19937& gen);
void PlayRandomScreamerSoundOnly(HINSTANCE hInstance, std::mt19937& gen);
void PlayFlashbangSoundOnly(HINSTANCE hInstance, std::mt19937& gen);

// --- Экранные / Визуальные эффекты ---
void InvertScreenOverlay(HINSTANCE hInstance, int durationMs = 12000);
void GlitchScreenOverlay(HINSTANCE hInstance, const std::wstring& overlayText, int durationMs = 12000);
void BlackStripeOverlay(HINSTANCE hInstance, int durationMs = 10000);
void ZoomScreen(float magFactor = 2.0f, int durationMs = 8000);
void Flashbang(HINSTANCE hInstance, int durationMs, std::mt19937& gen);
void ShowScreamerWindow(HINSTANCE hInstance, std::mt19937& gen);

// --- Остальные / Системные эффекты ---
void ShakeWindow(HWND hwnd, int durationMs = 2000);
void SetHugeCursorForDuration(HINSTANCE hInstance, int durationMs = 7000);
void PIZDA(HINSTANCE hInstance);

// --- Новые приколы ---
void LowQualityScreenEffect(int durationMs = 10000);
void EpilepticMouseEffect(int durationMs = 2500);
void ShowKeyboardCaptcha(HINSTANCE hInstance, std::mt19937& gen);
void ShowBSODOverlay(HINSTANCE hInstance, int durationMs = 5000);

// --- Вызов случайных эффектов по категориям ---
void TriggerRandomSoundEffect(HINSTANCE hInstance, std::mt19937& gen);
void TriggerRandomScreenEffect(HINSTANCE hInstance, std::mt19937& gen);
void TriggerRandomOtherEffect(HINSTANCE hInstance, std::mt19937& gen);

// Тройное наказание (запуск сразу 3 случайных эффектов — по одному из каждой категории ОДНОВРЕМЕННО)
void TriggerThreeCategoryPunishment(HINSTANCE hInstance, std::mt19937& gen);

