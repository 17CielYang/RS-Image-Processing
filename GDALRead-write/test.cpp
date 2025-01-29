#include "raster.h"
#include "vector.h"
#include<iostream>

using namespace std;

int main() {
    // 栅格数据的读取
    string RasterInput = "../../../data/GF1_SX_MSS.tif"; /* 注意：需要根据自己的文件路径修改 */ 
    string RasterOutput = "../../../output/newRaster.tif";

    myRasterIO rasterIO;
    rasterIO.openRaster(RasterInput);
    rasterIO.createNewRaster(RasterOutput, rasterIO.getWidth(), rasterIO.getHeight(), rasterIO.getProjection()); // 新建图像B，与图像A基本信息一致（但是只要1个波段）
    rasterIO.copyRaster(rasterIO.getInputDataset(), rasterIO.getOutputDataset(), 3); // 将图像A的第3个波段内容，复制给图像B（从A中读数据，向B写数据）

    // 矢量数据的读取
    string VectorInput = "../../../data/zj_Polygon.shp"; /* 注意：需要根据自己的文件路径修改 */ 
    string VectorOutput = "../../../output";
    myVectorIO vectorIO;

    vectorIO.OpenFile(VectorInput);
    vectorIO.ReadObject(); 
    vectorIO.CreateFile(VectorOutput);
    vectorIO.copyObject(VectorInput, VectorOutput, 100); // 复制前100个对象

    return 0;
}
