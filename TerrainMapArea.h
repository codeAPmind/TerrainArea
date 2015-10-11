/*==========================================================================
* @ filename	: TerrainMapArea.h
* @ copyright: Land Cool. 2015. All rights reserved
* @ author : Milong.wu
* @ contact: milongwu@gmail.com
* @ created : 2015-7-15  09:09
* @ describe : get terrain map area && name
* @ version : 1.0
*///==========================================================================

#pragma once

#include <map>
#include <list>
using namespace std;
using namespace stdext;
#include "ISchemeEngine.h"
//#include "TxSysGuiRect.h"
#include "ITerrainMapArea.h"

class TiXmlElement;

// configuration file of area polygon vertex
#define MAPAREA_SCHEME_FILENAME				_T("Scp\\TerrainMapArea.xml")
#define SAVEMAPAREA_SCHEME_FILENAME				_T("\\Data\\Scp\\TerrainMapArea.xml")

// vertex count of a polygon
#define POLYGON_VERTEX_COUNT				30
#define MAP_AREA_LOC_COUNT					4

class CTerrainMapArea : public ITerrainMapArea, public ISchemeUpdateSink
{	
	SUBSYSTEM_DECLARE(CTerrainMapArea,ITerrainMapArea);
	struct SMapAreaInfo
	{
		typedef vector<POINT> Polygon;

		 size_t		nAreaID;						// area map ID
		 size_t		nFatherMapID;			    // father ID, a nearest area which contain it
		// tchar	szAreaName[32];			// Area name
		 tstring szAreaName;
		 size_t		nShowHp;				// 该区域是否显示HP
		 RECT areaMapRect;            //area Rect
		Polygon points;                     //edge points
	};
	struct SMapStateInfo
	{
		struct QueryItem{ 
			short tile; 
			size_t areaid ;
		};
		typedef vector<QueryItem> MapLine;
		typedef vector<MapLine> MapQueryInfo;

		typedef std::vector<size_t> StateAreaList;
	    size_t nStateID;                                           //state ID, key to a map that should be showed independent
		StateAreaList  nStateAreaInfo;  
		MapQueryInfo QueryVertex;                    // query map information
	};
	typedef std::map<size_t,SMapAreaInfo>		TMAP_AREAINFO;          //map of SMapAreaInfo list 
	typedef map<size_t, SMapStateInfo>			TMAP_MAPSTATE;         //map of  SMapStateInfo list map
public:
	 virtual bool Create(void);

	void Destroy();

public:

	virtual bool			OnSchemeLoad(ICSVReader * pCSVReader,const tchar* szFileName);
	virtual bool			OnSchemeUpdate(ICSVReader * pCSVReader, const tchar* szFileName);
	/// purpose: notation when XML file downloading 
	virtual bool        OnSchemeLoad(TiXmlDocument * pTiXmlDocument,const tchar* szFileName);

	/// purpose: notation when XML updating 
	virtual bool        OnSchemeUpdate(TiXmlDocument * pTiXmlDocument, const tchar* szFileName);

	size_t						GetTerrainStateID(size_t nMapID, POINT ptTile) const;
	CTerrainMapArea(void);

	virtual ~CTerrainMapArea(void);
public:
	/// 取区域列表
	/// @param result 返回区域ID列表
	/// @param count result的长度
	virtual size_t getAllAreaIds(size_t * result, size_t count);

	/// 取指定地图的区域
	/// @param nFatherMapID 地图ID
	/// @param result 返回区域ID列表
	/// @param count result的长度
	/// @param allArea 是否返回所有区域，否则只返回直接下级区域
	/// @warning 如果查询某个区域的所有子区域会存在性能问题，请慎用
	virtual size_t getChildAreaID(size_t nFatherMapID, size_t * result, size_t count, bool allArea);

	/// 取指定区域的边界顶点列表
	/// @param nAreaID 区域ID
	/// @param points 返回顶点列表
	/// @param count points缓冲区的长度。
	/// @return 返回拷贝的顶点数量，如果points为NULL或count为0，这返回区域的顶点数量。
	virtual size_t getAreaPoint(size_t nAreaID, POINT * points, size_t count);

	/// 取指定区域的地图区域的范围
	/// @param nAreaID 区域ID
	/// @param rc 区域地图的范围
	virtual  LPCRECT GetAreaBounding(size_t nAreaID)const;

public:
	/// 取指定区域的地图区域的‘包围盒s
	/// @param nAreaID 区域ID
	/// @param rc 区域地图的包围盒
	virtual bool GetBoundingBox(size_t nAreaID, RECT & rc) const;
	/// 取区域名称
	/// @param nFatherMapID 地图编号
	/// @param nAreaID 区域编号
	virtual tstring GetTerrainMapName(size_t nAreaID) const;

	virtual size_t GetTerrainAreaID(size_t nStateID, int x, int y) const;

	//bool writeAreaRect(TiXmlDocument & doc, int areaId, LPCRECT areaRect);

	virtual bool saveas(const tchar * szFile);
	virtual bool save();

	virtual size_t getStateID(size_t nAreaID);
	virtual bool  setAreaRect(size_t nAreaID,LONG &areaLeft, LONG &areaRight, LONG &areaTop, LONG &areaBottom) ;

	virtual bool IsShowHP(int nAreaID);

private:
	
	void initStates();

	//size_t getStateID(size_t areaid);

	void addStateArea(size_t stateid, size_t areaid);

	/* initial state map query information, contains title.x and title's area Id
	* @param mapinfo: is a truct shows a area's infomation such as poins,ids,string names
	* @reurn: none
	* @desc: for each level line in a rect, there record title.x of areas
	*  line  *1   *2     *1 *2   POINT (Area1_edgeTitle1.x, areaid_1)  POINT(Area1_edgeTitle2.x, areaid_1)  
	*                                 POINT (Area2_edgeTitle1.x, areaid_2)  POINT(Area2_edgeTitle2.x, areaid_2)   
	*/
	void initStateQueryInfo(SMapStateInfo & mapinfo);

 /* check if RECT isChild is in RECT isFather
 * @param isChild: child Rect
 * @param isFather: father Rect
 * @return true/ fasle
 */
	 bool rectContain(LPCRECT isChild, LPCRECT isFather);

	 bool isChildOf(size_t nParentID, SMapAreaInfo & areainfo,bool recur);

	 bool writeAreaRectEx(TiXmlElement * elem, LPCRECT areaRect);

private:
	TMAP_AREAINFO			m_mapTerrainMapArea;
	TMAP_MAPSTATE			m_mapTerrainMapState;

	/// 额外提供一个绘制接口，编辑器生成区域地图时调用该接口，绘制区域边界  -by 武斌博
public:
	void Render(IRenderSystem* pRenderSys,int mapID);
};
