/*--------------------------------------------------------------------------
* GDI+�̃��b�p�[
* 
* created by Nekorabbit
*--------------------------------------------------------------------------*/
#pragma once

#include <objidl.h>
#include <gdiplus.h>
#include "GDICapture.h"

#undef min
#undef max

namespace chkimg {

void multi_byte_to_wchar(
	const std::string& value, std::wstring& out
) {
	int wideCharLen = ::MultiByteToWideChar(CP_ACP, 0, value.c_str(), -1, NULL, 0);
	out.resize(wideCharLen);
	out.resize(::MultiByteToWideChar(CP_ACP, 0, value.c_str(), -1, &out[0], wideCharLen));
}

void multi_byte_to_wchar(
	const char* value, std::wstring& out
) {
	int wideCharLen = ::MultiByteToWideChar(CP_ACP, 0, value, -1, NULL, 0);
	out.resize(wideCharLen);
	out.resize(::MultiByteToWideChar(CP_ACP, 0, value, -1, &out[0], wideCharLen));
}

//////////////////////////////////////////////////////////////////////////
// Encoder
//////////////////////////////////////////////////////////////////////////

GDIEncoder::GDIEncoder()
{
	if(initEncoderClsid() == false)
		throw "�G���R�[�_��������܂���";
}

// �⏕�֐�
void GDIEncoder::saveBitmapAsBMP(Gdiplus::Bitmap* bitmap, const char* filename_) const {
	std::wstring filename; multi_byte_to_wchar(filename_, filename);
	bitmap->Save(filename.c_str(), &bmpEncoder, NULL);
}
void GDIEncoder::saveBitmapAsPNG(Gdiplus::Bitmap* bitmap, const char* filename_) const {
	std::wstring filename; multi_byte_to_wchar(filename_, filename);
	bitmap->Save(filename.c_str(), &pngEncoder, NULL);
}
void GDIEncoder::saveBitmapAsJPEG(Gdiplus::Bitmap* bitmap, const char* filename_) const {
	std::wstring filename; multi_byte_to_wchar(filename_, filename);
	bitmap->Save(filename.c_str(), &jpegEncoder, NULL);
}

bool GDIEncoder::initEncoderClsid() {
	UINT  num = 0, size = 0;

	Gdiplus::GetImageEncodersSize(&num, &size);
	if (size == 0)
		return false;

	Gdiplus::ImageCodecInfo* pImageCodecInfo = (Gdiplus::ImageCodecInfo*)malloc(size);
	if (pImageCodecInfo == NULL)
		return false;  // Failure

	Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);

	int numEncoders = 0;
	for(UINT j = 0; j < num; ++j) {
		if (wcscmp(pImageCodecInfo[j].MimeType, L"image/bmp") == 0) {
			bmpEncoder = pImageCodecInfo[j].Clsid;
			++numEncoders;
		}
		else if (wcscmp(pImageCodecInfo[j].MimeType, L"image/png") == 0) {
			pngEncoder = pImageCodecInfo[j].Clsid;
			++numEncoders;
		}
		else if (wcscmp(pImageCodecInfo[j].MimeType, L"image/jpeg") == 0) {
			jpegEncoder = pImageCodecInfo[j].Clsid;
			++numEncoders;
		}
	}

	free(pImageCodecInfo);
	return numEncoders == 3;
}

//////////////////////////////////////////////////////////////////////////
// �摜�t�@�C���Í���
//////////////////////////////////////////////////////////////////////////

static DWORD g_encrypt_key = 0xE8F29FACUL;

void xorEncrypt(void* data, int len) {
	DWORD* ptr = (DWORD*)data;
	for(int i = 0; i < (len/sizeof(DWORD)); ++i) {
		ptr[i] ^= g_encrypt_key;
	}
}

//////////////////////////////////////////////////////////////////////////
// �⏕�֐�
//////////////////////////////////////////////////////////////////////////

void ForceIntoBoundingBox(RECT* rect,
						  int bleft, int btop, int bw, int bh)
{
	rect->top = std::max<int>(rect->top, btop);
	rect->left = std::max<int>(rect->left, bleft);
	rect->right = std::min<int>(rect->right, btop + bw);
	rect->bottom = std::min<int>(rect->bottom, btop + bh);
}

// �摜��ǂݍ��񂾃������u���b�N��IStream���쐬
IStream * createFileStream(const char *filepath, bool decrypt)
{
	// �摜�t�@�C���I�[�v��
	const HANDLE hFile = CreateFile(filepath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile == INVALID_HANDLE_VALUE)
		throw_exception("�t�@�C����������܂���: %s", filepath);

	// �t�@�C���T�C�Y�擾
	const DWORD dwFileSize = GetFileSize(hFile, NULL);
	// �摜�t�@�C���f�[�^�i�[�p�������u���b�N�m��
	const HGLOBAL hBuf = GlobalAlloc(GMEM_MOVEABLE, dwFileSize);
	if(hBuf == NULL)
		throw_exception("GlobalAlloc�Ɏ��s");
		
	// �������u���b�N�����b�N���A�h���X���擾
	const LPVOID lpBuf = GlobalLock(hBuf);
	// �摜�t�@�C���̃f�[�^���������u���b�N�ɓǂݍ���
	DWORD dwLoadSize;
	ReadFile(hFile, lpBuf, dwFileSize, &dwLoadSize, NULL);
	if(dwFileSize != dwLoadSize)
		throw_exception("ReadFile�Ɏ��s");
	CloseHandle(hFile);
	// �������u���b�N�̃��b�N����
	GlobalUnlock(hBuf);

	// �K�v�Ȃ畜��
	if(decrypt) {
		xorEncrypt(lpBuf, dwLoadSize);
	}

	// �������u���b�N����IStream���쐬
	IStream * isFile;
	HRESULT hr = CreateStreamOnHGlobal(hBuf, TRUE, &isFile);
	if(FAILED(hr) || isFile == NULL)
		throw_exception("CreateStreamOnHGlobal�Ɏ��s");
	// IStream��Ԃ�
	return isFile;
}

// �摜��ǂݍ��񂾃������u���b�N��IStream���쐬
IStream * createResourceStream(HMODULE module, WORD id)
{
	HRSRC hRsc = FindResource(module, MAKEINTRESOURCE(id), "PNG");
	if(hRsc == NULL)
		throw_exception("���\�[�X��������܂���");
	
	// �܂��A���ۂ͉������Ȃ��A�����̎擾�֐��Ȃ񂾂��ǂ�
	DWORD dwFileSize = SizeofResource(module, hRsc);
	if(dwFileSize == 0)
		throw_exception("���\�[�X�̃T�C�Y�擾�Ɏ��s");
	
	// �܂��A���ۂ͉������Ȃ��A�����̎擾�֐��Ȃ񂾂��ǂ�
	HGLOBAL hData = LoadResource(module, hRsc);
	if(hData == NULL)
		throw_exception("���\�[�X�̃��[�h�Ɏ��s");
	
	// �܂��A���ۂ͉������Ȃ��A�����̎擾�֐��Ȃ񂾂��ǂ�
	LPVOID pData = LockResource(hData);
	if(pData == NULL)
		throw_exception("���\�[�X�̃��b�N�Ɏ��s");

	// �摜�t�@�C���f�[�^�i�[�p�������u���b�N�m��
	const HGLOBAL hBuf = GlobalAlloc(GMEM_MOVEABLE, dwFileSize);
	if(hBuf == NULL)
		throw_exception("GlobalAlloc�Ɏ��s");
		
	// �������u���b�N�����b�N���A�h���X���擾
	const LPVOID lpBuf = GlobalLock(hBuf);
	// �摜�t�@�C���̃f�[�^���������u���b�N�ɓǂݍ���
	memcpy(lpBuf, pData, dwFileSize);
	// �������u���b�N�̃��b�N����
	GlobalUnlock(hBuf);

	// �������u���b�N����IStream���쐬
	IStream * isFile;
	HRESULT hr = CreateStreamOnHGlobal(hBuf, TRUE, &isFile);
	if(FAILED(hr) || isFile == NULL)
		throw_exception("CreateStreamOnHGlobal�Ɏ��s");
	// IStream��Ԃ�
	return isFile;
}

GDIBitmap* copyBitmap(HDC winDC, int x, int y, int w, int h, double scale) {
	int dst_w = w * scale;
	int dst_h = h * scale;
	
	// �R�s�[��̃r�b�g�}�b�v���쐬
	GDIBitmap* bmp = new GDIBitmap(dst_w, dst_h);
	try {
		//Graphics�̍쐬
		Gdiplus::Graphics g(bmp->ptr());
		//Graphics�̃f�o�C�X�R���e�L�X�g���擾
		HDC hDC = g.GetHDC();
		//Bitmap�ɉ摜���R�s�[����
		if(scale == 1.0) {
			if(BitBlt(hDC, 0, 0, w, h, winDC, x, y, SRCCOPY) == 0)
				throw_exception("BitBlt�Ɏ��s");
		}
		else {
			if(StretchBlt(hDC, 0, 0, dst_w, dst_h, winDC, x, y, w, h, SRCCOPY) == 0)
				throw_exception("StretchBlt�Ɏ��s");
		}
		//���
		g.ReleaseHDC(hDC);
	}
	catch(const char* str) {
		delete bmp; bmp = NULL;
		throw str;
	}

	return bmp;
}

// �֐�����A���
// ���ۂɃL���v�`���������W��rect�ɓ���
GDIBitmap* captureScreen(HWND hWnd, RECT* rect, double scale, bool backgroud_mode) {
	// �f�X�N�g�b�v�X�N���[���̃f�o�C�X�R���e�L�X�g���쐬
	HDC winDC;
	if(hWnd != NULL) {
		// ����E�B���h�E
		winDC = GetWindowDC(hWnd);
		if(winDC == NULL)
				throw_exception("GetWindowDC�Ɏ��s");
	}
	else {
		// �f�X�N�g�b�v
		winDC = GetDC(NULL);
		if(winDC == NULL)
				throw_exception("GetDC�Ɏ��s");
	}
 
	// �͈͂��E�B���h�E���Ɏ��߂�
	RECT wndRect;
	if(hWnd != NULL) {
		GetWindowRect(hWnd, &wndRect);
		ForceIntoBoundingBox(rect,
			0, 0, wndRect.right - wndRect.left, wndRect.bottom - wndRect.top);
	}
	else {
		ForceIntoBoundingBox(rect,
			GetSystemMetrics(SM_XVIRTUALSCREEN), GetSystemMetrics(SM_YVIRTUALSCREEN),
			GetSystemMetrics(SM_CXVIRTUALSCREEN), GetSystemMetrics(SM_CYVIRTUALSCREEN));
	}
	int x = rect->left, y = rect->top;
	int w = rect->right - rect->left, h = rect->bottom - rect->top;
	
	GDIBitmap* bmp = NULL;
	GDIBitmap* tmp = NULL;

	if(hWnd != NULL && !backgroud_mode) {
		// ���ڎ��Ȃ��ꍇ������̂�PrintWindow�Ŏ���Ă���
		int win_w = wndRect.right - wndRect.left;
		int win_h = wndRect.bottom - wndRect.top;
		tmp = new GDIBitmap(win_w, win_h);
		try {
			//Graphics�̍쐬
			Gdiplus::Graphics g(tmp->ptr());
			//Graphics�̃f�o�C�X�R���e�L�X�g���擾
			HDC hDC = g.GetHDC();
			if(PrintWindow(hWnd, hDC, 0) == 0)
				throw_exception("PrintWindow�Ɏ��s");

			if(x == 0 && y == 0 && w == win_w && h == win_h && scale == 1.0) {
				// �E�B���h�E�S�̂����̂܂܂̑傫���Ȃ̂ŃR�s�[���Ȃ�
				bmp = tmp;
			}
			else {
				// PrintWindow����bitmap����R�s�[
				bmp = copyBitmap(hDC, x, y, w, h, scale);
			}
			//���
			g.ReleaseHDC(hDC);
		}
		catch(const char* str) {
			delete tmp; tmp = NULL;
			throw str;
		}
	}
	else {
		// ���ڃR�s�[
		bmp = copyBitmap(winDC, x, y, w, h, scale);
	}

	// ���
	ReleaseDC(hWnd, winDC);
	if(tmp != NULL && tmp != bmp) {
		delete tmp; tmp = NULL;
	}

	return bmp;
}

GDIBitmap* captureScreen(HWND hWnd, double scale) {
	RECT rect;
	if(hWnd == NULL) {
		// �f�X�N�g�b�v�S�̂��L���v�`��
		rect.left = GetSystemMetrics(SM_XVIRTUALSCREEN);
		rect.top = GetSystemMetrics(SM_YVIRTUALSCREEN);
		rect.right = rect.left + GetSystemMetrics(SM_CXVIRTUALSCREEN);
		rect.bottom = rect.top + GetSystemMetrics(SM_CYVIRTUALSCREEN);
	}
	else {
		GetWindowRect(hWnd, &rect);
		rect.right -= rect.left; rect.left = 0;
		rect.bottom -= rect.top; rect.top = 0;
	}
	return captureScreen(hWnd, &rect, scale);
}

} // chkimg
