// fastdraw.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include "fastdraw.h"
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wincodec.h>

#define MAX_LOADSTRING 100

// グローバル変数:
HINSTANCE hInst;								// 現在のインターフェイス
TCHAR szTitle[MAX_LOADSTRING];					// タイトル バーのテキスト
TCHAR szWindowClass[MAX_LOADSTRING];			// メイン ウィンドウ クラス名

ID2D1Factory* factory = NULL;
IDWriteFactory* writeFactory = NULL;
IDWriteTextFormat* textFormat = NULL;
ID2D1HwndRenderTarget* renderTarget = NULL;
ID2D1Bitmap* bitmap = NULL;
ID2D1SolidColorBrush* brush = NULL;
UINT32* memImg = NULL;							// メモリ上の画像データ
UINT32 cnt = 0;
LARGE_INTEGER liFreq, liPrev;

// このコード モジュールに含まれる関数の宣言を転送します:
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

 	// TODO: ここにコードを挿入してください。
	MSG msg;
	HACCEL hAccelTable;

	// グローバル文字列を初期化しています。
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_FASTDRAW, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// アプリケーションの初期化を実行します:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_FASTDRAW));

	// メイン メッセージ ループ:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}


//
//  関数: MyRegisterClass()
//
//  目的: ウィンドウ クラスを登録します。
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
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_FASTDRAW));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_FASTDRAW);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

HRESULT CreateDeviceIndependentResources(){
	HRESULT hr;

	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &factory);
	if (SUCCEEDED(hr)){
		hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&writeFactory));
		if (SUCCEEDED(hr)){
			hr = writeFactory->CreateTextFormat(L"Vernada", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 20, L"", &textFormat);
			if (SUCCEEDED(hr)){
				textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
			}
		}
	}
	return hr;
}

template<class Interface>
inline void SafeRelease(Interface **ppInterfaceToRelease){
    if (*ppInterfaceToRelease != NULL){
        (*ppInterfaceToRelease)->Release();
        (*ppInterfaceToRelease) = NULL;
    }
}

HRESULT CreateDeviceResources(HWND hWnd){
	RECT rc;
	HRESULT hr = S_OK;

	if (!renderTarget){
		GetClientRect(hWnd, &rc);
		D2D1_SIZE_U size = D2D1::SizeU(rc.right-rc.left, rc.bottom-rc.top);
		D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties();
		D2D1_PIXEL_FORMAT pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE);
		props.pixelFormat = pixelFormat;
		hr = factory->CreateHwndRenderTarget(props, D2D1::HwndRenderTargetProperties(hWnd, size), &renderTarget);
		if (SUCCEEDED(hr)){
			D2D1_BITMAP_PROPERTIES bitmapProp;
			bitmapProp.pixelFormat = pixelFormat;
			bitmapProp.dpiX = 96;//props.dpiX; // a value of 0 causes error
			bitmapProp.dpiY = 96;//props.dpiY;
			hr = renderTarget->CreateBitmap(size, NULL, 0, &bitmapProp, &bitmap);
			if (SUCCEEDED(hr)){
				if (memImg){
					free(memImg);
				}
				memImg = (UINT32*)malloc(size.width*size.height*sizeof(UINT32));
				hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White, 1.0f), &brush);
			}
		}
	}
	return hr;
}

void DiscardDeviceResources(){
	SafeRelease(&renderTarget);
	SafeRelease(&bitmap);
	SafeRelease(&brush);
}

//
//   関数: InitInstance(HINSTANCE, int)
//
//   目的: インスタンス ハンドルを保存して、メイン ウィンドウを作成します。
//
//   コメント:
//
//        この関数で、グローバル変数でインスタンス ハンドルを保存し、
//        メイン プログラム ウィンドウを作成および表示します。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // グローバル変数にインスタンス処理を格納します。

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   QueryPerformanceFrequency(&liFreq);

   CreateDeviceIndependentResources();
   CreateDeviceResources(hWnd);

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

void OnResize(UINT width, UINT height){
	if (renderTarget){
		DiscardDeviceResources();
//		renderTarget->Resize(D2D1::SizeU(width, height));
	}
}

void OnRender(HWND hWnd){
	HRESULT hr;
	LARGE_INTEGER liCur;
	wchar_t buf[128];

	QueryPerformanceCounter(&liCur);
	//int len = swprintf_s(buf, sizeof(buf), L"%.5f[ms]", (liCur.QuadPart - liPrev.QuadPart)*1000.0 / liFreq.QuadPart);	// this causes stack corruption. Why?
	int len = swprintf(buf, sizeof(buf), L"%.5f[ms]", (liCur.QuadPart - liPrev.QuadPart)*1000.0 / liFreq.QuadPart);
	
	if (SUCCEEDED(CreateDeviceResources(hWnd))){
		renderTarget->BeginDraw();

		D2D1_SIZE_F sz = renderTarget->GetSize();
		renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
		//renderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

		cnt++;
		for (int j = 0; j < sz.height; j++)
			for (int i = 0; i < sz.width; i++)
				memImg[j*(UINT32)sz.width+i] = (i+cnt)<<16; // ARGB[31:0]

		hr = bitmap->CopyFromMemory(NULL, memImg, sz.width * sizeof(UINT32));
		if (SUCCEEDED(hr)){
			renderTarget->DrawBitmap(bitmap);
		}
		renderTarget->DrawText(buf, len, textFormat, D2D1::RectF(0, 0, sz.width, sz.height), brush);

		if (D2DERR_RECREATE_TARGET == renderTarget->EndDraw()){
			DiscardDeviceResources();
		}
	}

	liPrev = liCur;
}

//
//  関数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的:  メイン ウィンドウのメッセージを処理します。
//
//  WM_COMMAND	- アプリケーション メニューの処理
//  WM_PAINT	- メイン ウィンドウの描画
//  WM_DESTROY	- 中止メッセージを表示して戻る
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// 選択されたメニューの解析:
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
//		hdc = BeginPaint(hWnd, &ps);
		OnRender(hWnd);
//		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_SIZE:
		OnResize(LOWORD(lParam), HIWORD(lParam));
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// バージョン情報ボックスのメッセージ ハンドラーです。
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
