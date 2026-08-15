#pragma once

#include "Globals.h"
#include "Effects.h"
#include "SkillCheckEffect.h"
#include "MarionetteEffect.h"
#include "AdsEffect.h"

// Обновление описания текущего режима в GUI
void UpdateStatusText();

// Оконная процедура главной панели управления GUI
LRESULT CALLBACK GuiWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
