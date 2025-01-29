#include <string>
#include <vector>
#include <iostream>
#include <stdio.h>
#include <gdal.h>
#include <gdal_priv.h>

using namespace std;

bool ReadProjectionInfo(){
    GDALDataset* pDataset = (GDALDataset*)GDALOpen("/Users/shiqiyang/Files/Codes/RSI-fundation/lab1/data/Iknos_Mercator/Iknos_Mercator.tif", GA_ReadOnly);
    if(!pDataset){
        cout << "Open Image Error!!";
        return false;
    }

    // 读取图像中的投影信息
    const char* strProjectionInfo = pDataset->GetProjectionRef();

    // 读取图像中的仿射变换系数（arrGeoTransform用于记录读取的数据；strGeoTransform用于在对话框中显示）
    double arrGeoTransform[6];
    char strGeoTransform[100];
    pDataset->GetGeoTransform(arrGeoTransform);
    printf(strGeoTransform, "%.6f, %.6f, %.6f, %.6f, %.6f, %.6f\n",
        arrGeoTransform[0], 
        arrGeoTransform[1], 
        arrGeoTransform[2],
        arrGeoTransform[3], 
        arrGeoTransform[4], 
        arrGeoTransform[5]);

    printf(strGeoTransform);
    printf(strProjectionInfo);
    GDALClose(pDataset);
    return true;
}

bool WriteProjectionInfo(){
    // 打开原始图像文件
    GDALDataset* pDataset = (GDALDataset *)GDALOpen("/Users/shiqiyang/Files/Codes/RSI-fundation/lab1/data/Iknos_pan.tif", GA_ReadOnly);
    if(!pDataset){
        cout << "Open Image Error!!";
        return false;
    }

    // 建立用于写出TIFF格式文件的驱动
    GDALDriver *pDriver = GetGDALDriverManager()->GetDriverByName("GTIFF");
    if(!pDriver){
        cout << "Get Driver Error!!";
        return false;
    }

    // 在输出路径建立原图像的拷贝
    char outPath[100] = ("/Users/shiqiyang/Files/Codes/RSI-fundation/lab1/output/Iknos_Mercator_output.tif");
    GDALDataset *pOutDataset;
    pOutDataset = pDriver->CreateCopy(outPath, pDataset, FALSE, NULL, NULL, NULL);

    // 读取原图的投影字符串并赋值给新图像
    const char* strProjectionInfo;
    strProjectionInfo = pDataset->GetProjectionRef();
    pOutDataset->SetProjection(strProjectionInfo);

    // 设置新的仿射变换系数并赋值给图像
    double arrGeoTransform[6];
    arrGeoTransform[0] = 100;
    arrGeoTransform[1] = 200;
    arrGeoTransform[2] = 300;
    arrGeoTransform[3] = 400;
    arrGeoTransform[4] = 500;
    arrGeoTransform[5] = 600;
    pOutDataset->SetGeoTransform(arrGeoTransform);

    // 再次读取投影字符串和仿射变换系数，并将其组织成字符串形式显示
    char strGeoTransform[100];
    pOutDataset->GetGeoTransform(arrGeoTransform);
    printf(strGeoTransform);
    printf("%.6f, %.6f, %.6f, %.6f, %.6f, %.6f\n",
        arrGeoTransform[0], 
        arrGeoTransform[1], 
        arrGeoTransform[2],
        arrGeoTransform[3],
        arrGeoTransform[4], 
        arrGeoTransform[5]);
    strProjectionInfo = pOutDataset->GetProjectionRef();
    printf(strGeoTransform);
    printf(strProjectionInfo);

    GDALClose(pDataset);
    GDALClose(pOutDataset);
    cout << "\nWrite Image Done!";
    return true;
}

bool TransformCoordinate(){
    // 打开目标栅格图像
    GDALDataset* pDataset = (GDALDataset *)GDALOpen("/Users/shiqiyang/Files/Codes/RSI-fundation/lab1/data/Iknos_pan.tif", GA_ReadOnly);
    if(!pDataset){
        cout << "Open Image Error!!";
        return false;
    }

    const char *strProjectionInfo = pDataset->GetProjectionRef();
    // 将投影信息转化成WKT模式
    char *ori_SRS_WKT = NULL;
    int length = strlen(strProjectionInfo) + 1;  // 计算包括null终止符的长度
    ori_SRS_WKT = (char *)malloc(length); 
    strncpy(ori_SRS_WKT, strProjectionInfo, length - 1); 
    ori_SRS_WKT[length - 1] = '\0';  // 确保以零结尾

    // 将投影信息转化为OGRSpatialReference格式
    OGRSpatialReference oriSpatialReference;
    oriSpatialReference.importFromWkt(&ori_SRS_WKT);

    // 读取图像中的仿射变换系数
    double arrGeoTransform[6];
    pDataset->GetGeoTransform(arrGeoTransform);

    // 原始图像上（100，100）位置的点及其平面坐标
    int col, row;
    double projX_UTM, projY_UTM, projX_Gauss, projY_Gauss;
    col = 100;
    row = 100;
    projX_Gauss = projX_UTM = arrGeoTransform[0] + col * arrGeoTransform[1] + row * arrGeoTransform[2];
    projY_Gauss = projY_UTM = arrGeoTransform[3] + col * arrGeoTransform[4] + row * arrGeoTransform[5];

    // 建立转换后的目标空间参考（原图为UTM投影51N分带，目标Gauss为WGS84坐标系，3度带带号41的高斯克吕格投影）
    OGRSpatialReference* destSpatialReference_Gauss;
    destSpatialReference_Gauss = oriSpatialReference.CloneGeogCS();
    destSpatialReference_Gauss->SetTM(0, 123, 1.0, 41500000, 0);
    OGRCoordinateTransformation* ProjectTransform_Gauss = OGRCreateCoordinateTransformation(&oriSpatialReference, destSpatialReference_Gauss);
    if(!ProjectTransform_Gauss){
        cout << "Create ProjectTransform_Gauss Error!!";
        return false;
    }
    ProjectTransform_Gauss->Transform(1, &projX_Gauss, &projY_Gauss);

    // 在MessageBox中显示转换结果
    char transformResult[1000];
    printf(transformResult);
    printf("原始影像(100, 100)位置的点在UTM投影下经纬度为: %.5f, %.5f;\n在高斯克吕格投影下的经纬度: %.5f, %.5f", 
            projX_UTM, projY_UTM, projX_Gauss, projY_Gauss);
    

    OGRCoordinateTransformation::DestroyCT(ProjectTransform_Gauss);
    ProjectTransform_Gauss = nullptr;
    return true;
}

int main(){
    // 初始化GDAL
    GDALAllRegister();

    // 使gdal不默认使用UTF-8编码，以支持中文路径
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8","NO");

    // ReadProjectionInfo();
    WriteProjectionInfo();
    // TransformCoordinate();

    return 0;
}
