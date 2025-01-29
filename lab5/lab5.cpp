#include <string>
#include <vector>
#include <iostream>
#include <stdio.h>
#include <gdal.h>
#include <gdal_priv.h>

using namespace std;

int main(){
    // 初始化gdal
    GDALAllRegister();

    // 使gdal不默认使用UTF8编码，以支持中文路径
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    // 定义参数并读取影像
    int oriWidth; // 图像宽
    int oriHeight; // 图像高
    int oriBand; // 图像波段数
    unsigned char * oriData; // 原始影像存储空间
    unsigned char * equData; // 均衡化后影像存储空间
    unsigned char * oriHistImg; // 原始影像直方图图像
    unsigned char * equHistImg; // 均衡化后影像直方图图像
    // 获取输入文件的路径
    const char * FilePathIn = "/Users/shiqiyang/Files/Codes/RSI-fundation/lab5/data/band1.dat";
    const char * FilePathOut = "/Users/shiqiyang/Files/Codes/RSI-fundation/lab5/data/euqalization.tif";
    const char * oriHistImgPath = "/Users/shiqiyang/Files/Codes/RSI-fundation/lab5/data/oriHistImg.tif";
    const char * equHistImgPath = "/Users/shiqiyang/Files/Codes/RSI-fundation/lab5/data/equHistImg.tif";

    // 定义GDAL操作句柄
    GDALDataset* poDataset = (GDALDataset*)GDALOpen(FilePathIn, GA_ReadOnly);
    if(!poDataset){
        cout << "ERROR: cannot open image file";
        return 1;
    }

    // 获取图像基本信息
    oriWidth = poDataset->GetRasterXSize();
    oriHeight = poDataset->GetRasterYSize();
    oriBand = poDataset->GetRasterCount();

    // 分配原始影像存储空间
    oriData = (unsigned char*)CPLMalloc(sizeof(unsigned char)*oriWidth*oriHeight*oriBand);
    equData = (unsigned char*)CPLMalloc(sizeof(unsigned char)*oriWidth*oriHeight*oriBand);

    // 获取原始影像数据
    GDALRasterBand *poBand = poDataset->GetRasterBand(1);
    poBand->RasterIO(GF_Read, 0, 0, oriWidth, oriHeight, oriData, oriWidth, oriHeight, GDT_Byte, 0, 0);

    // ————————————————统计原始直方图并绘制——————————————————————
    float oriHist[256] = {};
    float oriMax = 0;
    for(int i = 0; i < oriHeight * oriWidth; i++){
        oriHist[oriData[i]] ++;
    }

    // 找到波段的最大值
    for(int i = 0; i < 256; i++){
        oriHist[i] = oriHist[i] / (oriHeight * oriWidth);
        if(i == 0){
            oriMax = oriHist[i];
        }else if(oriMax < oriHist[i]){
            oriMax = oriHist[i];
        }
    }

    oriHistImg = new unsigned char[256 * 256]();
    float fStepY = 256.0 / (oriMax * (float)1.2);
    for(int i = 0; i < 256; i++){
        for(int j = 0; j < int(oriHist[i] * fStepY); j++){
            oriHistImg[(255 - j) * 256 + i] = 255;
        }
    }

    GDALDriver *pDriver = GetGDALDriverManager()->GetDriverByName("GTIFF");
    GDALDataset *poDatasetW = pDriver->Create(oriHistImgPath, 256, 256, 1, GDT_Byte, NULL);
    if(!poDatasetW){
        cout << "ERROR:Create OriHistImg failed";
        return 1;
    }
    GDALRasterBand *poBandW = poDatasetW->GetRasterBand(1);
    // 写出原始影像直方图图像
    poBandW->RasterIO(GF_Write, 0, 0, 256, 256, oriHistImg, 256, 256, GDT_Byte, 0, 0);
    cout << "Output OriHistImge Done!" << endl;
    // ————————————————直方图均衡化———————————————————————
    float equHist[256] = {};
    equData = new unsigned char[oriHeight * oriWidth];
    for(int i = 0; i < 256; i++){
        if(i == 0){
            equHist[i] = oriHist[i];
        }else{
            equHist[i] = equHist[i - 1] + oriHist[i];
        }
    }
    // 计算灰度映射关系
    for(int i = 0; i < 256; i++){
        equHist[i] = float((int)(255.0 * equHist[i] + 0.5));
    }
    // 对原图像进行直方图均衡化映射
    for(int i = 0; i < oriHeight * oriWidth; i++){
        if(oriData[i] == 0){
            equData[i] = 0;
        }else{
            equData[i] = (unsigned char)equHist[oriData[i]];
        }
    }

    // 写出直方图均衡后图像
    pDriver = GetGDALDriverManager()->GetDriverByName("GTIFF");
    poDatasetW = pDriver->Create(FilePathOut, oriWidth, oriHeight, oriBand, GDT_Byte, NULL);
    if(!poDatasetW) {
        cout << "ERROR: Create EquHistImg failed";
        return 1;
    }
    poBandW = poDatasetW->GetRasterBand(1);

    // 写出结果影像的波段
    poBandW->RasterIO(GF_Write, 0, 0, oriWidth, oriHeight, equData, oriWidth, oriHeight, GDT_Byte, 0, 0);
    cout << "Equalization Done!" << endl;

    // ——————————————计算增强后的直方图并绘制—————————————————
    // 统计新直方图
    float equMax = 0;
    memset(equHist, 0, 256*sizeof(float));
    for(int i = 0; i < oriHeight * oriWidth; i++){
        equHist[equData[i]] ++;
    }
    // 找到波段的最大值
    for(int i = 0; i < 256; i++){
        equHist[i] = equHist[i] / (oriHeight * oriWidth);
        if(i == 0){
            equMax = equHist[i];
        }else if(equMax < equHist[i]){
            equMax = equHist[i];
        }
    }

    equHistImg = new unsigned char[256 * 256]();
    fStepY = 256.0 / (equMax * (float)1.2);
    for(int i = 0; i < 256; i++){
        for(int j = 0; j < int(equHist[i] * fStepY); j++){
            equHistImg[(255 - j) * 256 + i] = 255;
        }
    }

    pDriver = NULL;
    pDriver = GetGDALDriverManager()->GetDriverByName("GTIFF");
    poDatasetW = pDriver->Create(equHistImgPath, 256, 256, 1, GDT_Byte, NULL);
    if(!poDatasetW){
        cout << "ERROR: Create EquHistImg failed" << endl;
        return 1;
    }
    poBandW = poDatasetW->GetRasterBand(1);
    // 写出均衡化后影像直方图图像
    poBandW->RasterIO(GF_Write, 0, 0, 256, 256, equHistImg, 256, 256, GDT_Byte, 0, 0);
    cout << "Output EquHistImg Done!" << endl;

    // 释放内存
    delete[] oriData;
    oriData = NULL;
    delete[] equData;
    equData = NULL;
    delete[] oriHistImg;
    oriHistImg = NULL;
    delete[] equHistImg;
    equHistImg = NULL;
    GDALClose(poDataset);
    GDALClose(poDatasetW);

    return 0;
}