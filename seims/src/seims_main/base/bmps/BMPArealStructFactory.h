/*!
 * \file BMPArealStructFactory.h
 * \brief Areal struct BMP factory
 *
 * Changelog:
 *   - 1. 2017-07-13 - lj - Partially rewrite this class, Scenario data only read from MongoDB.
 *                          \ref DataCenter will perform the data updating.
 *   - 2. 2017-11-29 - lj - Code style review.
 *   - 3. 2018-04-12 - lj - Code reformat.
 *
 * \author Huiran Gao, Liangjun Zhu
 */
#ifndef SEIMS_BMP_AREALSTRUCT_H
#define SEIMS_BMP_AREALSTRUCT_H

#include "tinyxml.h"
#include "basic.h"
#include "data_raster.h"

#include "BMPFactory.h"
#include "ParamInfo.h"

using namespace ccgl;
using namespace bmps;

namespace bmps {
/*!
 * \class bmps::BMPArealStruct
 * \brief Manage areal Structural BMP data, inherited from \ref ParamInfo
 */
class BMPArealStruct: Interface {
public:
    //! Constructor
    BMPArealStruct(const bson_t*& bsonTab, bson_iter_t& iter);
    //! Destructor
    ~BMPArealStruct();
    //! Get name
    string getBMPName() { return m_name; }
    //! Get suitable landuse
    vector<int>& getSuitableLanduse() { return m_landuse; }
    //! Get parameters
    map<string, ParamInfo*>& getParameters() { return m_parameters; }
private:
    int m_id; ///< unique BMP ID
    string m_name; ///< name
    string m_desc; ///< description
    string m_refer; ///< references
    vector<int> m_landuse; ///< suitable placement landuse
    /*!
     * \key the parameter name, remember to add subbasin number as prefix when use GridFS file in MongoDB
     * \value the \ref ParamInfo class
     */
    map<string, ParamInfo*> m_parameters;
};

/*!
 * \class bmps::BMPArealStructFactory
 * \brief Initiate Areal Structural BMPs
 *
 */
class BMPArealStructFactory: public BMPFactory {
public:
    /// Constructor
    BMPArealStructFactory(int scenarioId, int bmpId, int subScenario,
                          int bmpType, int bmpPriority, vector<string>& distribution,
                          const string& collection, const string& location);

    /// Destructor
    ~BMPArealStructFactory();

    //! Load BMP parameters from MongoDB
    void loadBMP(MongoClient* conn, const string& bmpDBName) OVERRIDE;

    //! Set raster data if needed
    void setRasterData(map<string, FloatRaster*>& sceneRsMap) OVERRIDE;

    //! Get management fields data
    float* GetRasterData() OVERRIDE { return m_mgtFieldsRs; };

    //! Get effect unit IDs
    const vector<int>& getUnitIDs() const { return m_unitIDs; }

    //! Get areal BMP parameters
    const map<int, BMPArealStruct*>& getBMPsSettings() const { return m_bmpStructMap; }

    //! Output
    void Dump(std::ostream* fs) OVERRIDE;

private:
    //! management units file name
    string m_mgtFieldsName;
    //! management units raster data
    float* m_mgtFieldsRs;
    //! locations
    vector<int> m_unitIDs;
    /*!
     *\key The unique areal BMP ID
     *\value Instance of \ref BMPArealStruct
     */
    map<int, BMPArealStruct*> m_bmpStructMap;
};
}
#endif /* SEIMS_BMP_AREALSTRUCT_H */
