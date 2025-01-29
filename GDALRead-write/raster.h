#include <string>
#include <vector>
#include <iostream>
#include <stdio.h>
#include <gdal.h>
#include <gdal_priv.h>

using namespace std;

class myRasterIO {
public:
    myRasterIO() {
        GDALAllRegister();
        CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");
        CPLSetConfigOption("SHAPE_ENCODING", "");
    }

    ~myRasterIO() {
        if (inputDataset != nullptr) {
            GDALClose(inputDataset);
        }
        if (outputDataset != nullptr) {
            GDALClose(outputDataset);
        }
    }

    void openRaster(const string& inputPath); // 打开文件

    void createNewRaster(const string& outputPath, int width, int height, const char* projection); // 按照输入的基本信息创建的文件

    void copyRaster(GDALDataset *inputDataset,  GDALDataset *outputDataset, int bandNum); // 把输入路径文件的第bandNum个波段复制到输出路径文件

    int getWidth() const { return width; }
    int getHeight() const { return height; }
    GDALDataset *getInputDataset() const { return inputDataset; }
    GDALDataset *getOutputDataset() const { return outputDataset; }
    const char* getProjection() const { return projection; } 

private:
    GDALDataset *inputDataset = nullptr;
    GDALDataset *outputDataset = nullptr;
    int width = 1;
    int height = 1;
    const char* projection;
};