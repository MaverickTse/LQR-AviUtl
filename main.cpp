#include <mathimf.h>
#include <Windows.h>
#include "filter.h"
#include "glib.h"
#include "lqr.h"
#if _DEBUG
#include <cilk\cilk_stub.h>
#else
#include <cilk\cilk.h>
#endif

//---------------------------------------------------------------------
//		フィルタ構造体定義
//---------------------------------------------------------------------
#define	TRACK_N	5														//	トラックバーの数
TCHAR	*track_name[] = { "W%", "H%", "Step", "Rigidity", "Grad" };	//	トラックバーの名前
int		track_default[] = { 1000, 1000, 1, 0, 0 };	//	トラックバーの初期値
int		track_s[] = { 100, 100, 1, 0, 0 };	//	トラックバーの下限値
int		track_e[] = { 1000, 1000, 10, 1000, 5 };	//	トラックバーの上限値

#define	CHECK_N	2														//	チェックボックスの数
TCHAR	*check_name[] = { "Swap R-B channels", "HELP" };				//	チェックボックスの名前
int		check_default[] = { 0, -1 };				//	チェックボックスの初期値 (値は0か1)

FILTER_DLL filter = {
	FILTER_FLAG_EX_INFORMATION,	//	フィルタのフラグ
	//	FILTER_FLAG_ALWAYS_ACTIVE		: フィルタを常にアクティブにします
	//	FILTER_FLAG_CONFIG_POPUP		: 設定をポップアップメニューにします
	//	FILTER_FLAG_CONFIG_CHECK		: 設定をチェックボックスメニューにします
	//	FILTER_FLAG_CONFIG_RADIO		: 設定をラジオボタンメニューにします
	//	FILTER_FLAG_EX_DATA				: 拡張データを保存出来るようにします。
	//	FILTER_FLAG_PRIORITY_HIGHEST	: フィルタのプライオリティを常に最上位にします
	//	FILTER_FLAG_PRIORITY_LOWEST		: フィルタのプライオリティを常に最下位にします
	//	FILTER_FLAG_WINDOW_THICKFRAME	: サイズ変更可能なウィンドウを作ります
	//	FILTER_FLAG_WINDOW_SIZE			: 設定ウィンドウのサイズを指定出来るようにします
	//	FILTER_FLAG_DISP_FILTER			: 表示フィルタにします
	//	FILTER_FLAG_EX_INFORMATION		: フィルタの拡張情報を設定できるようにします
	//	FILTER_FLAG_NO_CONFIG			: 設定ウィンドウを表示しないようにします
	//	FILTER_FLAG_AUDIO_FILTER		: オーディオフィルタにします
	//	FILTER_FLAG_RADIO_BUTTON		: チェックボックスをラジオボタンにします
	//	FILTER_FLAG_WINDOW_HSCROLL		: 水平スクロールバーを持つウィンドウを作ります
	//	FILTER_FLAG_WINDOW_VSCROLL		: 垂直スクロールバーを持つウィンドウを作ります
	//	FILTER_FLAG_IMPORT				: インポートメニューを作ります
	//	FILTER_FLAG_EXPORT				: エクスポートメニューを作ります
	0, 0,						//	設定ウインドウのサイズ (FILTER_FLAG_WINDOW_SIZEが立っている時に有効)
	"Liquid Rescale",			//	フィルタの名前
	TRACK_N,					//	トラックバーの数 (0なら名前初期値等もNULLでよい)
	track_name,					//	トラックバーの名前郡へのポインタ
	track_default,				//	トラックバーの初期値郡へのポインタ
	track_s, track_e,			//	トラックバーの数値の下限上限 (NULLなら全て0～256)
	CHECK_N,					//	チェックボックスの数 (0なら名前初期値等もNULLでよい)
	check_name,					//	チェックボックスの名前郡へのポインタ
	check_default,				//	チェックボックスの初期値郡へのポインタ
	func_proc,					//	フィルタ処理関数へのポインタ (NULLなら呼ばれません)
	NULL,						//	開始時に呼ばれる関数へのポインタ (NULLなら呼ばれません)
	NULL,						//	終了時に呼ばれる関数へのポインタ (NULLなら呼ばれません)
	NULL,						//	設定が変更されたときに呼ばれる関数へのポインタ (NULLなら呼ばれません)
	func_WndProc,						//	設定ウィンドウにウィンドウメッセージが来た時に呼ばれる関数へのポインタ (NULLなら呼ばれません)
	NULL, NULL,					//	システムで使いますので使用しないでください
	NULL,						//  拡張データ領域へのポインタ (FILTER_FLAG_EX_DATAが立っている時に有効)
	NULL,						//  拡張データサイズ (FILTER_FLAG_EX_DATAが立っている時に有効)
	"Liquid rescale v1.0 by MT",
	//  フィルタ情報へのポインタ (FILTER_FLAG_EX_INFORMATIONが立っている時に有効)
	NULL,						//	セーブが開始される直前に呼ばれる関数へのポインタ (NULLなら呼ばれません)
	NULL,						//	セーブが終了した直前に呼ばれる関数へのポインタ (NULLなら呼ばれません)
};


//---------------------------------------------------------------------
//		フィルタ構造体のポインタを渡す関数
//---------------------------------------------------------------------
EXTERN_C FILTER_DLL __declspec(dllexport) * __stdcall GetFilterTable(void)
{
	return &filter;
}
//	下記のようにすると1つのaufファイルで複数のフィルタ構造体を渡すことが出来ます
/*
FILTER_DLL *filter_list[] = {&filter,&filter2,NULL};
EXTERN_C FILTER_DLL __declspec(dllexport) ** __stdcall GetFilterTableList( void )
{
return (FILTER_DLL **)&filter_list;
}
*/


//---------------------------------------------------------------------
//		フィルタ処理関数
//---------------------------------------------------------------------
BOOL func_proc(FILTER *fp, FILTER_PROC_INFO *fpip)
{
	if ((fp->track[0] == 100) && (fp->track[1] == 100)) return FALSE;
	guchar* rgbBuffer = NULL;
	rgbBuffer = g_try_new(guchar, fpip->w*fpip->h * 3);
	if (rgbBuffer == NULL) return FALSE;
	// ->BGR
	_Cilk_for(int line = 0; line < fpip->h; ++line)
	{
		fp->exfunc->yc2rgb((PIXEL*)(rgbBuffer + line*fpip->w * 3), fpip->ycp_edit + line*fpip->max_w, fpip->w);
	}
	//
	if (fp->check[0])
	{
		PIXEL* pSrc = (PIXEL*)rgbBuffer;
		int elem = fpip->w*fpip->h;
		for (int i = 0; i < elem; ++i, ++pSrc)
		{
			unsigned char temp = 0;
			temp = pSrc->b;
			pSrc->b = pSrc->r;
			pSrc->r = temp;
		}
	}
	//
	LqrEnergyFuncBuiltinType energyType;
	if (fp->track[4] == 0)
	{
		energyType = LqrEnergyFuncBuiltinType::LQR_EF_GRAD_XABS;
	}
	else if (fp->track[4] == 1)
	{
		energyType = LqrEnergyFuncBuiltinType::LQR_EF_GRAD_SUMABS;
	}
	else if (fp->track[4] == 2)
	{
		energyType = LqrEnergyFuncBuiltinType::LQR_EF_GRAD_NORM;
	}
	else if (fp->track[4] == 3)
	{
		energyType = LqrEnergyFuncBuiltinType::LQR_EF_LUMA_GRAD_XABS;
	}
	else if (fp->track[4] == 4)
	{
		energyType = LqrEnergyFuncBuiltinType::LQR_EF_LUMA_GRAD_SUMABS;
	}
	else if (fp->track[4] == 5)
	{
		energyType = LqrEnergyFuncBuiltinType::LQR_EF_LUMA_GRAD_NORM;
	}
	else
	{
		energyType = LqrEnergyFuncBuiltinType::LQR_EF_GRAD_XABS;
	}
	LqrCarver *carver = lqr_carver_new(rgbBuffer, fpip->w, fpip->h, 3);
	lqr_carver_set_energy_function_builtin(carver, energyType);
	lqr_carver_init(carver, fp->track[2], (float)fp->track[3] / 1000.0);
	int new_width = round((float)fp->track[0] * (float)fpip->w / 1000.0);
	int new_height = round((float)fp->track[1] * (float)fpip->h / 1000.0);
	lqr_carver_resize(carver, new_width, new_height);
	//
	gint x, y;
	guchar *oRGB;
	PIXEL* oBuffer = g_try_new(PIXEL, new_width*new_height);
	while (lqr_carver_scan(carver, &x, &y, &oRGB))
	{
		PIXEL* pDst = oBuffer + y*new_width + x;
		pDst->b = (!fp->check[0])? oRGB[0] : oRGB[2];
		pDst->g = oRGB[1];
		pDst->r = (!fp->check[0]) ? oRGB[2] : oRGB[0];
	}
	//
	_Cilk_for (int line = 0; line < new_height; ++line)
	{
		fp->exfunc->rgb2yc(fpip->ycp_edit + line* fpip->max_w, oBuffer + line*new_width, new_width);
	}
	//
	fpip->w = new_width;
	fpip->h = new_height;
	//
	g_free(oBuffer);
	lqr_carver_destroy(carver);
	return TRUE;
}

BOOL func_WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, void *editp, FILTER *fp)
{
	if (message == WM_COMMAND)
	{
		if (wparam == MID_FILTER_BUTTON + 1)
		{
			MessageBox(NULL,
				"=Liquid Rescale=\n"
				"-----------------------\n"
				"A non-uniform rescaler that aims to change aspect ratio\n"
				"while causing less deformation on prominent objects.\n"
				"This plugin can only shrink image.\n\n"
				"Parameters\n"
				"-----------------------\n"
				"W% and H%: setting the target width and height as a percentage\n"
				"Step: Times of transversal for seam generation\n"
				"Rigidity: A higher value cause less deformation\n"
				"Grad: Method for gradient calculation\n\n"
				"INFO\n"
				"------------------------\n"
				"LQR Plugin for AviUtl by Maverick Tse(2015 feb)\n"
				"http://mavericktse.mooo.com/wordpress/\n"
				"Using libLqR\n"
				"http://liblqr.wikidot.com/\n"
				, "INFO", MB_OK | MB_ICONINFORMATION);
			return FALSE;
		}
	}
	return FALSE;
}
