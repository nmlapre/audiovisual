#pragma once

#include "imgui.h"
#include "framework.h"
#include <functional>

// The goal of this translation unit is to package slightly arcane and generally
// annoying-to-deal-with windowing logic into a somewhat-easier-to-deal-with
// form. This incldues DirectX, win32, and ImGui stuff.

bool CreateDeviceD3D(HWND hWnd);

void CleanupDeviceD3D();

void CreateRenderTarget();

void CleanupRenderTarget();

// Initialize imgui rendering for the application session.
// If this function returns false, abort the initialization failed.
bool InitImGuiRendering();

void TearDownWindowRendering();

// Render a single ImGui frame. This function calls the given functor after
// starting a new ImGui frame and before rendering the ImGui draw data. That is
// to say, put the ImGui stuff in that functor.
void RenderFrame(const std::function<void()>& fn);

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
