// libwav_test.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "libwav_test.h"
#include <cmath>

Wave w("sound.wav");
//Wave w("16bit_signed_pcm.wav");
//Wave w("24bit_signed_pcm.wav");
//Wave w("32bit_signed_pcm.wav");
HWND g_hWnd;
int scrW=800;
int scrH=600;

struct int24
{
	signed int data : 24;
};

double scaleRange(double in, double oldMin, double oldMax, double newMin, double newMax)
{
	return (in / ((oldMax - oldMin) / (newMax - newMin))) + newMin;
}

VOID OnPaint(HDC hdc)
{
	Graphics graphics(hdc);
	Pen      pen(Color::Red);
	Pen axis(Color::Black);
	graphics.Clear(Color::White);
	graphics.DrawLine(&axis, 0, scrH / 2, scrW, scrH / 2);

	Wave::memblock* mem = w.next(scrW);
	if (mem->nBytes == 0) return;

	int bps = (w.getH()->wBitsPerSample / 8);
	
	byte* u = (byte*)mem->p;

	int px = 0;
	double py;
	
	int newy;
	for (unsigned int i = 0; i < mem->nBytes; i+=w.getH()->nChannels*bps)
	{
		switch (bps)
		{
		case 1:
			newy = (int)(scrH / 2 + scaleRange(*(int8_t*)(u + i), INT8_MIN, INT8_MAX, -scrH/2, scrH/2));
			break;
		case 2:
			newy = (int)(scrH / 2 + scaleRange(*(int16_t*)(u + i), INT16_MIN, INT16_MAX, -scrH / 2, scrH / 2));
			break;
		case 3:
			newy = (int)(scrH / 2 + scaleRange((*(int24*)(u + i)).data, -pow(2, 23), pow(2, 23), -scrH / 2, scrH / 2));
			break;
		case 4:
			newy = (int)(scrH / 2 + scaleRange(*(int32_t*)(u + i), INT32_MIN, INT32_MAX, -scrH / 2, scrH / 2));
			break;
		}
		newy += scrH / 2;
		if (i == 0) py = newy;
		graphics.DrawLine(&pen, px, (int)py, px + 1, newy);
		py = newy;
		px++;
	}
}


#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;


	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR           gdiplusToken;
	// Initialize GDI+.
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_LIBWAV_TEST, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_LIBWAV_TEST));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	GdiplusShutdown(gdiplusToken);
	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LIBWAV_TEST));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_LIBWAV_TEST);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
	   0, 0, scrW, scrH, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);
   g_hWnd = hWnd;
   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
bool isPaused = false;
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	long* _rect = new long[4];

	switch (message)
	{
	case WM_SIZE:
		GetClientRect(hWnd, (LPRECT)_rect);
		scrW = _rect[2] - _rect[0];
		scrH = _rect[3] - _rect[1];
		break;
	case WM_KEYUP:
		if (wParam == 0x50) isPaused = !isPaused;
		break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		if (!isPaused) OnPaint(hdc);
		EndPaint(hWnd, &ps);
		InvalidateRect(hWnd, NULL, FALSE);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
