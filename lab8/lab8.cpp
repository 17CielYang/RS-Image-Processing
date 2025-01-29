#include <string>
#include <vector>
#include <iostream>
#include <stdio.h>
#include <gdal.h>
#include <gdal_priv.h>

using namespace std;

int main(){
    GDALAllRegister();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    //==================
    // 0. 获取参数及有效性检验，参数均可自由修改
    //==================
    // 输入输出文件路径
    const char* strInputFile = "/Users/shiqiyang/Files/Codes/RSI-fundation/lab8/data/GF2.tif";
    const char* strOutputFile = "/Users/shiqiyang/Files/Codes/RSI-fundation/lab8/data/GLCM.txt";

    // 灰度级数
    int glcm_class = 8;

    // 步长
    int glcm_stride = 1;

    // 处理波段，范围为第1波段到第4波段
    int sel_Band = 1;

    // 空间位置关系，范围0~3
    // 0: 东-西方向（0°）
    // 1: 东北-西南方向（45°）
    // 2: 南-北方向（90°）
    // 3: 西北-东南方向（135°）
    int sel_Angle = 0;

    // 有效性检验
    if (glcm_class <= 0 || glcm_class > 256) { cout << "灰度级数太大或为负！"; return 1; }
    if (glcm_stride <= 0 || glcm_stride > 256) { cout << "步长太大或为负！"; return 1; }
    if (sel_Band <= 0 || sel_Band > 4) { cout << "波段数太大或为负！"; return 1; }
    if (sel_Angle < 0 || sel_Angle > 3) { cout << "空间位置关系错误！"; return 1; }


    //==================
    // 1. 读入影像（*.TIF）
    //==================
    // 打开影像，获取影像句柄
    GDALDataset* poDataset;
    poDataset = (GDALDataset*)GDALOpen(strInputFile, GA_ReadOnly);
    if (!poDataset) { cout << "poDataset failed"; return 1; }

    // 获取影像宽高
    int Ori_height = poDataset->GetRasterYSize();
    int Ori_width = poDataset->GetRasterXSize();
    int Ori_band = poDataset->GetRasterCount();

    // 分配数组空间
    unsigned short* pDataOri = new unsigned short[Ori_height * Ori_width];
    if (pDataOri == NULL) { cout << "pDataOri failed"; return 1; }
    memset(pDataOri, 0, (sizeof(unsigned short)) * Ori_height * Ori_width);

    // 读入指定波段数据
    GDALRasterBand* poBand;
    poBand = poDataset->GetRasterBand(sel_Band);
    poBand->RasterIO(GF_Read, 0, 0, Ori_width, Ori_height, pDataOri, Ori_width, Ori_height, GDT_UInt16, 0, 0);

    //==================
    // 2. 计算GLCM
    //==================
    // 创建并初始化灰度共生矩阵数组及共生矩阵数组
    int* glcm = new int[glcm_class * glcm_class];
    if (glcm == NULL) { cout << "glcm failed"; return 1; }
    int* histImage = new int[Ori_width * Ori_height];
    if (histImage == NULL) { cout << "histImage failed"; return 1; }
    memset(histImage, 0, sizeof(int) * Ori_width * Ori_height);
    memset(glcm, 0, sizeof(int) * glcm_class * glcm_class);

    // 获得图像像素值的最大最小值
    double maxV = -INFINITY, minV = INFINITY;
    for (int i = 0; i < Ori_height; i++)
    {
        for (int j = 0; j < Ori_width; j++)
        {
            if (maxV < pDataOri[i * Ori_width + j])
            {
                maxV = pDataOri[i * Ori_width + j];
            }
            if (minV > pDataOri[i * Ori_width + j])
            {
                minV = pDataOri[i * Ori_width + j];
            }
        }
    }

    // 进行灰度分级
    for (int i = 0; i < Ori_height; i++)
    {
        for (int j = 0; j < Ori_width; j++)
        {
            histImage[i * Ori_width + j] = int(floor((pDataOri[Ori_width * i + j] - minV) * (glcm_class - 1) / (double)(maxV - minV)));
        }
    }

    // 计算四个方向的灰度共生矩阵
    int firstpoint = 0, secondpoint = 0;
    switch (sel_Angle)
    {
    case 0:
        // 0: 东-西方向（0°）
        for (int i = 0; i < Ori_height; i++)
        {
            for (int j = 0; j < Ori_width - glcm_stride; j++)
            {
                firstpoint = histImage[i * Ori_width + j];
                secondpoint = histImage[i * Ori_width + j + glcm_stride];
                glcm[firstpoint * glcm_class + secondpoint]++;
            }
        }
        break;
    case 1:
        // 1: 东北-西南方向（45°）
        for (int i = 0; i < Ori_height - glcm_stride; i++)
        {
            for (int j = 0; j < Ori_width - glcm_stride; j++)
            {
                firstpoint = histImage[i * Ori_width + j];
                secondpoint = histImage[(i + glcm_stride) * Ori_width + j + glcm_stride];
                glcm[firstpoint * glcm_class + secondpoint]++;
            }
        }
        break;
    case 2:
        // 2: 南-北方向（90°）
        for (int i = 0; i < Ori_height - glcm_stride; i++)
        {
            for (int j = 0; j < Ori_width; j++)
            {
                firstpoint = histImage[i * Ori_width + j];
                secondpoint = histImage[(i + glcm_stride) * Ori_width + j];
                glcm[firstpoint * glcm_class + secondpoint]++;
            }
        }
        break;
    case 3:
        // 3: 西北-东南方向（135°）
        for (int i = 0; i < Ori_height - glcm_stride; i++)
        {
            for (int j = glcm_stride; j < Ori_width; j++)
            {
                firstpoint = histImage[i * Ori_width + j];
                secondpoint = histImage[(i + glcm_stride) * Ori_width + j - glcm_stride];
                glcm[firstpoint * glcm_class + secondpoint]++;
            }
        }
        break;
    default:
        cout << "空间位置关系选项出错！";
        return 1;
    }
    //==================
    // 3. 输出GLCM(*.TXT)
    //==================
    // txt文件指针
    FILE* file = fopen(strOutputFile, "w");
    if (file == nullptr) {
        std::cout << "Error: Unable to create the text file!" << std::endl;
        return 1;
    }

    // 按行列写入灰度共生矩阵
    for (int i = 0; i < glcm_class; i++)
    {
        for (int j = 0; j < glcm_class; j++)
        {
            fprintf(file, "%d\t", glcm[i * glcm_class + j]);
        }
        fprintf(file, "\n");
    }

    fclose(file);

    cout << "运算成功" << endl;

    // 关闭句柄，释放资源
    GDALClose(poDataset);
    delete[] pDataOri; pDataOri = NULL;
    delete[] histImage; histImage = NULL;
    delete[] glcm; glcm = NULL;

    return 0;
}