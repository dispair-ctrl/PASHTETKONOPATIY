#pragma once

#include "Globals.h"
#include "Utils.h"

// Показ случайного рекламного банера из папки "reklama" на рабочем столе
void ShowAdsPopup(HINSTANCE hInstance, std::mt19937& gen);
