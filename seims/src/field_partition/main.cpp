#if (defined _DEBUG) && (defined MSVC) && (defined VLD)
#include "vld.h"
#endif /* Run Visual Leak Detector during Debug */
#include <iostream>
#include <vector>
#include <ctime>
#include <string>
#include <sstream>
#include <map>
#include <set>

#include "utilities.h"
#include "clsRasterData.cpp"
#include "CellOrdering.h"
#include "FieldPartition.h"

using namespace std;

map<int, int> dirToIndexMap;

/// Check if the required raster files existed in the directory
bool checkInputRasterName(string &dir, map <string, string> &rstFilePaths);

/// Find outlet location according to flow direction, stream link, and DEM. Updated by LJ.
void findOutlet(clsRasterData<float> &rsDEM, clsRasterData<int> &rsStreamLink, clsRasterData<int> &rsDir,
                FlowDirectionMethod flowDirMtd, int &rowIndex, int &colIndex);

/// Do field partition mission.
void DoFieldsPartition(map <string, string> &rstFilePaths, FlowDirectionMethod flowDirMtd, int threshod);

int main(int argc, const char *argv[]) {
    GDALAllRegister();
    /// Header print information
    cout << "                     Field Partition Program                        " << endl;
    cout << "                           Version: 1.2                                     " << endl;
    cout << "                    Compile date: 2017-2-9                      " << endl;
    cout << "                        Author: Hui Wu                                   " << endl;
    cout << "                     Revised: Liang-Jun Zhu                       " << endl;

    string Dir = "";
    int Threshod = -1;
    FlowDirectionMethod flowDirMtd;
    map <string, string> rstFilePaths;
    clock_t start, finish;
    double duration = 0.;
    start = clock();
    if (argc < 2) {
        cout << "Error: To run this field partition program, please follow the COMMAND." << endl;
        goto errexit;
    } else if (argc == 2) {
        if (argv[1] != NULL)Dir = argv[1]; else goto errexit;
        Threshod = 50;
        flowDirMtd = (FlowDirectionMethod) 0;
    } else if (argc == 3) {
        if (argv[1] != NULL)Dir = argv[1]; else goto errexit;
        if (argv[2] != NULL)Threshod = atoi(argv[2]); else goto errexit;
        flowDirMtd = (FlowDirectionMethod) 0;
    } else {
        if (argv[1] != NULL)Dir = argv[1]; else goto errexit;
        if (argv[2] != NULL)Threshod = atoi(argv[2]); else goto errexit;
        if (argv[3] != NULL) {
            if (atoi(argv[3]) - 0. < UTIL_ZERO) {
                flowDirMtd = (FlowDirectionMethod) 0;
            } else {
                flowDirMtd = (FlowDirectionMethod) 1;
            }
        } else { goto errexit; }
    }

    if (checkInputRasterName(Dir, rstFilePaths)) {
        DoFieldsPartition(rstFilePaths, flowDirMtd, Threshod);
    } else {
        goto errexit;
    }
    finish = clock();

    duration = (double) (finish - start) / CLOCKS_PER_SEC;
    cout << "Finished! Time-consuming (sec): " << duration << endl;
    //system("pause");
    return 0;

    errexit:
    cout << " Command: <Raster files path> [<threshold>] [<FlowDirection method>]" << endl;
    cout
        << "\t1. Rasters include dem, mask, landuse, flow, stream MUST be located in raster files path, the format can be ASC or GTIFF; "
        << endl;
    cout << "\t2. Threshold MUST be greater than 0, the default is 50;" << endl;
    cout << "\t3. Flow direction method MUST be 0 (TauDEM) or 1 (ArcGIS), the default is 0." << endl;
    exit(0);
}

bool checkInputRasterName(string &dir, map <string, string> &rstFilePaths) {
    vector <string> dstFiles;
    FindFiles(dir.c_str(), "*.*", dstFiles);
    string rstNames[5] = {"dem", "landuse", "mask", "flow", "stream"};
    bool flag = true;
    for (int i = 0; i < 5; i++) {
        for (vector<string>::iterator it = dstFiles.begin(); it != dstFiles.end(); it++) {
            string tmpFileFullName = GetUpper(*it);
            if (tmpFileFullName.find(GetUpper(rstNames[i])) == string::npos) {
                flag = false;
                continue;
            } else {
                flag = true;
                rstFilePaths[GetUpper(rstNames[i])] = *it;
                break;
            }
        }
        if (!flag) return false;
    }
    return flag;
}

void findOutlet(clsRasterData<float> &rsDEM, clsRasterData<int> &rsStreamLink, clsRasterData<int> &rsDir,
                FlowDirectionMethod flowDirMtd, int &rowIndex, int &colIndex) {
    if (flowDirMtd) {
        /// flow direction in ArcGIS
        dirToIndexMap[1] = 0;
        dirToIndexMap[2] = 1;
        dirToIndexMap[4] = 2;
        dirToIndexMap[8] = 3;
        dirToIndexMap[16] = 4;
        dirToIndexMap[32] = 5;
        dirToIndexMap[64] = 6;
        dirToIndexMap[128] = 7;
    } else {
        /// TauDEM
        dirToIndexMap[1] = 0;
        dirToIndexMap[8] = 1;
        dirToIndexMap[7] = 2;
        dirToIndexMap[6] = 3;
        dirToIndexMap[5] = 4;
        dirToIndexMap[4] = 5;
        dirToIndexMap[3] = 6;
        dirToIndexMap[2] = 7;
    }
    if (rsDEM.getRows() <= 0 || rsDEM.getCols() <= 0) {
        cout << "Error: the input of DEM was invalid!\n";
        exit(-1);
    }
    /// updated by Liangjun Zhu, Apr. 1, 2016
    bool flag = false;
    for (int i = 0; i < rsStreamLink.getRows(); i++) {
        for (int j = 0; j < rsStreamLink.getCols(); j++) {
            RowColCoor rc(i, j);
            if (!rsStreamLink.isNoData(rc) && rsStreamLink.getValue(rc) > 0) {
                colIndex = j;
                rowIndex = i;
                flag = true;
                break;
            }
        }
        if (flag) {
            break;
        }
    }
    /// first stream grid
    /// cout<<rowIndex<<","<<colIndex<<endl;
    flag = true;
    while (flag) {
        RowColCoor rc(rowIndex, colIndex);
        int index = dirToIndexMap[rsDir.getValue(rc)];
        int ii = rowIndex + CellOrdering::m_d1[index];
        int jj = colIndex + CellOrdering::m_d2[index];
        if (ii < rsDEM.getRows() - 1 && jj < rsDEM.getCols() - 1) {
            RowColCoor rc_new(ii, jj);
            if (rsStreamLink.isNoData(rc_new) || rsStreamLink.getValue(rc_new) <= 0) {
                flag = false;
            } else {
                rowIndex = ii;
                colIndex = jj;
            }
        } else {
            flag = false;
        }
    }
    cout << "\t\tOutlet location: row is " << rowIndex << ", col is " << colIndex << endl;
}

void DoFieldsPartition(map <string, string> &rstFilePaths, FlowDirectionMethod flowDirMtd, int threshod) {
    ostringstream oss;
    string dirName, LanduName, maskName, demName, streamLinkName;
    dirName = rstFilePaths["FLOW"];
    LanduName = rstFilePaths["LANDUSE"];
    maskName = rstFilePaths["MASK"];
    demName = rstFilePaths["DEM"];
    streamLinkName = rstFilePaths["STREAM"];

    //output
    string dir = GetPathFromFullName(demName);
    oss.str("");
    //oss << dir << "field_"<<threshod<<"."<<GetLower(GetSuffix(dirName));
    oss << dir << "field_" << threshod << ".tif";
    string rsfieldFile = oss.str();

    oss.str("");
    oss << dir << "fields_" << threshod << ".txt";
    string txtfileFile = oss.str();

    /// Write basin input and output information
    cout << "Input Files:" << endl;
    cout << "\tFlow Direction: " << dirName << endl;
    cout << "\tLanduse: " << LanduName << endl;
    cout << "\tStream Links: " << streamLinkName << endl;
    cout << "\tDEM: " << demName << endl;
    cout << "\tMask: " << maskName << endl << endl;
    cout << "Output Files:" << endl;
    cout << "\tFields Raster: " << rsfieldFile << endl;
    cout << "\tFields Flow Relationship: " << txtfileFile << endl << endl;

    cout << "Executing ..." << endl;
    cout << "\tRead input raster files..." << endl;
    IntRaster rsDir(dirName, false);
    IntRaster rsLandu(LanduName, false);
    IntRaster rsMask(maskName, false);
    IntRaster rsStrLink(streamLinkName, false);

    //cout<<"xll: "<<rsMask.GetXllCenter()<<", yll: "<<rsMask.GetYllCenter()<<endl;
    clsRasterData<float> rsDEM(demName, false);

    int rowIndex, colIndex;
    cout << "\tFind outlet location..." << endl;
    findOutlet(rsDEM, rsStrLink, rsDir, flowDirMtd, rowIndex, colIndex);
    cout << "\tInitiate field partition class ..." << endl;
    CellOrdering cellOrdering(&rsDir, &rsLandu, &rsMask, flowDirMtd, threshod);
    cout << "\tExecute field partition ..." << endl;
    cellOrdering.ExcuteFieldsDis(rowIndex, colIndex);
    cout << "\tWrite output fields raster and flow relationship file ..." << endl;
    cellOrdering.OutputFieldMap(rsfieldFile.c_str());
    cellOrdering.OutputFieldRelationship(txtfileFile.c_str());
}
