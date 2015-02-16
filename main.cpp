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
//		�t�B���^�\���̒�`
//---------------------------------------------------------------------
#define	TRACK_N	5														//	�g���b�N�o�[�̐�
TCHAR	*track_name[] = { "W%", "H%", "Step", "Rigidity", "Grad" };	//	�g���b�N�o�[�̖��O
int		track_default[] = { 1000, 1000, 1, 0, 0 };	//	�g���b�N�o�[�̏����l
int		track_s[] = { 100, 100, 1, 0, 0 };	//	�g���b�N�o�[�̉����l
int		track_e[] = { 1000, 1000, 10, 1000, 5 };	//	�g���b�N�o�[�̏���l

#define	CHECK_N	2														//	�`�F�b�N�{�b�N�X�̐�
TCHAR	*check_name[] = { "Swap R-B channels", "HELP" };				//	�`�F�b�N�{�b�N�X�̖��O
int		check_default[] = { 0, -1 };				//	�`�F�b�N�{�b�N�X�̏����l (�l��0��1)

FILTER_DLL filter = {
	FILTER_FLAG_EX_INFORMATION,	//	�t�B���^�̃t���O
	//	FILTER_FLAG_ALWAYS_ACTIVE		: �t�B���^����ɃA�N�e�B�u�ɂ��܂�
	//	FILTER_FLAG_CONFIG_POPUP		: �ݒ���|�b�v�A�b�v���j���[�ɂ��܂�
	//	FILTER_FLAG_CONFIG_CHECK		: �ݒ���`�F�b�N�{�b�N�X���j���[�ɂ��܂�
	//	FILTER_FLAG_CONFIG_RADIO		: �ݒ�����W�I�{�^�����j���[�ɂ��܂�
	//	FILTER_FLAG_EX_DATA				: �g���f�[�^��ۑ��o����悤�ɂ��܂��B
	//	FILTER_FLAG_PRIORITY_HIGHEST	: �t�B���^�̃v���C�I���e�B����ɍŏ�ʂɂ��܂�
	//	FILTER_FLAG_PRIORITY_LOWEST		: �t�B���^�̃v���C�I���e�B����ɍŉ��ʂɂ��܂�
	//	FILTER_FLAG_WINDOW_THICKFRAME	: �T�C�Y�ύX�\�ȃE�B���h�E�����܂�
	//	FILTER_FLAG_WINDOW_SIZE			: �ݒ�E�B���h�E�̃T�C�Y���w��o����悤�ɂ��܂�
	//	FILTER_FLAG_DISP_FILTER			: �\���t�B���^�ɂ��܂�
	//	FILTER_FLAG_EX_INFORMATION		: �t�B���^�̊g������ݒ�ł���悤�ɂ��܂�
	//	FILTER_FLAG_NO_CONFIG			: �ݒ�E�B���h�E��\�����Ȃ��悤�ɂ��܂�
	//	FILTER_FLAG_AUDIO_FILTER		: �I�[�f�B�I�t�B���^�ɂ��܂�
	//	FILTER_FLAG_RADIO_BUTTON		: �`�F�b�N�{�b�N�X�����W�I�{�^���ɂ��܂�
	//	FILTER_FLAG_WINDOW_HSCROLL		: �����X�N���[���o�[�����E�B���h�E�����܂�
	//	FILTER_FLAG_WINDOW_VSCROLL		: �����X�N���[���o�[�����E�B���h�E�����܂�
	//	FILTER_FLAG_IMPORT				: �C���|�[�g���j���[�����܂�
	//	FILTER_FLAG_EXPORT				: �G�N�X�|�[�g���j���[�����܂�
	0, 0,						//	�ݒ�E�C���h�E�̃T�C�Y (FILTER_FLAG_WINDOW_SIZE�������Ă��鎞�ɗL��)
	"Liquid Rescale",			//	�t�B���^�̖��O
	TRACK_N,					//	�g���b�N�o�[�̐� (0�Ȃ疼�O�����l����NULL�ł悢)
	track_name,					//	�g���b�N�o�[�̖��O�S�ւ̃|�C���^
	track_default,				//	�g���b�N�o�[�̏����l�S�ւ̃|�C���^
	track_s, track_e,			//	�g���b�N�o�[�̐��l�̉������ (NULL�Ȃ�S��0�`256)
	CHECK_N,					//	�`�F�b�N�{�b�N�X�̐� (0�Ȃ疼�O�����l����NULL�ł悢)
	check_name,					//	�`�F�b�N�{�b�N�X�̖��O�S�ւ̃|�C���^
	check_default,				//	�`�F�b�N�{�b�N�X�̏����l�S�ւ̃|�C���^
	func_proc,					//	�t�B���^�����֐��ւ̃|�C���^ (NULL�Ȃ�Ă΂�܂���)
	NULL,						//	�J�n���ɌĂ΂��֐��ւ̃|�C���^ (NULL�Ȃ�Ă΂�܂���)
	NULL,						//	�I�����ɌĂ΂��֐��ւ̃|�C���^ (NULL�Ȃ�Ă΂�܂���)
	NULL,						//	�ݒ肪�ύX���ꂽ�Ƃ��ɌĂ΂��֐��ւ̃|�C���^ (NULL�Ȃ�Ă΂�܂���)
	func_WndProc,						//	�ݒ�E�B���h�E�ɃE�B���h�E���b�Z�[�W���������ɌĂ΂��֐��ւ̃|�C���^ (NULL�Ȃ�Ă΂�܂���)
	NULL, NULL,					//	�V�X�e���Ŏg���܂��̂Ŏg�p���Ȃ��ł�������
	NULL,						//  �g���f�[�^�̈�ւ̃|�C���^ (FILTER_FLAG_EX_DATA�������Ă��鎞�ɗL��)
	NULL,						//  �g���f�[�^�T�C�Y (FILTER_FLAG_EX_DATA�������Ă��鎞�ɗL��)
	"Liquid rescale v1.0 by MT",
	//  �t�B���^���ւ̃|�C���^ (FILTER_FLAG_EX_INFORMATION�������Ă��鎞�ɗL��)
	NULL,						//	�Z�[�u���J�n����钼�O�ɌĂ΂��֐��ւ̃|�C���^ (NULL�Ȃ�Ă΂�܂���)
	NULL,						//	�Z�[�u���I���������O�ɌĂ΂��֐��ւ̃|�C���^ (NULL�Ȃ�Ă΂�܂���)
};


//---------------------------------------------------------------------
//		�t�B���^�\���̂̃|�C���^��n���֐�
//---------------------------------------------------------------------
EXTERN_C FILTER_DLL __declspec(dllexport) * __stdcall GetFilterTable(void)
{
	return &filter;
}
//	���L�̂悤�ɂ����1��auf�t�@�C���ŕ����̃t�B���^�\���̂�n�����Ƃ��o���܂�
/*
FILTER_DLL *filter_list[] = {&filter,&filter2,NULL};
EXTERN_C FILTER_DLL __declspec(dllexport) ** __stdcall GetFilterTableList( void )
{
return (FILTER_DLL **)&filter_list;
}
*/


//---------------------------------------------------------------------
//		�t�B���^�����֐�
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
