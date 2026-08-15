#pragma once

#include "Globals.h"
#include <unknwn.h>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

// Инициализация/Завершение GDI+
void InitGDIPlus();
void ShutdownGDIPlus();

// Получение путей к рабочим столам пользователя (включая C:\Users\<user>\Desktop и OneDrive)
std::vector<std::wstring> GetAllDesktopFolderPaths();
std::wstring GetDesktopFolderPath();

// Загрузка изображения (PNG/JPG/BMP) в HBITMAP через GDI+
HBITMAP LoadImageToHBITMAP(const std::wstring& filePath);
HBITMAP LoadImageFromResource(HINSTANCE hInstance, WORD resId);

// Вспомогательная функция загрузки скримера
HBITMAP LoadScreamerImage(HINSTANCE hInstance, WORD resId);

// Процедура оверлейных окон
LRESULT CALLBACK OverlayProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Создание полноэкранного оверлейного окна
HWND CreateOverlayWindow(HINSTANCE hInstance);
