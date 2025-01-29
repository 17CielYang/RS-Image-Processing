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

// Function to compute the minimum and maximum class numbers in the dataset
void ComputeClassNum(GDALDataset *dataset, int &max) {
    int col = dataset->GetRasterXSize();
    int row = dataset->GetRasterYSize();
    GDALRasterBand *band = dataset->GetRasterBand(1);

    int *p = new int[col];
    // 先取一个点的值
    band->RasterIO(GF_Read, 0, 0, 1, 1, &max, 1, 1, GDT_Int32, 0, 0);

    for (int i = 0; i < row; i++) {
        band->RasterIO(GF_Read, 0, i, col, 1, p, col, 1, GDT_Int32, 0, 0);
        for (int j = 0; j < col; j++) {
            int temp = p[j];
            if (temp > max) max = temp;
        }
    }
    delete[] p;
}

int main() {
    GDALAllRegister();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    // 输入文件路径
    const char *ImgPath = "/Users/shiqiyang/Files/Codes/RSI-fundation/lab10/data/lc8mlresult.tif";
    const char *ROIPath = "/Users/shiqiyang/Files/Codes/RSI-fundation/lab10/data/lc8testroi.tif";
    // 输出txt路径
    const char *TxtPath = "/Users/shiqiyang/Files/Codes/RSI-fundation/lab10/data/CM_result.txt";

    GDALDataset *poPred = (GDALDataset *)GDALOpen(ImgPath, GA_ReadOnly);
    GDALDataset *poGT = (GDALDataset *)GDALOpen(ROIPath, GA_ReadOnly);

    if (poPred == NULL || poGT == NULL) {
        cout << "文件打开失败！" << endl;
        return 1;
    }

    GDALDriver *poDriver = GetGDALDriverManager()->GetDriverByName("GTiff");
    if (poDriver == NULL) {
        GDALClose((GDALDatasetH)poPred);
        GDALClose((GDALDatasetH)poGT);
        cout << "驱动运行失败！" << endl;
        return 1;
    }

    // 图像基本参数的获取
    int XSize = poPred->GetRasterXSize();
    int YSize = poPred->GetRasterYSize();
    int RXSize = poGT->GetRasterXSize();
    int RYSize = poGT->GetRasterYSize();
    if (XSize != RXSize || YSize != RYSize) {
        cout << "长宽不一致！";
        GDALClose((GDALDatasetH)poPred);
        GDALClose((GDALDatasetH)poGT);
        return 1;
    }

    GDALRasterBand *pBandPred = poPred->GetRasterBand(1);
    GDALRasterBand *pBandGT = poGT->GetRasterBand(1);

    // 获取类别数
    int PredClassNum = 0;
    int GTClassNum = 0;
    ComputeClassNum(poPred, PredClassNum);
    ComputeClassNum(poGT, GTClassNum);
    if (PredClassNum > 10) {
        cout << "类别数过多！";
        GDALClose((GDALDatasetH)poPred);
        GDALClose((GDALDatasetH)poGT);
        return 1;
    }
    if (PredClassNum != GTClassNum){
        cout << "类别数不一致！";
        GDALClose((GDALDatasetH)poPred);
        GDALClose((GDALDatasetH)poGT);
        return 1;
    }

    // 混淆矩阵、生产者精度、用户精度
    int CM[10][10] = { 0 };
    float UA[10] = { 0.0 };
    float PA[10] = { 0.0 }; 

    // 储存逐行读取得到的数据
    int *PredRow = new int[XSize];
    int *GTRow = new int[XSize];

    // 混淆矩阵像元点数量获取
    for (int i = 0; i < YSize; i++) {
        // 逐行读取数据
        if (pBandPred->RasterIO(GF_Read, 0, i, XSize, 1, PredRow, XSize, 1, GDT_Int32, 0, 0) != CE_None) {
            cout << "读取预测数据失败，行 " << i << endl;
            continue;
        }
        if (pBandGT->RasterIO(GF_Read, 0, i, XSize, 1, GTRow, XSize, 1, GDT_Int32, 0, 0) != CE_None) {
            cout << "读取真实数据失败，行 " << i << endl;
            continue;
        }
        for (int j = 0; j < XSize; j++) {
            int pred = PredRow[j];
            int gt = GTRow[j];
            
            // 假设类别编号从1开始
            if (pred > 0 && pred <= PredClassNum && gt > 0 && gt <= GTClassNum) {
                CM[pred - 1][gt - 1]++;
            }
        }
    }

    // 定义手动排序的映射数组
    // mappingPred[i] 表示新的行 i 对应原始的行 mappingPred[i]-1
    // mappingGT[j] 表示新的列 j 对应原始的列 mappingGT[j]-1
    // 注意：映射数组的大小应至少与类别数一致
    int mappingPred[5] = {3, 2, 5, 1, 4}; 
    int mappingGT[5] = {5, 4, 2, 3, 1};  

    // 创建排序后的混淆矩阵
    int sortedCM[10][10] = {0};

    for(int i = 0; i < PredClassNum; i++) {
        for(int j = 0; j < GTClassNum; j++) {
            // 确保映射索引在有效范围内
            if(mappingPred[i]-1 < PredClassNum && mappingGT[j]-1 < GTClassNum && mappingPred[i] > 0 && mappingGT[j] > 0){
                sortedCM[i][j] = CM[mappingPred[i]-1][mappingGT[j]-1];
            }
            else{
                cout << "映射数组索引超出范围！ i: " << i << ", j: " << j << endl;
                sortedCM[i][j] = 0;
            }
        }
    }

    // 将排序后的混淆矩阵复制回原矩阵
    for(int i = 0; i < PredClassNum; i++) {
        for(int j = 0; j < GTClassNum; j++) {
            CM[i][j] = sortedCM[i][j];
        }
    }

    // 计算生产者精度（PA）和用户精度（UA）
    // 生产者精度（PA）：每个真实类别的正确率
    for(int j = 0; j < GTClassNum; j++) {
        int sum = 0;
        for(int i = 0; i < PredClassNum; i++) {
            sum += CM[i][j];
        }
        if(sum > 0)
            PA[j] = ((float)CM[j][j] / sum) * 100.0;
        else
            PA[j] = 0.0;
    }

    // 用户精度（UA）：每个预测类别的正确率
    for(int i = 0; i < PredClassNum; i++) {
        int sum = 0;
        for(int j = 0; j < GTClassNum; j++) {
            sum += CM[i][j];
        }
        if(sum > 0)
            UA[i] = ((float)CM[i][i] / sum) * 100.0;
        else
            UA[i] = 0.0;
    }

    // 输出混淆矩阵和精度到文件
    ofstream out(TxtPath);
    if (!out.is_open()) {
        cout << "无法打开输出文件！" << endl;
        GDALClose((GDALDatasetH)poPred);
        GDALClose((GDALDatasetH)poGT);
        delete[] PredRow;
        delete[] GTRow;
        return 1;
    }

    out << "Class\t\t";
    for (int i = 0; i < PredClassNum; i++) {
        out << "class" << i+1 << "\t\t";
    }
    out << "PA(100%)\n";

    for (int i = 0; i < PredClassNum; i++) {
        out << "class" << i + 1;
        for (int j = 0; j < PredClassNum; j++) {
            out << "\t\t" << CM[i][j];
        }
        out << "\t\t" << PA[i] << "\n";
    }

    // 写入用户精度（UA）
    out << "UA(100%)";
    for (int i = 0; i < GTClassNum; i++) {
        out << "\t" << UA[i];
    }

    out.close();
    GDALClose((GDALDatasetH)poPred);
    GDALClose((GDALDatasetH)poGT);

    cout << "OK" << endl;

    // 释放内存
    delete[] PredRow;
    delete[] GTRow;

    return 0;
}