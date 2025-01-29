#include <string>
#include <vector>
#include <iostream>
#include <stdio.h>
#include <gdal.h>
#include <gdal_priv.h>

#pragma comment(lib, "gdal/lib/gdal_i.lib")
#define PI 3.14159265
#define pixelDepth 255

using namespace std;

float GetMin(float x, float y, float z);
void RGB_HIS(float r, float g, float b, float &h, float &i, float &s);
void HIS_RGB(float h, float i, float s, float &r, float &g, float &b);
void ComputeMaxAndMin(GDALDataset *dataset, int iband, float &max, float &min);

float GetMin(float x, float y, float z)
{
    float min = x;
    if (min > y) min = y;
    if (min > z) min = z;
    return min;
}

void RGB_HIS(float r, float g, float b, float &h, float &i, float &s)
{
    float R, G, B;
    float angle(0);

    // 归一化
    R = r / pixelDepth;
    G = g / pixelDepth;
    B = b / pixelDepth;

    // 三角形变换
    i = (R + G + B) / 3.0;
    s = 1 - GetMin(R, G, B) / i;
    angle = acos(0.5 * (R - G + R - B) / (sqrt((R - G) * (R - G) + (R - B) * (G - B))));
    angle = angle * 180.0 / PI;
    if (B > G) {
        h = 360 - angle;
    } else {
        h = angle;
    }

    h *= pixelDepth / 360.0;
    s *= pixelDepth;
}

void HIS_RGB(float h, float i, float s, float &r, float &g, float &b)
{
    h *= 360.0 / pixelDepth;
    i /= pixelDepth;
    s /= pixelDepth;
    float m[3];

    if (h < 0) {
        h += 360;
    }

    if (h >= 0 && h < 120) {
        m[0] = i * (1.0 + ((s * cos(h * PI / 180)) / cos((60 - h) * PI / 180)));
        m[2] = i * (1.0 - s);
        m[1] = 3.0 * i - (m[0] + m[2]);
    }

    if (h >= 120 && h < 240) {
        h -= 120;
        m[1] = i * (1.0 + ((s * cos(h * PI / 180)) / cos((60 - h) * PI / 180)));
        m[0] = i * (1.0 - s);
        m[2] = 3.0 * i - (m[0] + m[1]);
    }

    if (h >= 240 && h <= 360) {
        h -= 240;
        m[2] = i * (1.0 + ((s * cos(h * PI / 180)) / cos((60 - h) * PI / 180)));
        m[1] = i * (1.0 - s);
        m[0] = 3 * i - (m[1] + m[2]);
    }

    r = m[0] * pixelDepth;
    g = m[1] * pixelDepth;
    b = m[2] * pixelDepth;
}

void ComputeMaxAndMin(GDALDataset *dataset, int iband, float &max, float &min)
{
    int col = dataset->GetRasterXSize();
    int row = dataset->GetRasterYSize();
    GDALRasterBand *band = dataset->GetRasterBand(iband);

    float *p = new float[col];
    // 先取一个点的值
    band->RasterIO(GF_Read, 0, 0, 1, 1, &max, 1, 1, GDT_Float32, 0, 0);
    min = max;

    for (int i = 0; i < row; i++) {
        band->RasterIO(GF_Read, 0, i, col, 1, p, col, 1, GDT_Float32, 0, 0);
        for (int j = 0; j < col; j++) {
            float temp = p[j];
            if (temp > max) max = temp;
            if (temp < min) min = temp;
        }
    }

    delete[] p;
}

int main()
{
    GDALAllRegister();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    // ms是需要得到全色的多光谱文件, 在ENVI中实现
    const char *msPath = "/Users/shiqiyang/Files/Codes/RSI-fundation/lab7/data/MSSResample.tif";
    const char *panPath = "/Users/shiqiyang/Files/Codes/RSI-fundation/lab7/data/PAN.tif";
    const char *outPath = "/Users/shiqiyang/Files/Codes/RSI-fundation/lab7/data/result.tif";

    // 打开RGB多光谱像素(MSSResample)与pan全色影像文件
    GDALDataset *poSrcMss = (GDALDataset*)GDALOpen(msPath, GA_ReadOnly);
    GDALDataset *poSrcPan = (GDALDataset*)GDALOpen(panPath, GA_ReadOnly);
    if (poSrcMss == NULL || poSrcPan == NULL) {
        cout << "文件无法打开！" << endl;
        return 1;
    }

    GDALDriver *poDriver = GetGDALDriverManager()->GetDriverByName("GTiff");
    GDALRasterBand *pBand1 = poSrcMss->GetRasterBand(1);
    GDALRasterBand *pBand2 = poSrcMss->GetRasterBand(2);
    GDALRasterBand *pBand3 = poSrcMss->GetRasterBand(3);
    GDALRasterBand *pBandPan = poSrcPan->GetRasterBand(1);

    if (pBand1 == NULL || pBand2 == NULL || pBand3 == NULL || pBandPan == NULL) {
        GDALClose((GDALDataset*)poSrcMss);
        GDALClose((GDALDataset*)poSrcPan);
        cout << "打开波段失败!" << endl;
        return 1;
    }

    // 恒定定义要操作的各色带的波谱影像尺寸一致
    int panXSize = poSrcPan->GetRasterXSize();
    int panYSize = poSrcPan->GetRasterYSize();

    // 计算各波段的最值
    float MMax, MMin;
    float RMax, RMin, GMax, GMin, BMax, BMin;
    ComputeMaxAndMin(poSrcMss, 1, RMax, RMin);
    ComputeMaxAndMin(poSrcMss, 2, GMax, GMin);
    ComputeMaxAndMin(poSrcMss, 3, BMax, BMin);
    ComputeMaxAndMin(poSrcPan, 1, MMax, MMin);

    GDALDataset *pwDataset = poDriver->Create(outPath, panXSize, panYSize, 3, GDT_Float32, NULL);
    GDALRasterBand *pwBandR = pwDataset->GetRasterBand(3);
    GDALRasterBand *pwBandG = pwDataset->GetRasterBand(2);
    GDALRasterBand *pwBandB = pwDataset->GetRasterBand(1);

    float *R = new float[panXSize];
    float *G = new float[panXSize];
    float *B = new float[panXSize];
    float *H = new float[panXSize];
    float *I = new float[panXSize];
    float *S = new float[panXSize];
    float *panI = new float[panXSize];

    if (R == NULL || G == NULL || B == NULL || H == NULL || I == NULL || S == NULL || panI == NULL) {
        cout << "申请内存失败。" << endl;
        return 1;
    }

    // 此区域需要自己实现
    for (int i = 0; i < panYSize; i++) {
        // 1. 逐行读取数据
        pBand1->RasterIO(GF_Read, 0, i, panXSize, 1, R, panXSize, 1, GDT_Float32, 0, 0);
        pBand2->RasterIO(GF_Read, 0, i, panXSize, 1, G, panXSize, 1, GDT_Float32, 0, 0);
        pBand3->RasterIO(GF_Read, 0, i, panXSize, 1, B, panXSize, 1, GDT_Float32, 0, 0);
        pBandPan->RasterIO(GF_Read, 0, i, panXSize, 1, panI, panXSize, 1, GDT_Float32, 0, 0);

        for (int j = 0; j < panXSize; j++) {
            // 2. 对pan和RGB数据做归一化处理
            float pI = (panI[j] - MMin) * pixelDepth / (MMax - MMin);
            R[j] = (R[j] - RMin) * pixelDepth / (RMax - RMin);
            G[j] = (G[j] - GMin) * pixelDepth / (GMax - GMin);
            B[j] = (B[j] - BMin) * pixelDepth / (BMax - BMin);

            // 3. RGB转HIS
            RGB_HIS(R[j], G[j], B[j], H[j], I[j], S[j]);

            // 4. I分量替换后，HIS转RGB
            HIS_RGB(H[j], pI, S[j], R[j], G[j], B[j]);
        }

        // 5. 逐行写入数据
        pwBandR->RasterIO(GF_Write, 0, i, panXSize, 1, R, panXSize, 1, GDT_Float32, 0, 0);
        pwBandG->RasterIO(GF_Write, 0, i, panXSize, 1, G, panXSize, 1, GDT_Float32, 0, 0);
        pwBandB->RasterIO(GF_Write, 0, i, panXSize, 1, B, panXSize, 1, GDT_Float32, 0, 0);
    }

    GDALClose(poSrcMss);
    GDALClose(poSrcPan);
    GDALClose(pwDataset);
    delete[] R;
    delete[] G;
    delete[] B;
    delete[] H;
    delete[] I;
    delete[] S;
    delete[] panI;

    cout << "融合成功" << endl;
    return 0;
}
