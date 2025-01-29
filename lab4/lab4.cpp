#include <string>
#include <vector>
#include <iostream>
#include <stdio.h>
#include <gdal.h>
#include <gdal_priv.h>

using namespace std;

double* CalculateMean(double** oriData, int oriWidth, int oriHeight, int oriBand); // 计算各个波段的平均值
double** CalculateCovarMat(double** oriData, double* mean, int oriWidth, int oriHeight, int oriBand);
double** JacobiFunc(double **covaData, int oriBand);
double** MultipleMat(double** eig, double** oriData, double* mean, int oriWidth, int oriHeight, int oriBand);

double* CalculateMean(double** oriData, int oriWidth, int oriHeight, int oriBand){
    double* mean = new double[oriBand](); // 建立数组并初始化为0
    for(int b = 0; b < oriBand; b++){
        for(int i = 0; i < oriWidth * oriHeight; i++){
            mean[b] += oriData[b][i];
        }
        mean[b] /= (oriWidth * oriHeight);
    }
    return mean;
}

double** CalculateCovarMat(double** oriData, double* mean, int oriWidth, int oriHeight, int oriBand){
    // 给转置矩阵、协方差矩阵开辟空间
    double** covaData = new double*[oriBand]();
    for(int i = 0; i < oriBand; i++){
        covaData[i] = new double[oriBand]();
    }

    // 减去各个波段的均值，同时生成转置矩阵
    for(int b = 0; b < oriBand; b++){
        for(int i = 0; i < oriWidth * oriHeight; i++){
            oriData[b][i] -= mean[b];
        }
    }

    // 进行矩阵乘法计算协方差矩阵
    for(int i = 0; i < oriBand; i++){
        for(int j = 0; j < oriBand; j++){
            for(int k = 0; k < oriWidth * oriHeight; k++){
                covaData[i][j] += (oriData[i][k] * oriData[j][k]);
            }
            covaData[i][j] /= (oriWidth * oriHeight);
        }
    }

    return covaData;
}

double** JacobiFunc(double **covaData, int oriBand){
    // 为特征向量V开辟新的存储空间
    double** eigVector = new double*[oriBand]();
    for(int i = 0; i < oriBand; i++){
        eigVector[i] = new double[oriBand]();
    }

    // 初始化特征向量为对角阵，即主对角线的元素都是1，其他元素为0
    for(int i = 0; i < oriBand; i++){
        for(int j = 0; j < oriBand; j++){
            eigVector[i][j] = (i == j)? 1 : 0;
        }
    }

    int count = 0;
    double eps = 1e-5;
    int lim = 20000;
    while(1){
        // 统计迭代次数
        count++;
        // 在非主对角线元素中找到绝对值最大的元素，并记录行列号
        double maxVal = 0;
        int row_id = 0, col_id = 0;
        for(int i = 0; i < oriBand; i++){
            for(int j = i+1; j < oriBand; j++){
                double absVal = abs(covaData[i][j]);
                if(absVal > maxVal){
                    maxVal = absVal;
                    row_id = i;
                    col_id = j;
                }
            }
        }
        // 超过迭代次数上限或小于阈值
        if(maxVal < eps || count > lim) break;

        // 进行旋转变换，计算theta
        int p = row_id; 
        int q = col_id;
        double Apq = covaData[p][q];
        double App = covaData[p][p];
        double Aqq = covaData[q][q];
        double theta = 0.5 * atan2(-2.0 * Apq, Aqq - App);
        double sint = sin(theta);
        double cost = cos(theta);
        double sin2t = sin(2.0 * theta);
        double cos2t = cos(2.0 * theta);

        // 代入公式修改协方差矩阵的值
        covaData[p][p] = App * cost * cost + Aqq * sint * sint + 2.0 * Apq * cost * sint;
        covaData[q][q] = App * sint * sint + Aqq * cost * cost - 2.0 * Apq * cost * sint;
        covaData[p][q] = covaData[q][p] = 0.5 * (Aqq - App) * sin2t + Apq * cos2t;
        for(int i = 0; i < oriBand; i++){
            if(i != p && i != q){
                double u = covaData[p][i];
                double v = covaData[q][i];
                covaData[p][i] = u * cost + v * sint;
                covaData[q][i] = u * sint - v * cost;

                u = covaData[i][p];
                v = covaData[i][q];
                covaData[i][p] = u * cost + v * sint;
                covaData[i][q] = u * sint - v * cost;
            }
        }

        // 计算特征向量
        for(int i = 0; i < oriBand; i++){
            double u = eigVector[i][p];
            double v = eigVector[i][q];
            eigVector[i][p] = u * cost + v * sint;
            eigVector[i][q] = u * sint - v * cost;
        }

        // 上述计算得到的特征向量为列向量，转置将其变为行向量
        double tmpData1, tmpData2; // 作为中间临时变量
        for(int i = 0; i < oriBand; i++){
            for(int j = i + 1; j < oriBand; j++){
                tmpData1 = eigVector[i][j];
                eigVector[i][j] = eigVector[j][i];
                eigVector[j][i] = tmpData1;
            }
        }

        // 根据特征值的大小从大到小的顺序重新排列矩阵的特征值和特征向量（按行排列）
        int ij;
        for(int i = 0; i < oriBand - 1; i++){
            tmpData1 = covaData[i][i]; // 矩阵A的对角线值（i,i）
            ij = i;
            for(int j = i + 1; j < oriBand; j++){
                tmpData2 = covaData[j][j];
                if(tmpData2 <= tmpData1) continue;
                tmpData1 = tmpData2; // v1记录最大的特征向量
                ij = j; // ij记录该特征向量的行列号
            }
            if(ij = i) continue;
            for(int k = 0; k < oriBand; k++){  // 重新排列特征值和特征向量的顺序
                tmpData1 = covaData[i][k];
                covaData[i][k] = covaData[ij][k];
                covaData[ij][k] = tmpData1;

                tmpData1 = eigVector[i][k];
                eigVector[i][k] = eigVector[ij][k];
                covaData[ij][k] = tmpData1;

                tmpData1 = eigVector[i][k];
                eigVector[i][k] = eigVector[ij][k];
                eigVector[ij][k] = tmpData1;
            }
            for(int k = 0; k < oriBand; k++){
                tmpData1 = covaData[k][i];
                covaData[k][i] = covaData[k][ij];
                covaData[k][ij] = tmpData1;
            }
        }
        return eigVector;
    }
}

double** MultipleMat(double** eig, double** oriData, double* mean, int oriWidth, int oriHeight, int oriBand){
    // 减去各个波段的均值
    for(int b = 0; b < oriBand; b++){
        for(int i = 0; i < oriWidth * oriHeight; i++){
            oriData[b][i] -= mean[b];
        }
    }

    double** resultMat = new double*[oriBand]();
    for(int i = 0; i < oriBand; i++){
        resultMat[i] = new double[oriHeight * oriWidth]();
    }

    for(int i = 0; i < oriBand; i++){
        for(int j = 0; j < oriHeight * oriWidth; j++){
            for(int k = 0; k < oriBand; k++){
                resultMat[i][j] += eig[i][k] * oriData[k][j];
            }
        }
    }

    return resultMat;
}

int main(){
    // 初始化gdal
    GDALAllRegister();
    // 使gdal不默认使用UTF-8编码，以支持中文路径
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    // 获取输入输出文件的路径
    const char * FilePathIn = "";
    const char * FilePathOut = "";

    // 定义GDAL操作句柄
    GDALDataset * poDataset; // 指向对象数据的指针
    GDALRasterBand * poBand; // 指向对象数据单个波段的指针
    poDataset = (GDALDataset*)GDALOpen(FilePathIn, GA_ReadOnly);
    if(!poDataset){
        cout << "ERROR:cannot open image file";
        return 1;
    }

    // 获取图像基本信息
    int oriWidth = poDataset->GetRasterXSize(); // 获取图像宽
    int oriHeight = poDataset->GetRasterYSize(); // 获取图像高
    int oriBand = poDataset->GetRasterCount(); // 获取图像波段数

    // 开辟内存空间并读取数据
    double** oriData = new double*[oriBand];
    for(int b = 0; b < oriBand; b++){
        // 开辟原始数据存储空间
        oriData[b] = new double[oriWidth * oriHeight];
        poBand = poDataset->GetRasterBand(b+1);
        // 读取原始影像的波段
        poBand->RasterIO(GF_Read, 0, 0, oriWidth, oriHeight, oriData[b], oriWidth, oriHeight, GDT_Float64, 0, 0);
    }

    cout << "READ IMAGE DONE!" << endl;

    // 计算每个波段的平均值
    double* mean = CalculateMean(oriData, oriWidth, oriHeight, oriBand);
    cout << "CALCULATE MEAN DONE!" << endl;

    // 进行零均值化并计算协方差矩阵
    double** covaData = CalculateCovarMat(oriData, mean, oriWidth, oriHeight, oriBand);
    cout << "CALCULATE COVAMAT DONE!" << endl;

    // 计算特征向量
    double** eigVector = JacobiFunc(covaData, oriBand);
    cout << "CALCULATE ETGVECTOR DONE!" << endl;

    // 计算结果矩阵
    double** resultMat = MultipleMat(eigVector, oriData, mean, oriWidth, oriHeight, oriBand);
    cout << "CALCULATE RESULTMAT DONE!" << endl;

    // 创建输出文件的GDAL操作句柄
    GDALDriver* pDriver = NULL;
    pDriver = GetGDALDriverManager()->GetDriverByName("GTIFF");
    GDALDataset* poDatasetW = pDriver->Create(FilePathOut, oriWidth, oriHeight, oriBand, GDT_Float64, NULL);
    if(!poDatasetW){
        cout << "ERROR: Create image failed";
        return 1;
    }
    GDALRasterBand* poBandW;

    // 读取原图的投影字符串和仿射变换系数，并赋值给新图像
    const char* strProjectionInfo;
    strProjectionInfo = poDataset->GetProjectionRef();
    poDatasetW->SetProjection(strProjectionInfo);
    double arrGeoTransform[6];
    poDataset->GetGeoTransform(arrGeoTransform);
    poDatasetW->SetGeoTransform(arrGeoTransform);

    for(int b = 0; b < oriBand; b++){
        poBandW = poDatasetW->GetRasterBand(b+1);
        // 写出结果影像的波段
        poBandW->RasterIO(GF_Write, 0, 0, oriWidth, oriHeight, resultMat[b], oriWidth, oriHeight, GDT_Float64, 0, 0);
    }

    // 释放句柄及内存
    delete[] oriData, mean, covaData, eigVector, resultMat = NULL;
    GDALClose(poDataset);
    GDALClose(poDatasetW);
    cout << "DONE!" << endl;

    return 0;
}