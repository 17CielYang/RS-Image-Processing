#include <string>
#include <vector>
#include <iostream>
#include <stdio.h>
#include <gdal.h>
#include <gdal_priv.h>
#include <cmath>
#include <fstream>
#include <algorithm>
#include <limits>

using namespace std;

int main() {
    // 注册 GDAL 驱动
    GDALAllRegister();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    // 获取输入输出文件路径
    const char *InPath = "/Users/shiqiyang/Files/Codes/RSI-fundation/lab14/data/kmeans_cut.tif";
    const char *OutPath = "/Users/shiqiyang/Files/Codes/RSI-fundation/lab14/data/kmeans_out.tif";
    const char *StaPath = "/Users/shiqiyang/Files/Codes/RSI-fundation/lab14/data/kmeans_sta.tif";
    
    // 读取输入文件
    GDALDataset *pDataset = (GDALDataset*)GDALOpen(InPath, GA_ReadOnly);
    if (pDataset == NULL)
    {
        cout << "Cannot open the file!";
        return 1;
    }
    int rows = pDataset->GetRasterYSize();
    int cols = pDataset->GetRasterXSize();
    GDALRasterBand* pBand = pDataset->GetRasterBand(1);

    // 分配内存，读取数据
    unsigned char* pData = new unsigned char[rows * cols];
    if (!pData)
    {
        cout << "Cannot allocate memory!";
        return 1;
    }
    unsigned int* pDataOut = new unsigned int[rows * cols];
    if (!pDataOut)
    {
        cout << "Cannot allocate memory!";
        return 1;
    }
    unsigned int* pDataS = new unsigned int[rows * cols];
    if (!pDataS)
    {
        cout << "Cannot allocate memory!";
        return 1;
    }
    
    memset(pData, 0, sizeof(unsigned char) * cols * rows);
    memset(pDataOut, 0, sizeof(unsigned int) * cols * rows);
    memset(pDataS, 0, sizeof(unsigned int) * cols * rows);

    // 读入灰度(或分类)数据到 pData
    pBand->RasterIO(GF_Read, 0, 0, cols, rows, pData, cols, rows, GDT_Byte, 0, 0);

    // 用于给新图斑编号
    long long freeflag = 1;  // 从1开始分配图斑ID，也可以从0或别的数开始

    // 遍历图像，生成初步的结果
    int pos, pos_left, pos_up;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            pos = i * cols + j;
            pos_left = pos - 1;          // 左边像元的位置（同一行，列-1）
            pos_up   = (i - 1) * cols + j; // 上边像元的位置（行-1，同一列）

            // 如果当前像元值==0，可视为背景，无需编号
            if (pData[pos] == 0) {
                pDataOut[pos] = 0;
                continue;
            }

            // TODO: 边界条件的处理
            if (i == 0 || j == 0)
            {
                // 对第一行或第一列（只要没有“上”“左”两邻居同时可用）
                // 这里若像元 != 0，直接赋予新图斑编号
                pDataOut[pos] = freeflag;
                freeflag++;
            }
            else
            {
                // TODO: 非边界条件的处理
                // 分四种情况:
                // 1) 与左边像元不等，且与上边像元不等 -> 新图斑
                // 2) 与左边相等，且与上边不等 -> 合并到左边
                // 3) 与左边不等，且与上边相等 -> 合并到上边
                // 4) 与左边相等，且与上边相等 -> 判断是否同图斑或需合并

                bool same_left = (pData[pos] == pData[pos_left]);
                bool same_up   = (pData[pos] == pData[pos_up]);

                if (!same_left && !same_up)
                {
                    // 新图斑
                    pDataOut[pos] = freeflag;
                    freeflag++;
                }
                else if (same_left && !same_up)
                {
                    // 合并到左边
                    pDataOut[pos] = pDataOut[pos_left];
                }
                else if (!same_left && same_up)
                {
                    // 合并到上边
                    pDataOut[pos] = pDataOut[pos_up];
                }
                else
                {
                    // same_left && same_up
                    // 先判断左、上是否同属一个图斑
                    if (pDataOut[pos_left] == pDataOut[pos_up])
                    {
                        // 左上是同一个图斑，那就直接用这个图斑号
                        pDataOut[pos] = pDataOut[pos_left];
                    }
                    else
                    {
                        // 左上是两个不同图斑，需要“合并”
                        // 简单做法：统一替换 left 区域为 up 区域
                        unsigned int old_label = pDataOut[pos_left];
                        unsigned int new_label = pDataOut[pos_up];
                        
                        // 将整个 old_label 区域都改成 new_label
                        for (int k = 0; k < rows * cols; k++)
                        {
                            if (pDataOut[k] == old_label)
                                pDataOut[k] = new_label;
                        }
                        // 当前像元也归到 new_label
                        pDataOut[pos] = new_label;
                    }
                }
            }
        }
    }

    // 统计各图斑(类别)像元数
    // 这里假设最大可能有 50000 个图斑（可根据实际情况修改）
    int* numofclump = new int[50000];
    memset(numofclump, 0, sizeof(int) * 50000);

    // 逐像元统计
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            // pDataOut[i*cols + j] 就是该像元所属图斑编号
            // 作为数组下标要注意越界，所以保证 freeflag 不超过 50000
            numofclump[pDataOut[i * cols + j]]++;
        }
    }

    // 将每个像元的值更新为其所在图斑的面积（像元数）
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            pDataS[i * cols + j] = numofclump[pDataOut[i * cols + j]];
        }
    }

    // 创建输出图像（图斑编号）
    GDALDataset* poDatasetW = nullptr;
    GDALDataset* poDatasetS = nullptr;
    GDALDriver* pDriver = GetGDALDriverManager()->GetDriverByName("GTiff");
    if (pDriver == NULL) {
        GDALClose((GDALDatasetH)pDataset);
        cout << "Drive operation failed!";
        return 1;
    }

    // 建立输出的图斑编号影像（GDT_UInt32）
    poDatasetW = pDriver->Create(OutPath, cols, rows, 1, GDT_UInt32, NULL);
    if (!poDatasetW) {
        cout << "Cannot create new image file!";
        return 1;
    }
    // 建立输出的面积统计影像（GDT_UInt32）
    poDatasetS = pDriver->Create(StaPath, cols, rows, 1, GDT_UInt32, NULL);
    if (!poDatasetS) {
        cout << "Cannot create new image file!";
        return 1;
    }

    // 读取原图的投影字符串和仿射变换系数，并赋值给新图像
    const char* strProjectionInfo = pDataset->GetProjectionRef();
    poDatasetW->SetProjection(strProjectionInfo);
    poDatasetS->SetProjection(strProjectionInfo);

    double arrGeoTransform[6];
    pDataset->GetGeoTransform(arrGeoTransform);
    poDatasetW->SetGeoTransform(arrGeoTransform);
    poDatasetS->SetGeoTransform(arrGeoTransform);

    // 将结果写入输出影像
    GDALRasterBand* pBandW = poDatasetW->GetRasterBand(1);
    pBandW->RasterIO(GF_Write, 0, 0, cols, rows, pDataOut, cols, rows, GDT_UInt32, 0, 0);

    GDALRasterBand* pBandS = poDatasetS->GetRasterBand(1);
    pBandS->RasterIO(GF_Write, 0, 0, cols, rows, pDataS, cols, rows, GDT_UInt32, 0, 0);

    // 关闭文件，释放句柄
    GDALClose(pDataset);
    GDALClose(poDatasetW);
    GDALClose(poDatasetS);
    delete[] pData;
    delete[] pDataOut;
    delete[] pDataS;
    delete[] numofclump;

    cout << "Done!" << endl;
    return 0;
}