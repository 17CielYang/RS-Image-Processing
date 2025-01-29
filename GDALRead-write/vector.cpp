#include "vector.h"

using namespace std;

void myVectorIO::OpenFile(const string& inputPath) {
    inputDataset = (GDALDataset *)GDALOpenEx(inputPath.c_str(), GDAL_OF_VECTOR, NULL, NULL, NULL);
    if (inputDataset != NULL) {
        cout << "Opened source file: " << inputPath << endl;
    } else {
        cerr << "Failed to open source file." << endl;
    }
}

void myVectorIO::CreateFile(const string& outputPath){
    GDALDriver* driver = GetGDALDriverManager()->GetDriverByName("ESRI Shapefile");
    if(!driver) {
        cout << "Driver not available." << endl;
    }

    outputDataset = driver->Create(outputPath.c_str(), 0, 0, 0, GDT_Unknown, NULL);
    if(outputDataset != NULL){
        cout << "Created destination file: " << outputPath << endl;
    } else {
        cerr << "Destination dataset could not be created." << endl;
    }
}

void myVectorIO::ReadObject() {
    srcLayer = inputDataset->GetLayer(0);
    spatialRef = srcLayer->GetSpatialRef();
    geomType = srcLayer->GetGeomType();
}

void myVectorIO::copyObject(const string& inputPath, const string& outputPath, int itemCount) {
    destLayer = outputDataset->CreateLayer("NewLayer1", spatialRef, geomType, NULL);
    if (!destLayer) {
        cout << "Destination layer could not be created." << endl;
    }
    // Transfer fields from source layer
    OGRFeatureDefn *srcFDefn = srcLayer->GetLayerDefn(); 
    int fieldCount = srcFDefn->GetFieldCount(); // 获取字段数
    for (int i = 0; i < fieldCount; i++) {
        OGRFieldDefn *fieldDefn = srcFDefn->GetFieldDefn(i);
        destLayer->CreateField(fieldDefn);
    }

    // Add two new fields
    OGRFieldDefn newField1("NewField1", OFTString);
    newField1.SetWidth(32);
    destLayer->CreateField(&newField1);

    OGRFieldDefn newField2("NewField2", OFTInteger);
    destLayer->CreateField(&newField2);

    // 复制前itemCount个对象
    srcLayer->ResetReading();
    int count = 0;
    while ((feature = srcLayer->GetNextFeature()) != nullptr && count < itemCount) {
        OGRFeature *newFeature = OGRFeature::CreateFeature(destLayer->GetLayerDefn());
        newFeature->SetFrom(feature);
        newFeature->SetField("NewField1", "DefaultValue");
        newFeature->SetField("NewField2", 123);
        if (destLayer->CreateFeature(newFeature) != OGRERR_NONE) {
            cerr << "Failed to create feature in destination layer." << endl;
            OGRFeature::DestroyFeature(feature);
            OGRFeature::DestroyFeature(newFeature);
            break;
        }
        OGRFeature::DestroyFeature(feature);
        OGRFeature::DestroyFeature(newFeature);
        count++;
    }
}