import torch
import torch.nn as nn
import torch.utils.data.dataset as Dataset
from torch.utils.data import DataLoader
import torchvision.transforms as transforms
import numpy as np
from PIL import Image 
import os
from tqdm import tqdm

#自定义数据集类
class _dataset(Dataset.Dataset):
    def __init__(self, csv_dir):
        self.csv_dir = csv_dir          
        self.names_list = []
        self.size = 0
        self.transform = transforms.ToTensor()
        #读入csv文件
        if not os.path.isfile(self.csv_dir):
            print(self.csv_dir + ':txt file does not exist!')
        file = open(self.csv_dir)
        for f in file:
            self.names_list.append(f)
            self.size += 1

    def __len__(self):
        return self.size

    def __getitem__(self, idx):
        #读取图像路径并打开图像
        image_path = self.names_list[idx].split(',')[0]
        image = Image.open(image_path)
        #读取标签路径并打开标签图像
        label_path = self.names_list[idx].split(',')[1].strip('\n')
        label = Image.open(label_path)
        #转为tensor形式
        image = self.transform(image)
        label = torch.from_numpy(np.array(label))
        return image, label

#定义unet模型相关类
class conv_conv(nn.Module):
    ''' conv_conv: (conv[3*3] + BN + ReLU) *2 '''
    def __init__(self, in_channels, out_channels, bn_momentum=0.1):
        super(conv_conv, self).__init__()
        self.conv = nn.Sequential(
            nn.Conv2d(in_channels, out_channels, kernel_size=3, padding=1, stride=1),
            nn.BatchNorm2d(out_channels, momentum=bn_momentum),
            nn.ReLU(inplace=True),
            nn.Conv2d(out_channels, out_channels, kernel_size=3, padding=1, stride=1),
            nn.BatchNorm2d(out_channels, momentum=bn_momentum),
            nn.ReLU(inplace=True)
            )

    def forward(self, X):
        X = self.conv(X)
        return X

class downconv(nn.Module):
    ''' downconv: conv_conv => maxpool[2*2] '''
    def __init__(self, in_channels, out_channels, bn_momentum=0.1):
        super(downconv, self).__init__()
        self.conv = conv_conv(in_channels, out_channels, bn_momentum)
        self.pool = nn.MaxPool2d(kernel_size=2, stride=2)
    
    def forward(self, X):
        X = self.conv(X)
        pool_X = self.pool(X)
        return pool_X, X
        
class upconv_concat(nn.Module):
    ''' upconv_concat: upconv[2*2] => cat => conv_conv '''
    def __init__(self, in_channels, out_channels, bn_momentum=0.1):
        super(upconv_concat, self).__init__()
        self.upconv = nn.ConvTranspose2d(in_channels, out_channels, kernel_size=2, stride=2)
        self.conv = conv_conv(in_channels, out_channels, bn_momentum)

    def forward(self, X1, X2):
        X1 = self.upconv(X1)
        feature_map = torch.cat((X2, X1), dim=1)
        X1 = self.conv(feature_map)
        return X1

class UNet(nn.Module):
    ''' UNet(3-level): downconv *3 => conv_conv => upconv *3 => conv[1*1]'''
    def __init__(self, in_channels, out_channels, starting_filters=32, bn_momentum=0.1):
        super(UNet, self).__init__()
        self.conv1 = downconv(in_channels, starting_filters, bn_momentum) 
        self.conv2 = downconv(starting_filters, starting_filters * 2, bn_momentum)
        self.conv3 = downconv(starting_filters * 2, starting_filters * 4, bn_momentum)
        self.conv4 = conv_conv(starting_filters * 4, starting_filters * 8, bn_momentum)
        self.upconv1 = upconv_concat(starting_filters * 8, starting_filters * 4, bn_momentum)
        self.upconv2 = upconv_concat(starting_filters * 4, starting_filters * 2, bn_momentum)
        self.upconv3 = upconv_concat(starting_filters * 2, starting_filters, bn_momentum)
        self.conv_out = nn.Conv2d(starting_filters, out_channels, kernel_size=1, padding=0, stride=1)

    def forward(self, X):
        X, conv1 = self.conv1(X)
        X, conv2 = self.conv2(X)
        X, conv3 = self.conv3(X)
        X = self.conv4(X)
        X = self.upconv1(X, conv3)
        X = self.upconv2(X, conv2)
        X = self.upconv3(X, conv1)
        X = self.conv_out(X)
        return X

# 以下为训练部分
########################################################################
# 载入数据
datatrain = _dataset('dataset\\train.csv')
print("train:",len(datatrain))
train_dataloader = DataLoader(datatrain, batch_size=16, shuffle=True)
# 实例化模型
unet = UNet(in_channels=3,out_channels=6)
# 设置损失函数为交叉熵函数
Loss_function = nn.CrossEntropyLoss()
# 设置优化方法为自适应动量法
optimizer = torch.optim.Adam(unet.parameters(), lr=0.001)
# epoch代表遍历完所有样本的过程，将epoch设置10，即遍历完样本10次
epoch_num = 10
for epoch in range(epoch_num):
    print('EPOCH %d/%d'%(epoch + 1,epoch_num))
    print('-'*10)
    #批量输入数据
    for images, labels in tqdm(train_dataloader):
        outputs = unet(images)                          # 将影像输入网络得到输出 
        optimizer.zero_grad()                           # 将梯度置为0
        loss = Loss_function(outputs, labels.long())    # 计算损失
        loss.backward()                                 # 损失反向传播
        optimizer.step()                                # 优化更新网络权重   
    # 打印损失值
    print('loss is %f'%(loss.item()))
    # 保存模型
    torch.save(unet,'dataset\\model_epoch%d.pkl'%(epoch + 1))


# # 以下为验证部分
# #########################################################################
# # 载入数据
# datatest = _dataset('dataset\\test.csv')
# print("test:",len(datatest))
# test_dataloader = DataLoader(datatest, batch_size=16, shuffle=False)
# # 载入训练好的模型
# net = torch.load('dataset\\model_epoch10.pkl', map_location=lambda storage, loc: storage)
# # 设置为测试模式
# net.eval()
# running_correct_full = 0
# img_no = 0
# for images, labels in tqdm(test_dataloader):
#     outputs = net(images)                                   # 得到网络输出
#     _, preds = torch.max(outputs, 1)                        # 预测类别值
#     running_correct_full+=torch.sum(preds == labels.data)   # 统计分类正确像元数
#     # 输出结果查看
#     images = images.numpy()
#     preds = preds.numpy()
#     labels = labels.numpy()
#     for j in range(labels.shape[0]):
#         img = transforms.ToPILImage()(torch.from_numpy(images[j]))
#         label = Image.fromarray(labels[j].astype(np.uint8))
#         predect = transforms.ToPILImage()(preds[j].astype(np.uint8))
#         image_fn = 'dataset\\save\\img' + str(img_no) + '.png'
#         label_fn = 'dataset\\save\\lab' + str(img_no) + '.png'
#         predect_fn = 'dataset\\save\\pre' + str(img_no) + '.png'
#         img.save(image_fn)
#         label.save(label_fn)
#         predect.save(predect_fn)
#         img_no = img_no+1
# # 计算测试精度
# acc = running_correct_full.double()/(len(datatest)*labels.shape[1]*labels.shape[2])
# print("Overall Accuracy: ",acc.cpu().numpy())