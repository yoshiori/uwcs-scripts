/*--------------------------------------------------------------------------
* �e���v���[�g�}�b�`���O OpenCV���g�������� (Header)
* 
* created by Nekorabbit
*--------------------------------------------------------------------------*/
#pragma once

#include "opencv2/opencv.hpp"

#include "tmatch_opencv.h"

namespace chkimg {

void make_hole_empty(cv::Mat mask, int rw, int rh, cv::Point pt) {
	uchar* mask_data = (uchar*)mask.data;
	// �}�b�`�����̈���܂ޗ̈��mask���[���ɂ���
	int lower_x = std::max<int>(0, pt.x - rw/2 + 1);
	int lower_y = std::max<int>(0, pt.y - rh/2 + 1);
	int upper_x = std::min<int>(mask.cols, pt.x + rw/2);
	int upper_y = std::min<int>(mask.rows, pt.y + rh/2);
	for(int x = lower_x; x < upper_x; ++x) {
		for(int y = lower_y; y < upper_y; ++y) {
			int offset = y * mask.cols + x;
			mask_data[offset] = 0;
		}
	}
}

std::vector<SearchResult>& MatchTemplateGray::search(Bitmap ref, double diff_limit)
{
	result.clear();

	const int rw = ref.width;
	const int rh = ref.height;
	const int iw = imgData.width;
	const int ih = imgData.height;

	cv::Mat refMat(rh, rw, CV_8UC1, ref.scan0);
	cv::Mat imgMat(ih, iw, CV_8UC1, imgData.scan0);

	// �}�b�`���O���v�Z����̈�
	const int aw = iw - rw + 1;
	const int ah = ih - rh + 1;

	// �T���摜���Q�Ɖ摜��菬�����ꍇ�̓}�b�`�Ȃ�
	if(aw <= 0 || ah <= 0) {
		return result;
	}
	
	cv::Mat tmScore = cv::Mat::zeros(ah, aw, CV_32FC1);
	//int method = CV_TM_SQDIFF;
	//int method = CV_TM_SQDIFF_NORMED;
	//int method = CV_TM_CCORR;
	int method = CV_TM_CCORR_NORMED;
	//int method = CV_TM_CCOEFF;
	//int method = CV_TM_CCOEFF_NORMED;
	cv::matchTemplate(imgMat, refMat, tmScore, method);

	cv::Mat mask = cv::Mat::zeros(ah, aw, CV_8UC1)+1;
	for(int points = 0; points < MAX_MATCH_POINTS; ++points) {
		double score;
		cv::Point loc;

		// �ł��X�R�A�̍��������ʒu���o��
		// �ő�
		cv::minMaxLoc(tmScore, NULL, &score, NULL, &loc, mask);
		//if(1.0 - diff_limit > score) break;
		// �ŏ�
		//cv::minMaxLoc(tmScore, &score, NULL, &loc, NULL, mask);
		score = 100 - score * 100;
		// �Ȃ񂩌덷������̂ŏ�����3�ʂ��炢�Ŋۂ߂�
		score = (int)(score * 1000) / 1000.0;
		if(diff_limit < score)
			break;
#ifdef _CONSOLE
		printf("(x,y,s)=(%d,%d,%f)\n", loc.x, loc.y, score);
#endif
		SearchResult point;
		point.x = loc.x;
		point.y = loc.y;
		point.diff = score;
		result.push_back(point);

		// �}�b�`�����̈���܂ޗ̈��mask���[���ɂ���
		make_hole_empty(mask, rw, rh, loc);
	}

	return result;
}

std::vector<SearchResult>& MatchTemplateRGB::search(Bitmap ref, double diff_limit)
{
	result.clear();

	std::vector<cv::Mat> refBGR;
	split_bitmap(refBGR, ref);
	
	const int iw = imgData.width;
	const int ih = imgData.height;
		
	const int rw = ref.width;
	const int rh = ref.height;

	// �}�b�`���O���v�Z����̈�
	const int aw = iw - rw + 1;
	const int ah = ih - rh + 1;

	// �T���摜���Q�Ɖ摜��菬�����ꍇ�̓}�b�`�Ȃ�
	if(aw <= 0 || ah <= 0) {
		return result;
	}

	cv::Mat tmScore = cv::Mat::zeros(ah, aw, CV_32FC1);
	cv::Mat tmpScore(ah, aw, CV_32FC1);
	for(int ch = 0; ch < 3; ++ch) {
		//int method = CV_TM_SQDIFF;
		//int method = CV_TM_SQDIFF_NORMED;
		//int method = CV_TM_CCORR;
		int method = CV_TM_CCORR_NORMED;
		//int method = CV_TM_CCOEFF;
		//int method = CV_TM_CCOEFF_NORMED;
		cv::matchTemplate(imgBGR[ch], refBGR[ch], tmpScore, method);
		tmScore += tmpScore;
	}

	cv::Mat mask = cv::Mat::zeros(ah, aw, CV_8UC1)+1;
	for(int points = 0; points < MAX_MATCH_POINTS; ++points) {
		double score;
		cv::Point loc;

		// �ł��X�R�A�̍��������ʒu���o��
		// �ő�
		cv::minMaxLoc(tmScore, NULL, &score, NULL, &loc, mask);
#if 1
		score = 300 - score * 100;
		// �Ȃ񂩌덷������̂ŏ�����3�ʂ��炢�Ŋۂ߂�
		score = (int)(score * 1000) / 1000.0;
		if(diff_limit < score)
			break;
#else
		// �ŏ�
		cv::minMaxLoc(tmScore, &score, NULL, &loc, NULL, mask);
		score *= 100.0;
		if(diff_limit <= score)
			break;
#endif
#ifdef _CONSOLE
		printf("(x,y,s)=(%d,%d,%f)\n", loc.x, loc.y, score);
#endif
		SearchResult point;
		point.x = loc.x;
		point.y = loc.y;
		point.diff = score;
		result.push_back(point);
		
		// �}�b�`�����̈���܂ޗ̈��mask���[���ɂ���
		make_hole_empty(mask, rw, rh, loc);
	}

	return result;
}

void MatchTemplateRGB::split_bitmap(std::vector<cv::Mat>& bgr, const Bitmap& img) {
	uchar* src = (uchar*)img.scan0;
	for(int ch = 0; ch < 3; ++ch) {
		cv::Mat imgMat(img.height, img.width, CV_8UC1);
		bgr.push_back(imgMat);
		uchar* dst = imgMat.data;
		int dst_step = imgMat.step;

		for(int y = 0; y < img.height; ++y) {
			for(int x = 0; x < img.width; ++x) {
				dst[y * dst_step + x] = src[y * img.stride + x * 3 + ch];
			}
		}
	}
}

}