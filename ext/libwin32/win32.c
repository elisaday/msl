/*
    Matrix Script Language
    author: Zeb
    e-mail: zebbey@gmail.com
    create: 2013-6-5 12:53
    Copyright (c) 2006-2013 Zeb.  All rights reserved.
*/
#include <stdio.h>
#include <assert.h>
#include <windows.h>
#include "matrix.h"

#define CHECK_RESULT(ret)\
	do {\
		if ((ret) != 0) {\
			goto exit0;\
		}\
	} while (0);

#define CHECK_CONDITION(cond)\
	do {\
		if (!(cond)) {\
			ret = -1;\
			goto exit0;\
		}\
	} while (0);

LRESULT CALLBACK win32_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;

		case WM_KEYDOWN:
		case WM_KEYUP:
			return 1;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
}

static int win32_CreateWindow(matrix_t mat) {
	int ret;
	int32_t x;
	int32_t y;
	int32_t width;
	int32_t height;
	const char* title;
	HWND hWnd;
	uint32_t style;
	HINSTANCE inst;
	RECT rc;
	WNDCLASS wc;

	mat_obj_t o_x = MAT_get_stack_obj(mat, 1);
	mat_obj_t o_y = MAT_get_stack_obj(mat, 2);
	mat_obj_t o_width = MAT_get_stack_obj(mat, 3);
	mat_obj_t o_height = MAT_get_stack_obj(mat, 4);
	mat_obj_t o_title = MAT_get_stack_obj(mat, 5);

	ret = MAT_to_int32(mat, o_x, &x);
	CHECK_RESULT(ret);
	ret = MAT_to_int32(mat, o_y, &y);
	CHECK_RESULT(ret);
	ret = MAT_to_int32(mat, o_width, &width);
	CHECK_RESULT(ret);
	ret = MAT_to_int32(mat, o_height, &height);
	CHECK_RESULT(ret);
	title = MAT_to_str(mat, o_title);
	CHECK_CONDITION(title);

	style = WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX;
	SetRect(&rc, x, y, x + width, y + height);
	AdjustWindowRect(&rc, style, FALSE);
	inst = (HINSTANCE)GetModuleHandle(NULL);

	wc.style = CS_DBLCLKS;
	wc.lpfnWndProc = win32_WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = inst;
	wc.hIcon = NULL;
	wc.hCursor = NULL;
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = title;

	ret = RegisterClass(&wc);
	CHECK_CONDITION(ret != 0);

	hWnd = CreateWindowA(title, title, style, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, inst, 0);
	CHECK_CONDITION(hWnd);

	ShowWindow(hWnd, SW_NORMAL);
	MAT_push_int(mat, (int)hWnd);
	ret = 0;
exit0:
	return ret;
}

static win32_ProcessMessage(matrix_t mat) {
	MSG msg;

	if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
		if (msg.message == WM_QUIT)
			return MAT_push_none(mat);

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return MAT_push_int(mat, 1);
}

static int win32_GetDC(matrix_t mat) {
	int ret;
	HWND hWnd;
	mat_obj_t o = MAT_get_stack_obj(mat, 1);
	ret = MAT_to_int32(mat, o, (int*)&hWnd);
	CHECK_RESULT(ret);

	MAT_push_int(mat, (int)GetDC(hWnd));
	ret = 0;
exit0:
	return ret;
}

static int win32_CreateCompatibleDC(matrix_t mat) {
	int ret;
	HDC hdc;
	mat_obj_t o = MAT_get_stack_obj(mat, 1);
	ret = MAT_to_int32(mat, o, (int*)&hdc);
	CHECK_RESULT(ret);

	MAT_push_int(mat, (int)CreateCompatibleDC(hdc));
	ret = 0;
exit0:
	return ret;
}

static int win32_DeleteDC(matrix_t mat) {
	int ret;
	HDC hdc;
	mat_obj_t o = MAT_get_stack_obj(mat, 1);
	ret = MAT_to_int32(mat, o, (int*)&hdc);
	CHECK_RESULT(ret);

	DeleteDC(hdc);
	MAT_push_none(mat);
	ret = 0;
exit0:
	return ret;
}

static int win32_CreateCompatibleBitmap(matrix_t mat) {
	int ret;
	HBITMAP hbitmap;
	HDC hdc;
	int width, height;
	mat_obj_t o_hdc = MAT_get_stack_obj(mat, 1);
	mat_obj_t o_w = MAT_get_stack_obj(mat, 2);
	mat_obj_t o_h = MAT_get_stack_obj(mat, 3);

	ret = MAT_to_int32(mat, o_hdc, (int*)&hdc);
	CHECK_RESULT(ret);
	ret = MAT_to_int32(mat, o_w, &width);
	CHECK_RESULT(ret);
	ret = MAT_to_int32(mat, o_h, &height);
	CHECK_RESULT(ret);
	hbitmap = CreateCompatibleBitmap(hdc, width, height);
	MAT_push_int(mat, (int)hbitmap);
	ret = 0;
exit0:
	return ret;
}

static int win32_DeleteObject(matrix_t mat) {
	int ret;
	HGDIOBJ obj;
	mat_obj_t o = MAT_get_stack_obj(mat, 1);
	ret = MAT_to_int32(mat, o, (int*)&obj);
	CHECK_RESULT(ret);

	DeleteObject(obj);
	MAT_push_none(mat);
	ret = 0;
exit0:
	return ret;
}

static int win32_SelectObject(matrix_t mat) {
	int ret;
	HDC hdc;
	HGDIOBJ obj;
	mat_obj_t o_hdc = MAT_get_stack_obj(mat, 1);
	mat_obj_t o = MAT_get_stack_obj(mat, 2);
	ret = MAT_to_int32(mat, o_hdc, (int*)&hdc);
	CHECK_RESULT(ret);
	ret = MAT_to_int32(mat, o, (int*)&obj);
	CHECK_RESULT(ret);

	MAT_push_int(mat, (int)SelectObject(hdc, obj));
	ret = 0;
exit0:
	return ret;
}

static int win32_BitBlt(matrix_t mat) {
	int ret;
	HDC dsthdc, srchdc;
	int x, y, width, height, xsrc, ysrc;
	mat_obj_t o_dst_hdc = MAT_get_stack_obj(mat, 1);
	mat_obj_t o_x = MAT_get_stack_obj(mat, 2);
	mat_obj_t o_y = MAT_get_stack_obj(mat, 3);
	mat_obj_t o_w = MAT_get_stack_obj(mat, 4);
	mat_obj_t o_h = MAT_get_stack_obj(mat, 5);
	mat_obj_t o_src_hdc = MAT_get_stack_obj(mat, 6);
	mat_obj_t o_src_x = MAT_get_stack_obj(mat, 7);
	mat_obj_t o_src_y = MAT_get_stack_obj(mat, 8);

	ret = MAT_to_int32(mat, o_dst_hdc, (int*)&dsthdc);
	CHECK_RESULT(ret);
	ret = MAT_to_int32(mat, o_x, (int*)&x);
	CHECK_RESULT(ret);
	ret = MAT_to_int32(mat, o_y, (int*)&y);
	CHECK_RESULT(ret);
	ret = MAT_to_int32(mat, o_w, (int*)&width);
	CHECK_RESULT(ret);
	ret = MAT_to_int32(mat, o_h, (int*)&height);
	CHECK_RESULT(ret);
	ret = MAT_to_int32(mat, o_src_hdc, (int*)&srchdc);
	CHECK_RESULT(ret);
	ret = MAT_to_int32(mat, o_src_x, (int*)&xsrc);
	CHECK_RESULT(ret);
	ret = MAT_to_int32(mat, o_src_y, (int*)&ysrc);
	CHECK_RESULT(ret);

	MAT_push_int(mat, (int)BitBlt(dsthdc, x, y, width, height, srchdc, xsrc, ysrc, SRCCOPY));
	ret = 0;
exit0:
	return ret;
}

static int win32_CreateFont(matrix_t mat) {
	int ret;
	const char* face;
	int size, weight;

	mat_obj_t o_face = MAT_get_stack_obj(mat, 1);
	mat_obj_t o_size = MAT_get_stack_obj(mat, 2);
	mat_obj_t o_weight = MAT_get_stack_obj(mat, 3);

	face = MAT_to_str(mat, o_face);
	CHECK_CONDITION(face);
	ret = MAT_to_int32(mat, o_size, (int*)&size);
	CHECK_RESULT(ret);
	ret = MAT_to_int32(mat, o_weight, (int*)&weight);
	CHECK_RESULT(ret);

	MAT_push_int(mat, (int)CreateFont(
	                 size, 0, 0, 0, weight, 0, 0, 0,
	                 GB2312_CHARSET, OUT_DEFAULT_PRECIS,
	                 CLIP_DEFAULT_PRECIS,
	                 ANTIALIASED_QUALITY, DEFAULT_PITCH, face));

	ret = 0;
exit0:
	return ret;
}

static int win32_TextOut(matrix_t mat) {
	int ret;
	HDC hdc;
	int x, y;
	const char* txt;

	mat_obj_t o_hdc = MAT_get_stack_obj(mat, 1);
	mat_obj_t o_x = MAT_get_stack_obj(mat, 2);
	mat_obj_t o_y = MAT_get_stack_obj(mat, 3);
	mat_obj_t o_txt = MAT_get_stack_obj(mat, 4);

	ret = MAT_to_int32(mat, o_hdc, (int*)&hdc);
	CHECK_RESULT(ret);
	ret = MAT_to_int32(mat, o_x, (int*)&x);
	CHECK_RESULT(ret);
	ret = MAT_to_int32(mat, o_y, (int*)&y);
	CHECK_RESULT(ret);
	txt = MAT_to_str(mat, o_txt);
	CHECK_CONDITION(txt);

	MAT_push_int(mat, (int)TextOut(hdc, x, y, txt, strlen(txt)));
	ret = 0;
exit0:
	return ret;
}

static int win32_SetTextColor(matrix_t mat) {
	int ret;
	HDC hdc;
	int r, g, b;

	mat_obj_t o_hdc = MAT_get_stack_obj(mat, 1);
	mat_obj_t o_r = MAT_get_stack_obj(mat, 2);
	mat_obj_t o_g = MAT_get_stack_obj(mat, 3);
	mat_obj_t o_b = MAT_get_stack_obj(mat, 4);

	ret = MAT_to_int32(mat, o_hdc, (int*)&hdc);
	CHECK_RESULT(ret);
	ret = MAT_to_int32(mat, o_r, (int*)&r);
	CHECK_RESULT(ret);
	ret = MAT_to_int32(mat, o_g, (int*)&g);
	CHECK_RESULT(ret);
	ret = MAT_to_int32(mat, o_b, (int*)&b);
	CHECK_RESULT(ret);

	MAT_push_int(mat, SetTextColor(hdc, RGB(r, g, b)));
	ret = 0;
exit0:
	return ret;
}

static int win32_SetBkMode(matrix_t mat) {
	int ret;
	HDC hdc;
	int mode;

	mat_obj_t o_hdc = MAT_get_stack_obj(mat, 1);
	mat_obj_t o_mode = MAT_get_stack_obj(mat, 2);

	ret = MAT_to_int32(mat, o_hdc, (int*)&hdc);
	CHECK_RESULT(ret);
	ret = MAT_to_int32(mat, o_mode, (int*)&mode);
	CHECK_RESULT(ret);

	if (!mode)
		mode = OPAQUE;
	else
		mode = TRANSPARENT;

	MAT_push_int(mat, (int)SetBkMode(hdc, mode));
	ret = 0;
exit0:
	return ret;
}

static int win32_CreateSolidBrush(matrix_t mat) {
	int ret;
	int r, g, b;

	mat_obj_t o_r = MAT_get_stack_obj(mat, 1);
	mat_obj_t o_g = MAT_get_stack_obj(mat, 2);
	mat_obj_t o_b = MAT_get_stack_obj(mat, 3);

	ret = MAT_to_int32(mat, o_r, (int*)&r);
	CHECK_RESULT(ret);
	ret = MAT_to_int32(mat, o_g, (int*)&g);
	CHECK_RESULT(ret);
	ret = MAT_to_int32(mat, o_b, (int*)&b);
	CHECK_RESULT(ret);

	MAT_push_int(mat, (int)CreateSolidBrush(RGB(r, g, b)));
	ret = 0;
exit0:
	return ret;
}

static int win32_FillRect(matrix_t mat) {
	int ret;
	HDC hdc;
	RECT rc;
	HBRUSH brush;

	mat_obj_t o_hdc = MAT_get_stack_obj(mat, 1);
	mat_obj_t o_l = MAT_get_stack_obj(mat, 2);
	mat_obj_t o_t = MAT_get_stack_obj(mat, 3);
	mat_obj_t o_r = MAT_get_stack_obj(mat, 4);
	mat_obj_t o_b = MAT_get_stack_obj(mat, 5);
	mat_obj_t o_brush = MAT_get_stack_obj(mat, 6);

	ret = MAT_to_int32(mat, o_hdc, (int*)&hdc);
	CHECK_RESULT(ret);
	ret = MAT_to_int32(mat, o_l, (int*)&rc.left);
	CHECK_RESULT(ret);
	ret = MAT_to_int32(mat, o_t, (int*)&rc.top);
	CHECK_RESULT(ret);
	ret = MAT_to_int32(mat, o_r, (int*)&rc.right);
	CHECK_RESULT(ret);
	ret = MAT_to_int32(mat, o_b, (int*)&rc.bottom);
	CHECK_RESULT(ret);
	ret = MAT_to_int32(mat, o_brush, (int*)&brush);

	MAT_push_int(mat, (int)FillRect(hdc, &rc, brush));
	ret = 0;
exit0:
	return ret;
}

static int win32_MessageBox(matrix_t mat) {
	int ret;
	HWND hWnd;
	const char* txt;
	const char* caption;
	uint32_t type;

	mat_obj_t o_hWnd = MAT_get_stack_obj(mat, 1);
	mat_obj_t o_txt = MAT_get_stack_obj(mat, 2);
	mat_obj_t o_caption = MAT_get_stack_obj(mat, 3);
	mat_obj_t o_type = MAT_get_stack_obj(mat, 4);

	ret = MAT_to_int32(mat, o_hWnd, (int*)&hWnd);
	CHECK_RESULT(ret);
	txt = MAT_to_str(mat, o_txt);
	CHECK_CONDITION(txt);
	caption = MAT_to_str(mat, o_caption);
	CHECK_CONDITION(caption);
	ret = MAT_to_int32(mat, o_type, (int*)&type);
	CHECK_RESULT(ret);

	MAT_push_int(mat, (int)MessageBox(hWnd, txt, caption, type));
	ret = 0;
exit0:
	return ret;
}

static int win32_PostMessage(matrix_t mat) {
	int ret;
	HWND hWnd;
	uint32_t msg;
	uint32_t wparam;
	uint32_t lparam;

	mat_obj_t o_hWnd = MAT_get_stack_obj(mat, 1);
	mat_obj_t o_msg = MAT_get_stack_obj(mat, 2);
	mat_obj_t o_wparam = MAT_get_stack_obj(mat, 3);
	mat_obj_t o_lparam = MAT_get_stack_obj(mat, 4);

	ret = MAT_to_int32(mat, o_hWnd, (int*)&hWnd);
	CHECK_RESULT(ret);
	ret = MAT_to_int32(mat, o_msg, (int*)&msg);
	CHECK_RESULT(ret);
	ret = MAT_to_int32(mat, o_wparam, (int*)&wparam);
	CHECK_RESULT(ret);
	ret = MAT_to_int32(mat, o_lparam, (int*)&lparam);
	CHECK_RESULT(ret);

	MAT_push_int(mat, (int)PostMessage(hWnd, msg, wparam, lparam));
	ret = 0;
exit0:
	return ret;
}

static int win32_GetAsyncKeyState(matrix_t mat) {
	int ret;
	int k;
	mat_obj_t o = MAT_get_stack_obj(mat, 1);
	ret = MAT_to_int32(mat, o, &k);
	CHECK_RESULT(ret);

	MAT_push_int(mat, (int)GetAsyncKeyState(k));
	ret = 0;
exit0:
	return ret;
}

MATRIX_API_EXPORT int import(matrix_t mat, mat_mod_t mod) {
	MAT_reg_func(mat, mod, "CreateWindow", win32_CreateWindow);
	MAT_reg_func(mat, mod, "ProcessMessage", win32_ProcessMessage);
	MAT_reg_func(mat, mod, "GetDC", win32_GetDC);
	MAT_reg_func(mat, mod, "CreateCompatibleDC", win32_CreateCompatibleDC);
	MAT_reg_func(mat, mod, "DeleteDC", win32_DeleteDC);
	MAT_reg_func(mat, mod, "CreateCompatibleBitmap", win32_CreateCompatibleBitmap);
	MAT_reg_func(mat, mod, "DeleteObject", win32_DeleteObject);
	MAT_reg_func(mat, mod, "SelectObject", win32_SelectObject);
	MAT_reg_func(mat, mod, "BitBlt", win32_BitBlt);
	MAT_reg_func(mat, mod, "CreateFont", win32_CreateFont);
	MAT_reg_func(mat, mod, "TextOut", win32_TextOut);
	MAT_reg_func(mat, mod, "SetTextColor", win32_SetTextColor);
	MAT_reg_func(mat, mod, "SetBkMode", win32_SetBkMode);
	MAT_reg_func(mat, mod, "CreateSolidBrush", win32_CreateSolidBrush);
	MAT_reg_func(mat, mod, "FillRect", win32_FillRect);
	MAT_reg_func(mat, mod, "MessageBox", win32_MessageBox);
	MAT_reg_func(mat, mod, "PostMessage", win32_PostMessage);
	MAT_reg_func(mat, mod, "GetAsyncKeyState", win32_GetAsyncKeyState);

	MAT_reg_int(mat, mod, "MB_YESNO", MB_YESNO);
	MAT_reg_int(mat, mod, "IDYES", IDYES);
	MAT_reg_int(mat, mod, "IDNO", IDNO);
	MAT_reg_int(mat, mod, "VK_LEFT", VK_LEFT);
	MAT_reg_int(mat, mod, "VK_RIGHT", VK_RIGHT);
	MAT_reg_int(mat, mod, "VK_UP", VK_UP);
	MAT_reg_int(mat, mod, "VK_DOWN", VK_DOWN);
	MAT_reg_int(mat, mod, "VK_SPACE", VK_SPACE);
	MAT_reg_int(mat, mod, "WM_DESTROY", WM_DESTROY);

	return 0;
}

