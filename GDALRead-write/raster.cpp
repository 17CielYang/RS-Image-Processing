#include "raster.h"

using namespace std;

void myRasterIO::openRaster(const string& inputPath) {
    inputDataset = (GDALDataset *)GDALOpen(inputPath.c_str(), GA_ReadOnly); // 以只读方式打开图像，以指针形式存储
    if (inputDataset == nullptr) {
        cerr << "Failed to open the input dataset." << endl;
        return;
    }
    width = inputDataset->GetRasterXSize(); // 把图像的基本信息存在类成员变量中
    height = inputDataset->GetRasterYSize();
    projection = inputDataset->GetProjectionRef(); // 获取投影信息
    cout << "Image opened successfully.\nWidth: " << width << ", Height: " << height << "\nProjection: "<< projection << endl; 
}

void myRasterIO::createNewRaster(const string& outputPath, int width, int height, const char* projection) {
    GDALDriver *driver = GetGDALDriverManager()->GetDriverByName("GTIFF"); // 从GDAL驱动管理器中获取GTIFF驱动
    outputDataset = driver->Create(outputPath.c_str(), width, height, 1, GDT_UInt16, NULL); // 用获取到的驱动创建一个新的数据集
    if (outputDataset == nullptr) {
        cerr << "Failed to create the output dataset." << endl;
        return;
    }
    cout << "Output image created successfully." << endl;
}

void myRasterIO::copyRaster(GDALDataset *inputDataset,  GDALDataset *outputDataset, int bandNum) {
    if (inputDataset == nullptr || outputDataset == nullptr) {
        cerr << "Datasets are not properly initialized." << endl;
        return;
    }
    GDALRasterBand *inputBand = inputDataset->GetRasterBand(bandNum);
    GDALRasterBand *outputBand = outputDataset->GetRasterBand(1);
    vector<GUInt16> data(width * height); // 用vector管理图像内容，大小与图像A保持一致
    inputBand->RasterIO(GF_Read, 0, 0, width, height, 
                        &data[0], width, height, GDT_UInt16, 0, 0);
    outputBand->RasterIO(GF_Write, 0, 0, width, height, 
                            &data[0], width, height, GDT_UInt16, 0, 0);
    cout << "Band data copied successfully from input to output." << endl;
}