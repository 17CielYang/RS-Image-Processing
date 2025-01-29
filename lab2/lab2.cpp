#include <string>
#include <vector>
#include <iostream>
#include <stdio.h>
#include <gdal.h>
#include <gdal_priv.h>

using namespace std;

bool NearestNeighborInterpolation(const char *FilePathIn, const char *FilePathOut, int Resample_Width, int Resample_Height){
    // 定义读取图像的GDAL操作句柄
    GDALDataset* poDatasetR; // 指向对象数据的指针
    GDALRasterBand* poBandR; // 指向对象数据单个波段的指针
    poDatasetR = (GDALDataset*)GDALOpen(FilePathIn, GA_ReadOnly);
    if(!poDatasetR){
        cout << "ERROR:cannot open image file";
        return false;
    }

    // 获取图像基本信息
    int Ori_Width = poDatasetR->GetRasterXSize(); // 获取图像宽
    int Ori_Height = poDatasetR->GetRasterYSize(); // 获取图像高
    int Ori_Band = poDatasetR->GetRasterCount(); // 获取图像波段数

    // 获取重采样比例
    float Height_Scale, Width_Scale;
    Width_Scale = (float)Ori_Width / (float)Resample_Width;
    Height_Scale = (float)Ori_Height / (float)Resample_Height;

    // 新图像文件的输出创建与输出
    // 创建输出文件的GDAL操作句柄
    GDALDriver* pDriver = NULL;
    pDriver = GetGDALDriverManager()->GetDriverByName("GTIFF");
    GDALDataset* poDatasetW = pDriver->Create(FilePathOut, Resample_Width, Resample_Height, Ori_Band, GDT_Byte, NULL);
    if(!poDatasetW){
        cout << "ERROR: Create image failed";
        return false;
    }
    GDALRasterBand* poBandW;

    // 读取原图的投影字符串，并赋值给新图像
    const char * strProjectionInfo;
    strProjectionInfo = poDatasetR->GetProjectionRef();
    poDatasetW->SetProjection(strProjectionInfo);

    // 计算新的仿射变换系数，并赋值给新图像
    double geo_transform[6];
    poDatasetR->GetGeoTransform(geo_transform); // 获取原始图像的仿射变换参数
    double new_geo_transform[6]; // 新建新图像仿射变换系数
    double x_offset = 0.5 * geo_transform[1] - 0.5 * geo_transform[1] * Width_Scale; // 计算x坐标偏移量
    double y_offset = 0.5 * geo_transform[1] - 0.5 * geo_transform[5] * Height_Scale; // 计算y坐标偏移量
    new_geo_transform[0] = geo_transform[0] + x_offset; // 左上角X坐标
    new_geo_transform[1] = geo_transform[1] * Width_Scale; // 像素宽度
    new_geo_transform[2] = geo_transform[2]; // 旋转参数
    new_geo_transform[3] = geo_transform[3]; // 左上角Y坐标
    new_geo_transform[4] = geo_transform[4]; // 旋转参数
    new_geo_transform[5] = geo_transform[5] * Height_Scale; // 像素高度
    poDatasetW->SetGeoTransform(new_geo_transform); // 设置新图像的仿射变换参数

    // 开辟内存空间（仅为一行数据）
    unsigned char * OriLineData = NULL;
    unsigned char * ResampleLineData = NULL;
    OriLineData = new unsigned char[std::max(Ori_Width, Resample_Width)]; // 使用最大宽度以确保足够空间
    if(!OriLineData){
        cout << "ERROR: allocate OriLineData failed";
        return false;
    }
    ResampleLineData = new unsigned char[Resample_Width];
    if(!ResampleLineData){
        cout << "ERROR: allocate ResampleLineData failed";
        return false;
    }

    // 逐波段读入原始数据并重采样
    for(int b = 1; b <= Ori_Band; b++){
        poBandR = poDatasetR->GetRasterBand(b);
        poBandW = poDatasetW->GetRasterBand(b);
        for(int h = 0; h < Resample_Height; h++){
            // 读取原始影像对应行（最近邻的行）
            int corr_h = (int)(h * Height_Scale + 0.5);
            if(corr_h >= 0 && corr_h < Ori_Height) {
                poBandR->RasterIO(GF_Read, 0, corr_h, Ori_Width, 1, OriLineData, Ori_Width, 1, GDT_Byte, 0, 0);
            }

            // 对当前行进行重采样
            for(int w = 0; w < Resample_Width; w++){
                int corr_w = (int)(w * Width_Scale + 0.5);
                if(corr_w >= 0 && corr_w < Ori_Width) {
                    ResampleLineData[w] = OriLineData[corr_w];
                }
            }

            // 写入重采样后的数据行
            poBandW->RasterIO(GF_Write, 0, h, Resample_Width, 1, ResampleLineData, Resample_Width, 1, GDT_Byte, 0, 0);
        }
    }

    // 释放内存
    delete[] OriLineData;
    delete[] ResampleLineData;

    GDALClose(poDatasetR);
    GDALClose(poDatasetW);

    cout << "Nearest Neighbor Interpolation Done!";
    return true;
}

bool BilinearInterpolation(const char *FilePathIn, const char * FilePathOut, int Resample_Width, int Resample_Height){
    // 读入原始影像文件
    // 定义文件读入的GDAL操作句柄
    GDALDataset* poDatasetR; // 指向对象数据的指针
    GDALRasterBand* poBand; // 指向对象数据单个波段的指针
    poDatasetR = (GDALDataset*)GDALOpen(FilePathIn, GA_ReadOnly);
    if(!poDatasetR){
        cout << "ERROR:cannot open image file";
        return false;
    }

    // 获取图像基本信息
    int Ori_Width = poDatasetR->GetRasterXSize(); // 获取图像宽
    int Ori_Height = poDatasetR->GetRasterYSize(); // 获取图像高
    int Ori_Band = poDatasetR->GetRasterCount(); // 获取图像波段数

    // 获取重采样比例
    float Height_Scale, Width_Scale;
    Width_Scale = (float)Ori_Width / (float)Resample_Width;
    Height_Scale = (float)Ori_Height / (float)Resample_Height;

    // 写出新图像文件
    // 创建输出文件的GDAL操作句柄
    GDALDriver* pDriver = NULL;
    pDriver = GetGDALDriverManager()->GetDriverByName("GTIFF");
    GDALDataset* poDatasetW = pDriver->Create(FilePathOut, Resample_Width, Resample_Height, Ori_Band, GDT_Byte, NULL);
    if(!poDatasetW) {
        cout << "ERROR:Create image failed";
        return false;
    }
    GDALRasterBand* poBandW;

    // 读取原图的投影字符串，并赋值给新图像
    const char * strProjectionInfo;
    strProjectionInfo = poDatasetR->GetProjectionRef();
    poDatasetW->SetProjection(strProjectionInfo);

    // 计算新的仿射变换系数，并赋值给新图像
    double geo_transform[6];
    poDatasetR->GetGeoTransform(geo_transform); // 获取原始图像的仿射变换参数
    double new_geo_transform[6]; // 新建图像仿射变换系数
    double x_offset = 0.5 * geo_transform[1] - 0.5 * geo_transform[1] * Width_Scale; // 计算x坐标偏移量
    double y_offset = 0.5 * geo_transform[1] - 0.5 * geo_transform[5] * Height_Scale; // 计算y坐标偏移量
    new_geo_transform[0] = geo_transform[0] + x_offset; // 左上角X坐标
    new_geo_transform[1] = geo_transform[1] * Width_Scale; // 像素宽度
    new_geo_transform[2] = geo_transform[2]; // 选转参数
    new_geo_transform[3] = geo_transform[3] + y_offset; // 左上角Y坐标
    new_geo_transform[4] = geo_transform[4]; // 旋转参数

    // 开辟内存空间（大小均为单一波段）
    unsigned char * ResampleData = NULL;
    unsigned char * OriData = NULL;
    ResampleData = new unsigned char[Resample_Height * Resample_Width];
    if(!ResampleData) {
        cout << "ERROR: allocate ResampleData failed";
        return false;
    }
    OriData = new unsigned char[Ori_Height * Ori_Width];
    if(!OriData) {
        cout << "ERROR: allocate OriData failed";
        return false;
    }

    // 双线性插值
    for(int b = 1; b <= Ori_Band; b++){
        poBand = poDatasetR->GetRasterBand(b);
        poBand->RasterIO(GF_Read, 0, 0, Ori_Width, Ori_Height, OriData, Ori_Width, Ori_Height, GDT_Byte, 0, 0);

        for(int h = 0; h < Resample_Height; h++){
            for(int w = 0; w < Resample_Width; w++){
                // 计算在原图上的浮点坐标
                float srcX = w * Width_Scale;
                float srcY = h * Height_Scale;
                
                // 找到左上角的整数坐标
                int x = (int)srcX;
                int y = (int)srcY;

                // 获取四个邻近点的值
                unsigned char p1 = OriData[y * Ori_Width + x];         // 左上
                unsigned char p2 = OriData[y * Ori_Width + (x + 1)];   // 右上
                unsigned char p3 = OriData[(y + 1) * Ori_Width + x];   // 左下
                unsigned char p4 = OriData[(y + 1) * Ori_Width + (x + 1)]; // 右下

                // 计算插值权重
                float dx = srcX - x;
                float dy = srcY - y;

                // 双线性插值公式
                float pixelValue = (1 - dx) * (1 - dy) * p1 +
                                   dx * (1 - dy) * p2 +
                                   (1 - dx) * dy * p3 +
                                   dx * dy * p4;

                // 将结果写入重采样数据
                ResampleData[h * Resample_Width + w] = (unsigned char)pixelValue;
            }
        }

        // 逐波段写出重采样数据
        poBandW = poDatasetW->GetRasterBand(b);
        poBandW->RasterIO(GF_Write, 0, 0, Resample_Width, Resample_Height, ResampleData, Resample_Width, Resample_Height, GDT_Byte, 0, 0);
    }
    // 释放句柄及内存
    delete[] ResampleData;
    ResampleData = NULL;
    delete[] OriData;
    OriData = NULL;
    GDALClose(poDatasetR);
    GDALClose(poDatasetW);

    cout << "Bilinear Interpolation Done!\n";
    return true;
}

int main(){
    // 初始化gdal
    GDALAllRegister();

    // 使gdal不默认使用UTF-8编码，以支持中文路径
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    // 定义文件路径和重采样参数
    string filePathIn = "Input_file_path";  // 替换为实际输入影像的路径
    string filePathOut = "Output_file_path";  // 替换为实际输出影像的路径
    int resample_width = 800;   // 重采样后的宽度，替换为实际值
    int resample_height = 2000;   // 重采样后的高度，替换为实际值

    // 调用最近邻重采样法
    bool result = NearestNeighborInterpolation(filePathIn.c_str(), filePathOut.c_str(), resample_width, resample_height);

    // 调用双线性插值，使用下面的代码
    // result = BilinearInterpolation(filePathIn.c_str(), filePathOut.c_str(), resample_width, resample_height);

    if (result) {
        cout << "重采样成功" << endl;
        return 0;
    } else {
        cout << "重采样失败" << endl;
        return 1;
    }
}