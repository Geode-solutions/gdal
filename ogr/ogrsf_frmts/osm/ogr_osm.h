/******************************************************************************
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  Private definitions for OGR/OpenStreeMap driver.
 * Author:   Even Rouault, <even dot rouault at spatialys.com>
 *
 ******************************************************************************
 * Copyright (c) 2012-2014, Even Rouault <even dot rouault at spatialys.com>
 *
 * SPDX-License-Identifier: MIT
 ****************************************************************************/

#ifndef OGR_OSM_H_INCLUDED
#define OGR_OSM_H_INCLUDED

// replace O(log2(N)) complexity of FindNode() by O(1)
#define ENABLE_NODE_LOOKUP_BY_HASHING 1

#include "ogrsf_frmts.h"
#include "cpl_string.h"

#include <array>
#include <set>
#include <unordered_set>
#include <map>
#include <vector>

#include "osm_parser.h"

#include "ogrsqlitevfs.h"

class OGROSMConstCharComp
{
  public:
    bool operator()(const char *a, const char *b) const
    {
        return strcmp(a, b) < 0;
    }
};

class OGROSMComputedAttribute
{
  public:
    CPLString osName{};
    int nIndex = -1;
    OGRFieldType eType = OFTString;
    CPLString osSQL{};
    sqlite3_stmt *hStmt = nullptr;
    std::vector<CPLString> aosAttrToBind{};
    std::vector<int> anIndexToBind{};
    bool bHardcodedZOrder = false;

    OGROSMComputedAttribute() = default;

    explicit OGROSMComputedAttribute(const char *pszName) : osName(pszName)
    {
    }

    OGROSMComputedAttribute(OGROSMComputedAttribute &&) = default;
    OGROSMComputedAttribute &operator=(OGROSMComputedAttribute &&) = default;

  private:
    OGROSMComputedAttribute(const OGROSMComputedAttribute &) = delete;
    OGROSMComputedAttribute &
    operator=(const OGROSMComputedAttribute &) = delete;
};

/************************************************************************/
/*                           OGROSMLayer                                */
/************************************************************************/

class OGROSMDataSource;

class OGROSMLayer final : public OGRLayer
{
    friend class OGROSMDataSource;

    OGROSMDataSource *m_poDS = nullptr;
    int m_nIdxLayer = 0;
    OGRFeatureDefn *m_poFeatureDefn = nullptr;
    OGRSpatialReference *m_poSRS = nullptr;

    std::vector<char *>
        m_apszNames{}; /* Needed to keep a "reference" to the string inserted
                          into oMapFieldNameToIndex */
    std::map<const char *, int, OGROSMConstCharComp> m_oMapFieldNameToIndex{};

    std::vector<OGROSMComputedAttribute> m_oComputedAttributes{};

    bool m_bResetReadingAllowed = false;

    size_t m_nFeatureArrayIndex = 0;
    std::vector<std::unique_ptr<OGRFeature>> m_apoFeatures{};

    bool m_bHasOSMId = false;
    int m_nIndexOSMId = -1;
    int m_nIndexOSMWayId = -1;
    bool m_bHasVersion = false;
    bool m_bHasTimestamp = false;
    bool m_bHasUID = false;
    bool m_bHasUser = false;
    bool m_bHasChangeset = false;
    bool m_bHasOtherTags = true;
    int m_nIndexOtherTags = -1;
    bool m_bHasAllTags = false;
    int m_nIndexAllTags = -1;

    bool m_bHasWarnedTooManyFeatures = false;

    std::string m_osAllTagsBuffer{};

    bool m_bUserInterested = true;

    bool AddToArray(std::unique_ptr<OGRFeature>, bool bCheckFeatureThreshold);

    int AddInOtherOrAllTags(const char *pszK);

    char szLaunderedFieldName[256];
    const char *GetLaunderedFieldName(const char *pszName);

    std::vector<char *> apszInsignificantKeys{};
    std::map<const char *, int, OGROSMConstCharComp> aoSetInsignificantKeys{};

    std::vector<char *> apszIgnoreKeys{};
    std::map<const char *, int, OGROSMConstCharComp> aoSetIgnoreKeys{};

    std::set<std::string> aoSetWarnKeys{};

    OGROSMLayer(const OGROSMLayer &) = delete;
    OGROSMLayer &operator=(const OGROSMLayer &) = delete;

  public:
    OGROSMLayer(OGROSMDataSource *m_poDS, int m_nIdxLayer, const char *pszName);
    virtual ~OGROSMLayer();

    virtual OGRFeatureDefn *GetLayerDefn() override
    {
        return m_poFeatureDefn;
    }

    virtual void ResetReading() override;
    virtual int TestCapability(const char *) override;

    virtual OGRFeature *GetNextFeature() override;

    OGRFeature *MyGetNextFeature(OGROSMLayer **ppoNewCurLayer,
                                 GDALProgressFunc pfnProgress,
                                 void *pProgressData);

    virtual GIntBig GetFeatureCount(int bForce) override;

    virtual OGRErr SetAttributeFilter(const char *pszAttrQuery) override;

    virtual OGRErr IGetExtent(int iGeomField, OGREnvelope *psExtent,
                              bool bForce) override;

    const OGREnvelope *GetSpatialFilterEnvelope();

    bool AddFeature(std::unique_ptr<OGRFeature> poFeature,
                    bool bAttrFilterAlreadyEvaluated,
                    bool *pbFilteredOut = nullptr,
                    bool bCheckFeatureThreshold = true);
    void ForceResetReading();

    void AddField(const char *pszName, OGRFieldType eFieldType,
                  OGRFieldSubType eSubType = OFSTNone);
    int GetFieldIndex(const char *pszName);

    bool HasOSMId() const
    {
        return m_bHasOSMId;
    }

    void SetHasOSMId(bool bIn)
    {
        m_bHasOSMId = bIn;
    }

    bool HasVersion() const
    {
        return m_bHasVersion;
    }

    void SetHasVersion(bool bIn)
    {
        m_bHasVersion = bIn;
    }

    bool HasTimestamp() const
    {
        return m_bHasTimestamp;
    }

    void SetHasTimestamp(bool bIn)
    {
        m_bHasTimestamp = bIn;
    }

    bool HasUID() const
    {
        return m_bHasUID;
    }

    void SetHasUID(bool bIn)
    {
        m_bHasUID = bIn;
    }

    bool HasUser() const
    {
        return m_bHasUser;
    }

    void SetHasUser(bool bIn)
    {
        m_bHasUser = bIn;
    }

    bool HasChangeset() const
    {
        return m_bHasChangeset;
    }

    void SetHasChangeset(bool bIn)
    {
        m_bHasChangeset = bIn;
    }

    bool HasOtherTags() const
    {
        return m_bHasOtherTags;
    }

    void SetHasOtherTags(bool bIn)
    {
        m_bHasOtherTags = bIn;
    }

    bool HasAllTags() const
    {
        return m_bHasAllTags;
    }

    void SetHasAllTags(bool bIn)
    {
        m_bHasAllTags = bIn;
    }

    void SetFieldsFromTags(OGRFeature *poFeature, GIntBig nID, bool bIsWayID,
                           unsigned int nTags, const OSMTag *pasTags,
                           const OSMInfo *psInfo);

    void SetDeclareInterest(bool bIn)
    {
        m_bUserInterested = bIn;
    }

    bool IsUserInterested() const
    {
        return m_bUserInterested;
    }

    int HasAttributeFilter() const
    {
        return m_poAttrQuery != nullptr;
    }

    int EvaluateAttributeFilter(OGRFeature *poFeature);

    void AddInsignificantKey(const char *pszK);

    int IsSignificantKey(const char *pszK) const
    {
        return aoSetInsignificantKeys.find(pszK) ==
               aoSetInsignificantKeys.end();
    }

    void AddIgnoreKey(const char *pszK);
    void AddWarnKey(const char *pszK);

    void AddComputedAttribute(const char *pszName, OGRFieldType eType,
                              const char *pszSQL);
};

/************************************************************************/
/*                        OGROSMDataSource                              */
/************************************************************************/

struct KeyDesc
{
    char *pszK = nullptr;
    int nKeyIndex = 0;
    int nOccurrences = 0;
    std::vector<char *> apszValues{};
    //! map that is the reverse of apszValues
    std::map<const char *, int, OGROSMConstCharComp> anMapV{};
};

typedef struct
{
    short bKIsIndex; /* whether we should use nKeyIndex or
                        nOffsetInpabyNonRedundantKeys */
    short bVIsIndex; /* whether we should use nValueIndex or
                        nOffsetInpabyNonRedundantValues */

    union
    {
        int nKeyIndex; /* index of OGROSMDataSource.asKeys */
        int nOffsetInpabyNonRedundantKeys; /* offset in
                                              OGROSMDataSource.pabyNonRedundantKeys
                                            */
    } uKey;

    union
    {
        int nValueIndex;                     /* index of KeyDesc.apszValues */
        int nOffsetInpabyNonRedundantValues; /* offset in
                                                OGROSMDataSource.pabyNonRedundantValues
                                              */
    } uVal;
} IndexedKVP;

typedef struct
{
    GIntBig nOff;

    /* Note: only one of nth bucket pabyBitmap or panSectorSize must be free'd
     */
    union
    {
        GByte *pabyBitmap;    /* array of BUCKET_BITMAP_SIZE bytes */
        GByte *panSectorSize; /* array of BUCKET_SECTOR_SIZE_ARRAY_SIZE bytes.
                                 Each values means (size in bytes - 8 ) / 2,
                                 minus 8. 252 means uncompressed */
    } u;
} Bucket;

typedef struct
{
    int nLon;
    int nLat;
} LonLat;

struct WayFeaturePair
{
    GIntBig nWayID = 0;
    /* point to a sub-array of OGROSMDataSource.anReqIds */
    GIntBig *panNodeRefs = nullptr;
    unsigned int nRefs = 0;
    unsigned int nTags = 0;
    IndexedKVP *pasTags = nullptr; /*  point to a sub-array of
                            OGROSMDataSource.pasAccumulatedTags */
    OSMInfo sInfo{};
    std::unique_ptr<OGRFeature> poFeature{};
    bool bIsArea = false;
    bool bAttrFilterAlreadyEvaluated = false;
};

#ifdef ENABLE_NODE_LOOKUP_BY_HASHING
typedef struct
{
    int nInd;  /* values are indexes of panReqIds */
    int nNext; /* values are indexes of psCollisionBuckets, or -1 to stop the
                  chain */
} CollisionBucket;
#endif

class OGROSMDataSource final : public GDALDataset
{
    friend class OGROSMLayer;

    std::vector<std::unique_ptr<OGROSMLayer>> m_apoLayers{};

    std::string m_osConfigFile{};

    OGREnvelope m_sExtent{};
    bool m_bExtentValid = false;

    // Starts off at -1 to indicate that we do not know.
    int m_bInterleavedReading = -1;
    OGROSMLayer *m_poCurrentLayer = nullptr;

    OSMContext *m_psParser = nullptr;
    bool m_bHasParsedFirstChunk = false;
    bool m_bStopParsing = false;

    sqlite3_vfs *m_pMyVFS = nullptr;

    sqlite3 *m_hDB = nullptr;
    sqlite3_stmt *m_hInsertNodeStmt = nullptr;
    sqlite3_stmt *m_hInsertWayStmt = nullptr;
    sqlite3_stmt **m_pahSelectNodeStmt = nullptr;
    sqlite3_stmt **m_pahSelectWayStmt = nullptr;
    sqlite3_stmt *m_hInsertPolygonsStandaloneStmt = nullptr;
    sqlite3_stmt *m_hDeletePolygonsStandaloneStmt = nullptr;
    sqlite3_stmt *m_hSelectPolygonsStandaloneStmt = nullptr;
    bool m_bHasRowInPolygonsStandalone = false;

    sqlite3 *m_hDBForComputedAttributes = nullptr;

    int m_nMaxSizeForInMemoryDBInMB = 0;
    bool m_bInMemoryTmpDB = false;
    bool m_bMustUnlink = true;
    CPLString m_osTmpDBName{};

    std::unordered_set<std::string> aoSetClosedWaysArePolygons{};
    int m_nMinSizeKeysInSetClosedWaysArePolygons = 0;
    int m_nMaxSizeKeysInSetClosedWaysArePolygons = 0;

    std::vector<LonLat> m_asLonLatCache{};

    std::array<const char *, 7> m_ignoredKeys = {{"area", "created_by",
                                                  "converted_by", "note",
                                                  "todo", "fixme", "FIXME"}};

    bool m_bReportAllNodes = false;
    bool m_bReportAllWays = false;
    bool m_bTagsAsHSTORE = true;  // if false, as JSON

    bool m_bFeatureAdded = false;

    bool m_bInTransaction = false;

    bool m_bIndexPoints = true;
    bool m_bUsePointsIndex = true;
    bool m_bIndexWays = true;
    bool m_bUseWaysIndex = true;

    std::vector<bool> m_abSavedDeclaredInterest{};
    OGRLayer *m_poResultSetLayer = nullptr;
    bool m_bIndexPointsBackup = false;
    bool m_bUsePointsIndexBackup = false;
    bool m_bIndexWaysBackup = false;
    bool m_bUseWaysIndexBackup = false;

    bool m_bIsFeatureCountEnabled = false;

    bool m_bAttributeNameLaundering = true;

    std::vector<GByte> m_abyWayBuffer{};

    int m_nWaysProcessed = 0;
    int m_nRelationsProcessed = 0;

    bool m_bCustomIndexing = true;
    bool m_bCompressNodes = false;

    unsigned int m_nUnsortedReqIds = 0;
    GIntBig *m_panUnsortedReqIds = nullptr;

    unsigned int m_nReqIds = 0;
    GIntBig *m_panReqIds = nullptr;

#ifdef ENABLE_NODE_LOOKUP_BY_HASHING
    bool m_bEnableHashedIndex = true;
    /* values >= 0 are indexes of panReqIds. */
    /*        == -1 for unoccupied */
    /*        < -1 are expressed as -nIndexToCollisionBuckets-2 where
     * nIndexToCollisionBuckets point to psCollisionBuckets */
    int *m_panHashedIndexes = nullptr;
    CollisionBucket *m_psCollisionBuckets = nullptr;
    bool m_bHashedIndexValid = false;
#endif

    LonLat *m_pasLonLatArray = nullptr;

    IndexedKVP *m_pasAccumulatedTags =
        nullptr; /* points to content of pabyNonRedundantValues or
                    aoMapIndexedKeys */
    int m_nAccumulatedTags = 0;
    unsigned int MAX_INDEXED_KEYS = 0;
    GByte *pabyNonRedundantKeys = nullptr;
    int nNonRedundantKeysLen = 0;
    unsigned int MAX_INDEXED_VALUES_PER_KEY = 0;
    GByte *pabyNonRedundantValues = nullptr;
    int nNonRedundantValuesLen = 0;
    std::vector<WayFeaturePair> m_asWayFeaturePairs{};

    std::vector<KeyDesc *> m_apsKeys{};
    std::map<const char *, KeyDesc *, OGROSMConstCharComp>
        m_aoMapIndexedKeys{}; /* map that is the reverse of asKeys */

    CPLString m_osNodesFilename{};
    bool m_bInMemoryNodesFile = false;
    bool m_bMustUnlinkNodesFile = true;
    GIntBig m_nNodesFileSize = 0;
    VSILFILE *m_fpNodes = nullptr;

    GIntBig m_nPrevNodeId = -INT_MAX;
    int m_nBucketOld = -1;
    int m_nOffInBucketReducedOld = -1;
    GByte *m_pabySector = nullptr;
    std::map<int, Bucket> m_oMapBuckets{};
    Bucket *GetBucket(int nBucketId);

    bool m_bNeedsToSaveWayInfo = false;

    static const GIntBig FILESIZE_NOT_INIT = -2;
    static const GIntBig FILESIZE_INVALID = -1;
    GIntBig m_nFileSize = FILESIZE_NOT_INIT;

    void CompressWay(bool bIsArea, unsigned int nTags,
                     const IndexedKVP *pasTags, int nPoints,
                     const LonLat *pasLonLatPairs, const OSMInfo *psInfo,
                     std::vector<GByte> &abyCompressedWay);
    void UncompressWay(int nBytes, const GByte *pabyCompressedWay,
                       bool *pbIsArea, std::vector<LonLat> &asCoords,
                       unsigned int *pnTags, OSMTag *pasTags, OSMInfo *psInfo);

    bool ParseConf(CSLConstList papszOpenOptions);
    bool CreateTempDB();
    bool SetDBOptions();
    void SetCacheSize();
    bool CreatePreparedStatements();
    void CloseDB();

    bool IndexPoint(const OSMNode *psNode);
    bool IndexPointSQLite(const OSMNode *psNode);
    bool FlushCurrentSector();
    bool FlushCurrentSectorCompressedCase();
    bool FlushCurrentSectorNonCompressedCase();
    bool IndexPointCustom(const OSMNode *psNode);

    void IndexWay(GIntBig nWayID, bool bIsArea, unsigned int nTags,
                  const IndexedKVP *pasTags, const LonLat *pasLonLatPairs,
                  int nPairs, const OSMInfo *psInfo);

    bool StartTransactionCacheDB();
    bool CommitTransactionCacheDB();

    int FindNode(GIntBig nID);
    void ProcessWaysBatch();

    void ProcessPolygonsStandalone();

    void LookupNodes();
    void LookupNodesSQLite();
    void LookupNodesCustom();
    void LookupNodesCustomCompressedCase();
    void LookupNodesCustomNonCompressedCase();

    unsigned int
    LookupWays(std::map<GIntBig, std::pair<int, void *>> &aoMapWays,
               const OSMRelation *psRelation);

    OGRGeometry *BuildMultiPolygon(const OSMRelation *psRelation,
                                   unsigned int *pnTags, OSMTag *pasTags);
    OGRGeometry *BuildGeometryCollection(const OSMRelation *psRelation,
                                         bool bMultiLineString);

    bool TransferToDiskIfNecesserary();

    Bucket *AllocBucket(int iBucket);

    void AddComputedAttributes(
        int iCurLayer, const std::vector<OGROSMComputedAttribute> &oAttributes);
    bool IsClosedWayTaggedAsPolygon(unsigned int nTags, const OSMTag *pasTags);

    OGROSMDataSource(const OGROSMDataSource &) = delete;
    OGROSMDataSource &operator=(const OGROSMDataSource &) = delete;

  public:
    OGROSMDataSource();
    virtual ~OGROSMDataSource();

    virtual int GetLayerCount() override
    {
        return static_cast<int>(m_apoLayers.size());
    }

    virtual OGRLayer *GetLayer(int) override;

    virtual int TestCapability(const char *) override;

    virtual OGRLayer *ExecuteSQL(const char *pszSQLCommand,
                                 OGRGeometry *poSpatialFilter,
                                 const char *pszDialect) override;
    virtual void ReleaseResultSet(OGRLayer *poLayer) override;

    virtual void ResetReading() override;
    virtual OGRFeature *GetNextFeature(OGRLayer **ppoBelongingLayer,
                                       double *pdfProgressPct,
                                       GDALProgressFunc pfnProgress,
                                       void *pProgressData) override;

    int Open(const char *pszFilename, CSLConstList papszOpenOptions);

    int MyResetReading();
    bool ParseNextChunk(int nIdxLayer, GDALProgressFunc pfnProgress,
                        void *pProgressData);
    OGRErr GetNativeExtent(OGREnvelope *psExtent);
    int IsInterleavedReading();

    void NotifyNodes(unsigned int nNodes, const OSMNode *pasNodes);
    void NotifyWay(const OSMWay *psWay);
    void NotifyRelation(const OSMRelation *psRelation);
    void NotifyBounds(double dfXMin, double dfYMin, double dfXMax,
                      double dfYMax);

    OGROSMLayer *GetCurrentLayer()
    {
        return m_poCurrentLayer;
    }

    void SetCurrentLayer(OGROSMLayer *poLyr)
    {
        m_poCurrentLayer = poLyr;
    }

    bool IsFeatureCountEnabled() const
    {
        return m_bIsFeatureCountEnabled;
    }

    bool DoesAttributeNameLaundering() const
    {
        return m_bAttributeNameLaundering;
    }
};

#endif /* ndef OGR_OSM_H_INCLUDED */
