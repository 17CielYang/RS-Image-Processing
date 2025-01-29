#include <string>
#include <vector>
#include <iostream>
#include <stdio.h>
#include <gdal.h>
#include <gdal_priv.h>

using namespace std;

bool bFileLoad; // 文件数据载入标志
unsigned char * pDataRead; // 原始数据容器
unsigned char * pDataWrite; // 增强数据容器
int oriWidth; 
int oriHeight; 
const char * FilePathIn = "/Users/shiqiyang/Files/Codes/RSI-fundation/lab6/data/band1.tif";
const char * FilePathSmooth = "/Users/shiqiyang/Files/Codes/RSI-fundation/lab6/data/band1_smooth_1.tif";
const char * FilePathSharpen = "/Users/shiqiyang/Files/Codes/RSI-fundation/lab6/data/band1_sharpen_1.tif";

// ————————————声明函数————————————
bool LoadImage();
bool Smooth();
bool Sharpen();

bool LoadImage(){
    // 定义GDAL操作句柄
    GDALDataset* poDataset; // 指向对象数据的指针
    GDALRasterBand* poBand; // 指向对象数据单个波段的指针
    poDataset = (GDALDataset*)GDALOpen(FilePathIn, GA_ReadOnly);
    if(!poDataset) {
        cout << "Open image error!";
        return false;
    }
    // 获取图像基本信息
    oriWidth = poDataset->GetRasterXSize(); // 获取图像宽
    oriHeight = poDataset->GetRasterYSize(); // 获取图像高

    // 申请原始数据内存空间
    pDataRead = new unsigned char[oriHeight * oriWidth];
    if(pDataRead == NULL){
        cout << "Read memory error!";
        return false;
    }

    // 申请增强数据内存空间
    pDataWrite = new unsigned char[oriHeight * oriWidth];
    if(pDataWrite == NULL) {
        cout << "Write memory error!";
        return false;
    }

    // 读入原始图像数据
    poBand = poDataset->GetRasterBand(1);
    poBand->RasterIO(GF_Read, 0, 0, oriWidth, oriHeight, pDataRead, oriWidth, oriHeight, GDT_Byte, 0, 0);

    // 初始化增强数据
    memset(pDataWrite, 0, oriWidth * oriHeight);

    // 释放句柄
    GDALClose(poDataset);

    // 标志文件数据载入成功
    bFileLoad = TRUE;

    // 提示文件数据载入成功
    cout << "Load Image Done!" << endl;
    return true;
}

// ————————————均值平滑————————————
bool Smooth(){
    // 平滑模版
    int _Smooth_Template[9];
    for(int i = 0; i < 9; i++){
        _Smooth_Template[i] = 1;
    }

    // 文件数据未载入则直接退出
    if(!bFileLoad){
        cout << "File not load!";
        return false;
    }

    int iMinX, iMaxX;
    int iMinY, iMaxY;
    double dTmp;

    // 循环滑动模版
    for(int i = 0; i < oriWidth; i++){
        for(int j = 0; j < oriHeight; j++){
            // 边界直接跳过不处理
            iMinX = i - 1;
            iMaxX = i + 1;
            iMinY = j - 1;
            iMaxY = j + 1;
            if(iMinX < 0 || iMaxY >= oriWidth) continue;
            if(iMinY < 0 || iMaxY >= oriHeight) continue;

            // 计算模板中心数值
            dTmp = pDataRead[(j - 1) * oriWidth + (i - 1)] * _Smooth_Template[0]
                + pDataRead[(j - 1) * oriWidth + (i - 0)] * _Smooth_Template[1]
                + pDataRead[(j - 1) * oriWidth + (i + 1)] * _Smooth_Template[2]
                + pDataRead[(j - 0) * oriWidth + (i - 1)] * _Smooth_Template[3]
                + pDataRead[(j - 0) * oriWidth + (i - 0)] * _Smooth_Template[4]
                + pDataRead[(j - 0) * oriWidth + (i + 1)] * _Smooth_Template[5]
                + pDataRead[(j + 1) * oriWidth + (i - 1)] * _Smooth_Template[6]
                + pDataRead[(j + 1) * oriWidth + (i - 0)] * _Smooth_Template[7]
                + pDataRead[(j + 1) * oriWidth + (i + 1)] * _Smooth_Template[8];
            dTmp = dTmp / 9;

            // 更新中心像元值
            if(dTmp < 0) {
                pDataWrite[j * oriWidth + i] = 0;
            }else if(dTmp > 255){
                pDataWrite[j * oriWidth + i] = 255;
            }else{
                pDataWrite[j * oriWidth + i] = (unsigned char)dTmp;
            }
        }
    }
    // 保存增强数据至结果文件
    // 创建输出文件的GDAL操作句柄
    GDALDriver* pDriver = NULL;
    pDriver = GetGDALDriverManager()->GetDriverByName("GTIFF");
    GDALDataset* poDatasetW = pDriver->Create(FilePathSmooth, oriWidth, oriHeight, 1, GDT_Byte, NULL);
    if(!poDatasetW){
        cout << "Open write file error!";
        return false;
    }

    // 写输出数据
    GDALRasterBand* poBandW;
    poBandW = poDatasetW->GetRasterBand(1);
    poBandW->RasterIO(GF_Write, 0, 0, oriWidth, oriHeight, pDataWrite, oriWidth, oriHeight, GDT_Byte, 0, 0);

    // 释放句柄
    GDALClose(poDatasetW);

    // 提示成功
    cout << "Smooth Done!" << endl;
    return true;
}

// ————————————拉普拉斯锐化————————————
bool Sharpen()
{
    // 锐化模板
    int _Sharpen_Template[9];
    _Sharpen_Template[0] = 0; _Sharpen_Template[1] = -1; _Sharpen_Template[2] = 0;
    _Sharpen_Template[3] = -1; _Sharpen_Template[4] = 5; _Sharpen_Template[5] = -1;
    _Sharpen_Template[6] = 0; _Sharpen_Template[7] = -1; _Sharpen_Template[8] = 0;

    // 文件数据未载入则直接退出
    if (!bFileLoad) {
        cout << "File not load!";
        return false;
    }

    int iMinX, iMaxX;
    int iMinY, iMaxY;
    double dTmp;

    // 滑化滑动模板
    for (int i = 0; i < oriWidth; i++) {
        for (int j = 0; j < oriHeight; j++) {

            // 边界直接跳过不处理
            iMinX = i - 1; iMaxX = i + 1;
            iMinY = j - 1; iMaxY = j + 1;
            if (iMinX < 0 || iMaxX > oriWidth) { continue; }
            if (iMinY < 0 || iMaxY > oriHeight) { continue; }

            // 中心模板中心点数值
            dTmp = pDataRead[(j - 1) * oriWidth + (i - 1)] * _Sharpen_Template[0]
                 + pDataRead[(j - 1) * oriWidth + (i)] * _Sharpen_Template[1]
                 + pDataRead[(j - 1) * oriWidth + (i + 1)] * _Sharpen_Template[2]
                 + pDataRead[(j) * oriWidth + (i - 1)] * _Sharpen_Template[3]
                 + pDataRead[(j) * oriWidth + (i)] * _Sharpen_Template[4]
                 + pDataRead[(j) * oriWidth + (i + 1)] * _Sharpen_Template[5]
                 + pDataRead[(j + 1) * oriWidth + (i - 1)] * _Sharpen_Template[6]
                 + pDataRead[(j + 1) * oriWidth + (i)] * _Sharpen_Template[7]
                 + pDataRead[(j + 1) * oriWidth + (i + 1)] * _Sharpen_Template[8];
            // 更新中心像元值
            if (dTmp < 0) {
                pDataWrite[j * oriWidth + i] = 0;
            }
            else if (dTmp > 255) {
                pDataWrite[j * oriWidth + i] = 255;
            }
            else {
                pDataWrite[j * oriWidth + i] = (unsigned char)dTmp;
            }
        }
    }

    // 保存锐化结果至结果文件
    // 创建输出文件的GDAL读作句柄
    GDALDriver* pDriver = NULL;
    pDriver = GetGDALDriverManager()->GetDriverByName("GTIFF");
    GDALDataset* poDatasetW = pDriver->Create(FilePathSharpen, oriWidth, oriHeight, 1, GDT_Byte, NULL);
    if (!poDatasetW) {
        cout << "Open write file error!";
        return false;
    }

    // 写输出数据
    GDALRasterBand* poBandW;
    poBandW = poDatasetW->GetRasterBand(1);
    poBandW->RasterIO(GF_Write, 0, 0, oriWidth, oriHeight, pDataWrite, oriWidth, oriHeight, GDT_Byte, 0, 0);

    // 释放GDAL句柄
    GDALClose(poDatasetW);

    // 显示成功
    cout << "Sharpen Done!" << endl;
    return true;
}

int main() {
    // 初始化gdal
    GDALAllRegister();

    // 使gdal不默认使用UTF8编码，以支持中文路径
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    // 加载图像
    if (!LoadImage()) {
        cout << "Failed to load the image." << endl;
        return -1;
    }

    // 图像均值平滑
    if (!Smooth()) {
        cout << "Smooth operation failed." << endl;
        return -1;
    }
    cout << "Image smoothed successfully and saved to: " << FilePathSmooth << endl;

    // 图像拉普拉斯锐化
    if (!Sharpen()) {
        cout << "Sharpen operation failed." << endl;
        return -1;
    }
    cout << "Image sharpened successfully and saved to: " << FilePathSharpen << endl;

    // 释放内存
    if (pDataRead) {
        delete[] pDataRead;
        pDataRead = nullptr;
    }
    if (pDataWrite) {
        delete[] pDataWrite;
        pDataWrite = nullptr;
    }

    return 0;
}