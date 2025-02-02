//
//  M_DataManager.cpp
//
//  Created by TuLigen on 2019/10/22.
//  Copyright © 2019年 TuLigen. All rights reserved.
//

#include "M_DataManager.h"
#include "M_DataLoader.h"
#include "dirent.h"

/* 后缀
*/
static bool isPicSuffix(const char *pName,size_t len)
{
    const size_t suffix = 3;
    assert(pName);
    if(len < suffix)
    {
        return false;
    }
    const char *pSuffix =  &pName[len - suffix];
    
    return !strcasecmp(pSuffix, "jpg") | !strcasecmp(pSuffix, "png");
}
/* 扫描文件
*/
static int loadFiles( const std::string &dirpath, std::vector<std::string> &files )
{
    DIR *dp;
    struct dirent *dirp;
    if((dp = opendir(dirpath.c_str())) == NULL)
    {
        assert(NULL);
    }
    int index = 0;
    while((dirp = readdir(dp)) != NULL)
    {
        if(isPicSuffix(dirp->d_name,strlen(dirp->d_name)))
        {
            std::string filepath(dirpath);
            filepath.append("/");
            filepath.append(dirp->d_name);
            files.emplace_back(filepath);
            // printf("%s\n",filepath.c_str());
            ++index;
        }
    }
    closedir(dp);
    sort(files.begin(),files.end());
    return index;
}

M_DataManager *M_DataManager::getSingleton()
{
    static M_DataManager instance;
    return &instance;
}
//判断浮点型　相等
static inline bool isSame(double pre, double cur)
{
    return fabs(cur - pre) < 1e-6;
}
class Cmp
{
public:
    Cmp(double t):_t(t){}

    bool operator()(const ImuRawData &data)
    {
        return isSame(data._t,_t);
    }
private:
    double _t;
};



void M_DataManager::setIndicator(int index)
{
    if(mIMURawDatas.empty() || 
       index > mPoseDatas.size())
        return;
    mIMURawIndicator = std::find_if(mIMURawIndicator,mIMURawDatas.end(),Cmp(mPoseDatas[index].second._t));
    cout << "indicator : " << mIMURawIndicator->_t << endl;
}

/*
     * 加载数据
     * @param pstpath 预处理后的imgpst 文件
     * @param imupath 预处理后的imu 文件
     */
bool M_DataManager::LoadData(const string &pstpath, const string &imupath)
{
    if (pstpath.empty())
    {
        return false;
    }
    else
    {
        ifstream pstfile, imufile;

        try
        {
            cout << "开始加载图片数据" << endl;
            cout << pstpath.c_str() << endl;
            pstfile.open(pstpath);
            

            std::string pststr;
            std::string imustr;

            //read headline
            getline(pstfile, pststr);
            getline(imufile, imustr);

            //load img pst file
            while (!pstfile.eof())
            {
                getline(pstfile, pststr);
                char filename[255] = {0};
                PoseData pose;
                sscanf(pststr.c_str(),"%s %lf %lf %lf %lf %lf %lf %lf", filename,
                                                                            &pose._t,
                                                                            &pose.pos.longitude,
                                                                            &pose.pos.latitude,
                                                                            &pose.pos.altitude,
                                                                            &pose._pitch,
                                                                            &pose._yaw,
                                                                            &pose._roll);

                mPoseDatas.emplace_back(std::make_pair(filename, pose));
            }

            //load imu file
            if(!imupath.empty())
            {
                cout << "开始加载imu数据" << endl;
                cout << imupath.c_str() << endl;
                imufile.open(imupath);
                while (!imufile.eof())
                {
                    getline(imufile, imustr);
                    ImuRawData rawdata;
                    sscanf(imustr.c_str(),"%lf %lf %lf %lf %lf %lf %lf", &rawdata._t,
                                                                     &rawdata._gyro_x,
                                                                     &rawdata._gyro_y,
                                                                     &rawdata._gyro_z,
                                                                     &rawdata._acc_x,
                                                                     &rawdata._acc_y,
                                                                     &rawdata._acc_z);
                    mIMURawDatas.emplace_back(rawdata);
                }
                imufile.close();
            }
            
            pstfile.close();
            cout << "文件加载成功" << endl;
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << '\n';
            return false;
        }
        //数据加载成功后 设置imu文件游标
        mIMURawIndicator = mIMURawDatas.begin();

        return true;
    }

    return false;
}


/* 当前时间到上个时间段内的imu数据集合
     * @param cursec(天秒)
     * @return 数据集
     */
IMURawVector M_DataManager::getIMUDataFromLastTime(double cursec)
{
    //断言　imu数据集不能为空，且传入的时间不能与上次时间一致
    assert(!mIMURawDatas.empty());// && !isSame(mIMURawIndicator->_t,cursec));

    if(isSame(mIMURawIndicator->_t,cursec))
        return IMURawVector();

    IMURawVIter iter = std::find_if(mIMURawIndicator,mIMURawDatas.end(),Cmp(cursec));

    // cout << iter->_t << endl;

    IMURawVector tmp;
    size_t sz = iter - mIMURawIndicator;
    tmp.reserve(sz);

    tmp.assign(mIMURawIndicator,iter);

    mIMURawIndicator = iter;

    return tmp;
}

#define FTLEN 25
#define WRITESPACE(FILE) FILE.width(FTLEN);
#define WRITEPOSEDATA(FILE, NAME, TIME, LON, LAT, ALT, PITCH, YAW, ROLL) \
    FILE.width(FTLEN * 2);                                               \
    FILE << NAME;                                                        \
    WRITESPACE(FILE)                                                     \
    FILE << TIME;                                                        \
    WRITESPACE(FILE)                                                     \
    FILE << LON;                                                         \
    WRITESPACE(FILE)                                                     \
    FILE << LAT;                                                         \
    WRITESPACE(FILE)                                                     \
    FILE << ALT;                                                         \
    WRITESPACE(FILE)                                                     \
    FILE << PITCH;                                                       \
    WRITESPACE(FILE)                                                     \
    FILE << YAW;                                                         \
    WRITESPACE(FILE)                                                     \
    FILE << ROLL;                                                        \
    FILE << endl;

#define WRITEIMURAWDATA(FILE, TIME, GX, GY, GZ, AX, AY, AZ) \
    WRITESPACE(FILE);                                       \
    FILE << TIME;                                           \
    WRITESPACE(FILE);                                       \
    FILE << GX;                                             \
    WRITESPACE(FILE);                                       \
    FILE << GY;                                             \
    WRITESPACE(FILE);                                       \
    FILE << GZ;                                             \
    WRITESPACE(FILE);                                       \
    FILE << AX;                                             \
    WRITESPACE(FILE);                                       \
    FILE << AY;                                             \
    WRITESPACE(FILE);                                       \
    FILE << AZ;                                             \
    FILE << endl;


/* 预处理数据
 * @param imgpath  图片路径
 * @param pstpath  pst路径
 * @param imupath  imu路径
 * @param outpath  输出路径
 * @param oimpath  imu输出路径
 */
void M_DataManager::PreprocessData(const std::string &imgpath,
                                 const std::string &pstpath,
                                 const std::string &imupath,
                                 const std::string &outpath,
                                 const std::string &oimpath,
                                 PreprocessType type /*= eStim300*/)
{
    // assert(!imgpath.empty() && pstpath.empty() && !imupath.empty() && !outpath.empty() && !oimpath.empty());

    ofstream pFile, pImu;
    pFile.precision(15);
    pFile.flags(ios::left | ios::fixed);

    pImu.precision(15);
    pImu.flags(ios::left | ios::fixed);

    try
    {
        pFile.open(outpath);
        pImu.open(oimpath);
        typedef std::vector<std::string> FileNameVec;
        FileNameVec fms;
        int sz = loadFiles(imgpath, fms);

        Ptr<IDataLoader> pPstDataLoader = new Stim300PostTDataLoader();
        pPstDataLoader->loadData(pstpath);

        Ptr<IMURawDataLoader> pIMUDataloader = new STIM300IMURawDataLoader();
        pIMUDataloader->loadData(imupath);

        cout << "begin writing datas." << endl;
        WRITEPOSEDATA(pFile, "Name", "Time", "Lon", "Lat", "Alt", "Pitch", "Yaw", "Roll");
        WRITEIMURAWDATA(pImu, "Time", "Gyro_x", "Gyro_y", "Gyro_z", "Acc_x", "Acc_y", "Acc_z");

        double bgtime = 0.0;
        double edtime = 0.0;
        for (int i = 0; i < sz; ++i)
        {
            const std::string &imgname = fms[i].substr(fms[i].length() - 38);
            const double imgtime = M_Untils::GetDayTimeFromPicName(imgname);

            const PoseData posedata = pPstDataLoader->getData(imgtime);

            WRITEPOSEDATA(pFile, imgname, posedata._t,
                          posedata.pos.longitude,
                          posedata.pos.latitude,
                          posedata.pos.altitude,
                          posedata._pitch,
                          posedata._yaw,
                          posedata._roll);

            if (i > 0)
            {//from second frame
                edtime = imgtime;
                IMURawVector rawdatas(pIMUDataloader->getDatas(bgtime, edtime));
                const int sti = (i == 1) ? 0 : 1;
                for (size_t i = sti; i < rawdatas.size(); ++i)
                {
                    WRITEIMURAWDATA(pImu, rawdatas[i]._t,
                                    rawdatas[i]._gyro_x,
                                    rawdatas[i]._gyro_y,
                                    rawdatas[i]._gyro_z,
                                    rawdatas[i]._acc_x,
                                    rawdatas[i]._acc_y,
                                    rawdatas[i]._acc_z);
                }
                bgtime = edtime;
            }
            else
            {
                bgtime = imgtime;
            }
        }
        pFile.close();
        pImu.close();
        cout << "write successfully!!!" << endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
}