#include <string>
#include <vector>
#include <iostream>
#include <cstdint>
#include <gdal.h>
#include <gdal_priv.h>
#include <ogrsf_frmts.h>

using namespace std;

class myVectorIO {
public:
    myVectorIO() {
        GDALAllRegister();
    }

    void OpenFile(const string& inputPath); // 打开文件

    void CreateFile(const string& outputPath); // 创建文件
    
    void ReadObject(); // 读取矢量对象

    void copyObject(const string& inputPath, const string& outputPath, int itemCount); // 复制前itemCount个对象
    
    ~myVectorIO() {
        if (inputDataset) GDALClose(inputDataset);
        if (outputDataset) GDALClose(outputDataset);
    }

private:
    string inputPath, outputPath;
    GDALDataset *inputDataset = nullptr, *outputDataset = nullptr;
    OGRLayer *srcLayer = nullptr, *destLayer = nullptr;
    OGRSpatialReference *spatialRef = nullptr;
    OGRwkbGeometryType geomType;
    OGRFeature *feature = nullptr;
};