#include <string>
#include <vector>
#include <iostream>
#include <stdio.h>
#include <gdal.h>
#include <gdal_priv.h>
#include <cmath>

using namespace std;

int main() {
    GDALAllRegister();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    // 输入输出路径
    const char* InPath = "/Users/shiqiyang/Files/Codes/RSI-fundation/lab9/data/GF1.tif";
    const char* OutPath = "/Users/shiqiyang/Files/Codes/RSI-fundation/lab9/data/result.tif";

    // 类别数
    int ClassNum = 12;
    // 最大迭代次数
    int MaxIterateTime = 10;
    // 像元变化比率
    float ChangRatio = 0.05;
    // 类中心偏移阈值
    float ThresholdValue = 0.0005;

    // 读取输入文件
    GDALDataset* poIn = (GDALDataset*)GDALOpen(InPath, GA_ReadOnly);
    if (poIn == NULL) { cout << "文件打开失败!" << endl; return 1; }
    GDALDriver* poDriver = GetGDALDriverManager()->GetDriverByName("GTiff");
    if (poDriver == NULL) { GDALClose((GDALDatasetH)poIn); cout << "驱动运行失败!" << endl; return 1; }

    // 图像基本参数的获取
    int XSize = poIn->GetRasterXSize();
    int YSize = poIn->GetRasterYSize();
    int BandNum = poIn->GetRasterCount();
    const char* strDescription = poIn->GetDriver()->GetDescription();
    GDALDataType type = poIn->GetRasterBand(1)->GetRasterDataType();
    int ImgSize = XSize * YSize;
    cout << "X: " << XSize << " Y: " << YSize << " BandNum: " << BandNum << endl;
    if (type == GDT_UInt32) { cout << "数据类型: " << "UInt32" << endl; }

    // 输出文件的创建以及影像信息的写入
    GDALDataset* poOut = poDriver->Create(OutPath, XSize, YSize, 1, GDT_Int16, NULL);
    GDALRasterBand* pOut = poOut->GetRasterBand(1);
    if (pOut == NULL) { cout << "写出文件创建失败!" << endl; return 1; }
    double GeoTrans[6] = { 0.0 };
    poIn->GetGeoTransform(GeoTrans);
    poOut->SetGeoTransform(GeoTrans);
    poOut->SetProjection(poIn->GetProjectionRef());

    // 待分类数据组
    unsigned int* InData = new unsigned int[XSize * YSize * BandNum];
    if (InData == NULL) { cout << "创建三维数据数组失败!" << endl; return 1; }
    memset(InData, 0, XSize * YSize * BandNum * sizeof(unsigned int));

    // 图像数据读入
    GDALRasterBand* poBand;
    for (int i = 1; i <= BandNum; i++) {
        poBand = poIn->GetRasterBand(i);
        poBand->RasterIO(GF_Read, 0, 0, XSize, YSize, InData + (i - 1) * XSize * YSize, XSize, YSize, GDT_UInt32, 0, 0);
    }

    // 类中心数组
    double* Center = new double[4 * ClassNum]; if (!Center) { cout << "创建类中心数组失败!" << endl; return 1; }
    memset(Center, 0, 4 * ClassNum * sizeof(double));

    // 下一次分类计算类中心
    double* NextCenter = new double[4 * ClassNum]; if (!NextCenter) { cout << "创建统计类中心数组失败!" << endl; return 1; }
    memset(NextCenter, 0, 4 * ClassNum * sizeof(double));

    // 统计每个类的像元数
    int* PixelNum = new int[ClassNum]; if (!PixelNum) { cout << "创建统计像元数数组失败!" << endl; return 1; }
    memset(PixelNum, 0, ClassNum * sizeof(int));

    // 类数据信息
    short* Classify = new short[XSize * YSize]; if (!Classify) { cout << "创建分类数据信息数组失败!" << endl; return 1; }
    memset(Classify, -1, XSize * YSize * sizeof(short)); // 初始化值设为-1，之后随机值范围0至ClassNum-1

    // 随机数生成
    struct tm* ptr;
    time_t lt;
    lt = time(NULL); srand((unsigned)lt);
    int off;
    for (int i = 0; i < ClassNum; i++) {
        int ix = rand() % XSize;
        int iy = rand() % YSize;
        cout << "ix: " << ix << " iy: " << iy << endl;
        off = iy * XSize + ix;
        Center[4 * i + 0] = InData[off];
        Center[4 * i + 1] = InData[1 * ImgSize + off];
        Center[4 * i + 2] = InData[2 * ImgSize + off];
        Center[4 * i + 3] = InData[3 * ImgSize + off];
    }

    int flag; double mindis, curdis;
    unsigned int x1, x2, x3, x4;
    double y1, y2, y3, y4;
    double m1, m2, m3, m4;
    double n1, n2, n3, n4;
    double distance = 1.0;

    // 迭代次数
    int mm = 0;
    // 像元变化率
    float nn = 1.0;

    // K-Means分类
    while (mm < MaxIterateTime && nn > ChangRatio) {
        // 清空上一次数据，为此次做准备
        memset(NextCenter, 0, 4 * ClassNum * sizeof(double));
        memset(PixelNum, 0, ClassNum * sizeof(int));
        int count = 0; // 类中心变化距离小于阈值的类累计次数
        int PixelChangeNum = 0; // 变化像元数量
        for (int i = 0; i < YSize; i++) {
            for (int j = 0; j < XSize; j++) {
                off = i * XSize + j;

                // 像元点在空间中的坐标
                x1 = InData[off];
                x2 = InData[ImgSize + off];
                x3 = InData[2 * ImgSize + off];
                x4 = InData[3 * ImgSize + off];

                mindis = sqrt((Center[0] - x1) * (Center[0] - x1) + (Center[1] - x2) * (Center[1] - x2) + (Center[2] - x3) * (Center[2] - x3) + (Center[3] - x4) * (Center[3] - x4));
                flag = 0;

                for (int k = 1; k < ClassNum; k++) {
                    // 类中心的位置
                    y1 = Center[4 * k];
                    y2 = Center[4 * k + 1];
                    y3 = Center[4 * k + 2];
                    y4 = Center[4 * k + 3];

                    // 计算相对于中心的距离，并归类
                    curdis = sqrt((y1 - x1) * (y1 - x1) + (y2 - x2) * (y2 - x2) + (y3 - x3) * (y3 - x3) + (y4 - x4) * (y4 - x4));
                    if (curdis < mindis) {
                        mindis = curdis;
                        flag = k;
                    }
                }

                // 分类图像，若被分至新的类则变化像元数+1
                if (Classify[off] != flag) {
                    Classify[off] = flag;
                    PixelChangeNum++;
                }

                // 为计算下一个类中心准备
                NextCenter[4 * flag] += x1;
                NextCenter[4 * flag + 1] += x2;
                NextCenter[4 * flag + 2] += x3;
                NextCenter[4 * flag + 3] += x4;
                PixelNum[flag]++;
            }
        }
        // 遍历所有像元，计算新的类中心
        nn = (float)PixelChangeNum / ImgSize;
        for (int k = 0; k < ClassNum; k++) {
            // 计算每个类中心的均值
            m1 = NextCenter[4 * k] / PixelNum[k];
            m2 = NextCenter[4 * k + 1] / PixelNum[k];
            m3 = NextCenter[4 * k + 2] / PixelNum[k];
            m4 = NextCenter[4 * k + 3] / PixelNum[k];

            // 类中心发生偏移
            n1 = m1 - Center[4 * k];
            n2 = m2 - Center[4 * k + 1];
            n3 = m3 - Center[4 * k + 2];
            n4 = m4 - Center[4 * k + 3];

            // 更新类中心
            Center[4 * k] = m1;
            Center[4 * k + 1] = m2;
            Center[4 * k + 2] = m3;
            Center[4 * k + 3] = m4;

            // 判断是否满足停止条件
            distance = sqrt(n1 * n1 + n2 * n2 + n3 * n3 + n4 * n4);
            if (distance < ThresholdValue) {
                count++;
            }
        }
        mm++;
        // 如果所有类中心的变化都小于阈值，则停止迭代
        if (count == ClassNum) {
            break;
        }
    }

    // 将分类结果写入输出文件
    pOut->RasterIO(GF_Write, 0, 0, XSize, YSize, Classify, XSize, YSize, GDT_Int16, 0, 0);

    // 清理内存
    delete[] InData;
    delete[] Center;
    delete[] NextCenter;
    delete[] PixelNum;
    delete[] Classify;

    // 关闭文件
    GDALClose((GDALDatasetH)poIn);
    GDALClose((GDALDatasetH)poOut);

    cout << "分类完成!" << endl;
    return 0;
}