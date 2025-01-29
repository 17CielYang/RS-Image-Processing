#include <string>
#include <vector>
#include <iostream>
#include <stdio.h>
#include <gdal.h>
#include <gdal_priv.h>

using namespace std;

bool CalculateNDVIAndNDWI(const char *FilePathIn, const char *NDVIPathOut, const char *NDWIPathOut){
    // 读入原始影像文件
    // 定义文件读入的GDAL操作句柄
    GDALDataset* poDataset; // 指向对象数据的指针
    poDataset = (GDALDataset*)GDALOpen(FilePathIn, GA_ReadOnly);
    if(!poDataset){
        cout << "Error:cannot open image file";
        return 1;
    }

    // 获取影像的基本信息
    int Ori_Width = poDataset->GetRasterXSize(); // 获取图像宽
    int Ori_Height = poDataset->GetRasterYSize(); // 获取图像高

    // poBand2/3/4分别对应原始影像第2/3/4波段
    GDALRasterBand* poBand2; // 分别指向不同波段的指针
    GDALRasterBand* poBand3;
    GDALRasterBand* poBand4;
    poBand2 = poDataset->GetRasterBand(2);
    poBand3 = poDataset->GetRasterBand(3);
    poBand4 = poDataset->GetRasterBand(4);

    // 写出新图像文件
    // 创建输出文件的GDAL操作句柄
    GDALDriver* pDriver = NULL;
    pDriver = GetGDALDriverManager()->GetDriverByName("GTiff");
    GDALDataset* poNDVIW = pDriver->Create(NDVIPathOut, Ori_Width, Ori_Height, 1, GDT_Float64, NULL);
    GDALDataset* poNDWIW = pDriver->Create(NDWIPathOut, Ori_Width, Ori_Height, 1, GDT_Float64, NULL);
    if(!poNDVIW || !poNDWIW){
        cout << "Error: Create image failed";
        return false;
    }
    GDALRasterBand* poBandNDVIW = poNDVIW->GetRasterBand(1);
    GDALRasterBand* poBandNDWIW = poNDWIW->GetRasterBand(1);

    // 开辟内存空间（大小均为一整行）； OriData2/3/4分别用于记录原始影像第2/3/4波段
    double *OriData2 = new double[Ori_Width];
    double *OriData3 = new double[Ori_Width];
    double *OriData4 = new double[Ori_Width];

    double *NDVIData = new double[Ori_Width];
    double *NDWIData = new double[Ori_Width];
    if(!NDVIData || !NDWIData || !OriData3 || !OriData4){
        cout << "Error: allocate Memory Space failed";
        return false;
    }

    // 逐行计算写出新图像
    for(int i = 0; i < Ori_Height; i++){
        // 获取原始影像不同波段的值
        poBand2->RasterIO(GF_Read, 0, i, Ori_Width, 1, OriData2, Ori_Width, 1, GDT_Float64, 0, 0);
        poBand3->RasterIO(GF_Read, 0, i, Ori_Width, 1, OriData3, Ori_Width, 1, GDT_Float64, 0, 0);
        poBand4->RasterIO(GF_Read, 0, i, Ori_Width, 1, OriData4, Ori_Width, 1, GDT_Float64, 0, 0);

        // 计算NDVI和NDWI
        for(int j = 0; j < Ori_Width; j++){
            if(abs(OriData4[j] + OriData3[j]) < 0.0000001) NDVIData[j] = 0;
            else NDVIData[j] = (OriData4[j] - OriData3[j]) / (OriData4[j] + OriData3[j]);

            if(abs(OriData4[j] + OriData2[j]) < 0.0000001) NDWIData[j] = 0;
            else NDWIData[j] = (OriData2[j] - OriData4[j]) / (OriData2[j] + OriData4[j]);
        }
        poBandNDVIW->RasterIO(GF_Write, 0, i, Ori_Width, 1, NDVIData, Ori_Width, 1, GDT_Float64, 0, 0);
        poBandNDWIW->RasterIO(GF_Write, 0, i, Ori_Height, 1, NDVIData, Ori_Width, 1, GDT_Float64, 0, 0);
    }
    // 释放句柄及内存
        delete[] OriData2; 
        OriData2 = NULL;
        delete[] OriData3;
        OriData3 = NULL;
        delete[] OriData4;
        OriData4 = NULL;
        delete[] NDVIData;
        NDVIData = NULL;
        delete[] NDWIData;
        NDWIData = NULL;
        GDALClose(poDataset);
        GDALClose(poNDVIW);
        GDALClose(poNDWIW);
}

int main(){
    // 初始化gdal
    GDALAllRegister();

    // 使gdal不默认使用UTF-8编码，以支持中文路径
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    // 定义输入路径、输出路径
    const char * FilePathIn = "/Users/shiqiyang/Files/Codes/RSI-fundation/lab3/testData/hzsq.tif";
    const char * FileNDVIPathOut = "/Users/shiqiyang/Files/Codes/RSI-fundation/lab3/testData/NDVI1.tif";
    const char * FileNDWIPathOut = "/Users/shiqiyang/Files/Codes/RSI-fundation/lab3/testData/NDWI1.tif";

    CalculateNDVIAndNDWI(FilePathIn, FileNDVIPathOut, FileNDWIPathOut);
    return 0;
}