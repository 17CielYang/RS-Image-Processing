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
#include "cpl_conv.h"

using namespace std;

const float param_13 = 1.0f / 3.0f;
const float param_16116 = 16.0f / 116.0f;
const float Xn = 0.950456f;
const float Yn = 1.0f;
const float Zn = 1.088754f;

// Lab <-> XYZ 中的判断阈值
static const float EPSILON = 0.008856f; // 实际为 (6/29)^3
static const float KAPPA   = 903.3f;    // 实际为 (29/3)^3

// gamma: 将 sRGB (0~1) 转换到 线性空间
inline float gamma_sRGB_to_linear(float x) {
    return (x > 0.04045f) ? powf((x + 0.055f) / 1.055f, 2.4f)
                          : (x / 12.92f);
}

// gamma: 将线性空间 (0~1) 转换到 sRGB
inline float gamma_linear_to_sRGB(float x) {
    return (x > 0.0031308f) ? (1.055f * powf(x, 1.0f / 2.4f) - 0.055f)
                            : (12.92f * x);
}

// f(t)函数用于 XYZ -> Lab
inline float f_xyz_to_lab(float t) {
    return (t > EPSILON) ? cbrtf(t) : (7.787f * t + 16.0f / 116.0f);
}

// f^-1(t)函数用于 Lab -> XYZ
inline float finv_lab_to_xyz(float t) {
    // 反推，如果 t^3 > EPSILON，则说明 (L+16)/116 > 6/29
    float t3 = t * t * t;
    return (t3 > EPSILON) ? t3 : ((t - 16.0f / 116.0f) / 7.787f);
}

void RGB2XYZ(int R, int G, int B, float* X, float* Y, float* Z) {
    // 1) 先转到 [0..1]
    float r = R / 255.0f;
    float g = G / 255.0f;
    float b = B / 255.0f;

    // 2) gamma 校正 -> 线性空间
    float r_lin = gamma_sRGB_to_linear(r);
    float g_lin = gamma_sRGB_to_linear(g);
    float b_lin = gamma_sRGB_to_linear(b);

    // 3) 使用标准的 sRGB->XYZ 矩阵 (D65)
    *X = 0.412453f * r_lin + 0.357580f * g_lin + 0.180423f * b_lin;
    *Y = 0.212671f * r_lin + 0.715160f * g_lin + 0.072169f * b_lin;
    *Z = 0.019334f * r_lin + 0.119193f * g_lin + 0.950227f * b_lin;
}

void XYZ2Lab(float X, float Y, float Z, float* L, float* a, float* b) {
    // 1) 归一化
    float x = X / Xn;
    float y = Y / Yn;
    float z = Z / Zn;

    // 2) f(t)
    float fx = f_xyz_to_lab(x);
    float fy = f_xyz_to_lab(y);
    float fz = f_xyz_to_lab(z);

    // 3) 计算 L a b
    // L: [0..100], 但实际可能超出，取决于输入 XYZ 范围
    *L = (116.0f * fy) - 16.0f;
    *a = 500.0f * (fx - fy);
    *b = 200.0f * (fy - fz);
}

void Lab2XYZ(float L, float a, float b, float* X, float* Y, float* Z) {
    // 1) 求 fY, fX, fZ
    float fy = (L + 16.0f) / 116.0f;
    float fx = a / 500.0f + fy;
    float fz = fy - b / 200.0f;

    // 2) 逆变换
    float xr = finv_lab_to_xyz(fx);
    float yr = finv_lab_to_xyz(fy);
    float zr = finv_lab_to_xyz(fz);

    // 3) 乘回参考白点
    *X = xr * Xn;
    *Y = yr * Yn;
    *Z = zr * Zn;
}

void XYZ2RGB(float X, float Y, float Z, int* R, int* G, int* B) {
    // 1) 线性空间
    float r_lin =  3.2404542f * X - 1.5371385f * Y - 0.4985314f * Z;
    float g_lin = -0.9692660f * X + 1.8760108f * Y + 0.0415560f * Z;
    float b_lin =  0.0556434f * X - 0.2040259f * Y + 1.0572252f * Z;

    // 2) gamma 校正到 sRGB (0..1)
    float r = gamma_linear_to_sRGB(r_lin);
    float g = gamma_linear_to_sRGB(g_lin);
    float b = gamma_linear_to_sRGB(b_lin);

    // 3) 转到 [0..255] 并截断
    int Ri = static_cast<int>(std::floor(r * 255.0f + 0.5f));
    int Gi = static_cast<int>(std::floor(g * 255.0f + 0.5f));
    int Bi = static_cast<int>(std::floor(b * 255.0f + 0.5f));

    // 4) 防止溢出
    *R = std::max(0, std::min(255, Ri));
    *G = std::max(0, std::min(255, Gi));
    *B = std::max(0, std::min(255, Bi));
}

void RGB2Lab(int R, int G, int B, float* L, float* a, float* b) {
    float X, Y, Z;
    RGB2XYZ(R, G, B, &X, &Y, &Z);
    XYZ2Lab(X, Y, Z, L, a, b);
}

void Lab2RGB(float L, float a, float b, int* R, int* G, int* B) {
    float X, Y, Z;
    Lab2XYZ(L, a, b, &X, &Y, &Z);
    XYZ2RGB(X, Y, Z, R, G, B);
}

float gamma(float x) {
    return x > 0.04045f ? powf((x + 0.055f) / 1.055f, 2.4f) : (x / 12.92f);
}

float gamma_XYZ2RGB(float x) {
    return x > 0.0031308f ? (1.055f * powf(x, (1 / 2.4f)) - 0.055) : (x * 12.92f);
}

int main() {
    // 注册 GDAL 驱动
    GDALAllRegister();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    // 打开输入影像(假设 3 波段 8bit)
    const char* filepath = "/Users/shiqiyang/Files/Codes/RSI-fundation/lab13/data/1.tif";
    GDALDataset* poDataset = (GDALDataset*)GDALOpen(filepath, GA_ReadOnly);
    if (!poDataset) {
        cerr << "Read error for " << filepath << endl;
        return -1;
    }

    // 读图像大小
    int rows = poDataset->GetRasterYSize();
    int cols = poDataset->GetRasterXSize();
    int bands = poDataset->GetRasterCount(); // 若<3, 需适配

    cout << "Image size: " << rows << " x " << cols << ", bands: " << bands << endl;

    // 读取图像数据到 image[row][col][3] (存储RGB)
    vector<vector<vector<float>>> image(rows, vector<vector<float>>(cols, vector<float>(3, 0.f)));

    // 逐波段读取
    vector<GByte> bandData(rows * cols);
    for (int b = 1; b <= 3; b++) {
        GDALRasterBand* poBand = poDataset->GetRasterBand(b);
        if(!poBand) {
            cerr << "Cannot get band " << b << endl;
            return -1;
        }

        CPLErr err = poBand->RasterIO(
            GF_Read, 0, 0, cols, rows,
            &bandData[0], cols, rows, GDT_Byte,
            0, 0
        );
        if (err != CE_None) {
            cerr << "RasterIO error on band " << b << endl;
            return -1;
        }

        int channelIndex = b - 1; // 0->R, 1->G, 2->B
        for(int i = 0; i < rows; i++){
            for(int j = 0; j < cols; j++){
                int idx = i * cols + j;
                image[i][j][channelIndex] = static_cast<float>(bandData[idx]);
            }
        }
    }

    // 将RGB转为Lab
    for(int i = 0; i < rows; i++){
        for(int j = 0; j < cols; j++){
            float R = image[i][j][0];
            float G = image[i][j][1];
            float B = image[i][j][2];
            float L, A, Bb;
            RGB2Lab((int)R, (int)G, (int)B, &L, &A, &Bb);
            image[i][j][0] = L;
            image[i][j][1] = A;
            image[i][j][2] = Bb;
        }
    }

    // 准备 SLIC 参数
    int N = rows * cols; // 总像素数
    int K = 150;         // 超像素数量(可调)
    int S = static_cast<int>( sqrt(N / double(K)) ); // 步长
    float m = 40.f;      // 颜色/空间距离平衡系数(可调)
    int maxIter = 10;    // 迭代次数

    // --- 初始化聚类中心 [x, y, L, a, b] ---
    vector<vector<float>> Cluster; // Cluster[c] = [x, y, L, a, b]
    for(int i = S/2; i < rows; i += S){
        for(int j = S/2; j < cols; j += S){
            vector<float> c;
            c.push_back((float)i);          // x
            c.push_back((float)j);          // y
            c.push_back(image[i][j][0]);    // L
            c.push_back(image[i][j][1]);    // a
            c.push_back(image[i][j][2]);    // b
            Cluster.push_back(c);
        }
    }
    int cluster_num = (int)Cluster.size();
    cout << "Init cluster size = " << cluster_num << endl;

    // --- 可选：在 3x3 邻域内移动中心到梯度最小的位置 ---
    for(int c = 0; c < cluster_num; c++){
        int cx = (int)Cluster[c][0];
        int cy = (int)Cluster[c][1];

        float minGrad = numeric_limits<float>::max();
        int bestX = cx, bestY = cy;
        // 在 3x3 中搜梯度最小
        for(int dx = -1; dx <= 1; dx++){
            for(int dy = -1; dy <= 1; dy++){
                int nx = cx + dx;
                int ny = cy + dy;
                if(nx > 0 && nx < rows - 1 && ny > 0 && ny < cols - 1){
                    float gx = image[nx+1][ny][0] - image[nx-1][ny][0];
                    float gy = image[nx][ny+1][0] - image[nx][ny-1][0];
                    float grad = gx*gx + gy*gy;
                    if(grad < minGrad){
                        minGrad = grad;
                        bestX = nx; bestY = ny;
                    }
                }
            }
        }
        // 更新中心
        Cluster[c][0] = (float)bestX;
        Cluster[c][1] = (float)bestY;
        Cluster[c][2] = image[bestX][bestY][0]; // L
        Cluster[c][3] = image[bestX][bestY][1]; // a
        Cluster[c][4] = image[bestX][bestY][2]; // b
    }

    // --- SLIC 迭代 ---
    // 1) 距离矩阵 distance[x][y], 标签矩阵 label[x][y]
    vector<vector<double>> distance(rows, vector<double>(cols, numeric_limits<double>::infinity()));
    vector<vector<int>> label(rows, vector<int>(cols, -1));

    // 2) 每个 cluster 的像素集合
    vector<vector<vector<int>>> pixel(cluster_num); // pixel[c] = {{x,y},...}

    for(int t = 0; t < maxIter; t++){
        cout << "\nIteration = " << (t+1) << endl;

        // 2.1) 每次迭代都要重置 distance,label,pixel
        for(int i = 0; i < rows; i++){
            for(int j = 0; j < cols; j++){
                distance[i][j] = numeric_limits<double>::infinity();
                label[i][j] = -1;
            }
        }
        for(int c = 0; c < cluster_num; c++){
            pixel[c].clear();
        }

        // 2.2) 为每个中心在 2S 范围内的像素计算距离
        for(int c = 0; c < cluster_num; c++){
            int cx = (int)Cluster[c][0];
            int cy = (int)Cluster[c][1];
            float Lc = Cluster[c][2];
            float ac = Cluster[c][3];
            float bc = Cluster[c][4];

            int startX = max(0, cx - 2*S);
            int endX   = min(rows, cx + 2*S);
            int startY = max(0, cy - 2*S);
            int endY   = min(cols, cy + 2*S);

            for(int x = startX; x < endX; x++){
                for(int y = startY; y < endY; y++){
                    float Lp = image[x][y][0];
                    float ap = image[x][y][1];
                    float bp = image[x][y][2];

                    // 颜色距离(用 L,a,b 的欧氏距离)
                    float Dc = (Lp - Lc)*(Lp - Lc)
                             + (ap - ac)*(ap - ac)
                             + (bp - bc)*(bp - bc);
                    // 空间距离
                    float Ds = float((x - cx)*(x - cx) + (y - cy)*(y - cy));

                    // 综合距离(可以开方或保持平方，都可调)
                    float D = sqrt(Dc) + (m / S) * sqrt(Ds);

                    // 若更小则更新
                    if(D < distance[x][y]){
                        distance[x][y] = D;
                        label[x][y] = c;
                    }
                }
            }
        }

        // 2.3) 根据 label，把像素 (x,y) 放进 pixel[label[x][y]]
        for(int x = 0; x < rows; x++){
            for(int y = 0; y < cols; y++){
                int c = label[x][y];
                if(c >= 0){
                    pixel[c].push_back({x, y});
                }
            }
        }

        // 2.4) 更新每个中心
        for(int c = 0; c < cluster_num; c++){
            if(pixel[c].empty()) continue;

            double sumx=0, sumy=0, sumL=0, suma=0, sumb=0;
            int count = (int)pixel[c].size();
            for(int p = 0; p < count; p++){
                int px = pixel[c][p][0];
                int py = pixel[c][p][1];
                sumx += px;
                sumy += py;
                sumL += image[px][py][0];
                suma += image[px][py][1];
                sumb += image[px][py][2];
            }
            float meanx = (float)(sumx / count);
            float meany = (float)(sumy / count);
            float meanL = (float)(sumL / count);
            float meana = (float)(suma / count);
            float meanb = (float)(sumb / count);

            Cluster[c][0] = meanx;
            Cluster[c][1] = meany;
            Cluster[c][2] = meanL;
            Cluster[c][3] = meana;
            Cluster[c][4] = meanb;
        }
    }

    cout << "SLIC iteration finished.\n";

    // --- 生成输出：把每个像素替换成其所属 cluster 的中心颜色 ---
    //   (这样可视化就会出现大的色块)
    vector<vector<vector<float>>> out_image = image; // 复制一份
    for(int c = 0; c < cluster_num; c++){
        // cluster c 的 (L,a,b)
        float Lc = Cluster[c][2];
        float Ac = Cluster[c][3];
        float Bc = Cluster[c][4];
        for(auto &p : pixel[c]){
            int px = p[0], py = p[1];
            out_image[px][py][0] = Lc;
            out_image[px][py][1] = Ac;
            out_image[px][py][2] = Bc;
        }
        // 也可让中心点某个特殊颜色(如黑色)
        int ccx = (int)Cluster[c][0];
        int ccy = (int)Cluster[c][1];
        out_image[ccx][ccy][0] = 0.f;
        out_image[ccx][ccy][1] = 0.f;
        out_image[ccx][ccy][2] = 0.f;
    }

    // --- 写出结果图(先转回 RGB，再存成 TIFF) ---
    GDALDriver* poDriver = GetGDALDriverManager()->GetDriverByName("GTiff");
    if(!poDriver){
        cerr << "Cannot get GTiff driver." << endl;
        return -1;
    }

    const char* outPath = "/Users/shiqiyang/Files/Codes/RSI-fundation/lab13/data/result.tif"; // 请自行修改
    GDALDataset* poDstDS = poDriver->Create(outPath, cols, rows, 3, GDT_Byte, NULL);
    if(!poDstDS){
        cerr << "Create TIFF failed." << endl;
        return -1;
    }

    vector<GByte> bufR(rows*cols), bufG(rows*cols), bufB(rows*cols);

    for(int i = 0; i < rows; i++){
        for(int j = 0; j < cols; j++){
            float L_ = out_image[i][j][0];
            float A_ = out_image[i][j][1];
            float B_ = out_image[i][j][2];

            int R, G, Bb;
            Lab2RGB(L_, A_, B_, &R, &G, &Bb);

            // 裁剪 [0..255]
            R = max(0, min(255, R));
            G = max(0, min(255, G));
            Bb = max(0, min(255, Bb));

            int idx = i*cols + j;
            bufR[idx] = (GByte)R;
            bufG[idx] = (GByte)G;
            bufB[idx] = (GByte)Bb;
        }
    }

    // 写三波段
    poDstDS->GetRasterBand(1)->RasterIO(GF_Write, 0, 0, cols, rows,
                                        &bufR[0], cols, rows, GDT_Byte, 0, 0);
    poDstDS->GetRasterBand(2)->RasterIO(GF_Write, 0, 0, cols, rows,
                                        &bufG[0], cols, rows, GDT_Byte, 0, 0);
    poDstDS->GetRasterBand(3)->RasterIO(GF_Write, 0, 0, cols, rows,
                                        &bufB[0], cols, rows, GDT_Byte, 0, 0);

    GDALClose(poDstDS);
    cout << "Result saved: " << outPath << endl;

    // 关闭输入影像
    GDALClose(poDataset);
    return 0;
}