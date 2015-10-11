/*==========================================================================
* @ filename	: TerrainMapArea.cpp
* @ copyright: Land Cool. 2015. All rights reserved
* @ author : Milong.wu
* @ contact: milongwu@gmail.com
* @ created : 2015-7-15  09:09
* @ describe : get terrain map area && name
* @ version : 1.0
*///==========================================================================

#include "stdafx.h"
#include "TerrainMapArea.h"
#include "FacadeAPI.h"
#include "WinGDI.h"
#include "ReMath.h"

#include <stack>
#include "ITerrainMapArea.h"
#include "SubSystemIDs.h"
#include "IGlobalClient.h"
#include "tinyxml.h"
#include "TraceModuleId.h"
#include "Trace.h"
#include "path.h"

#include "IRenderSystem.h"
#include "IRenderEngine.h"
#include "ISceneManager.h"



SUBSYSTEM_REGISTER(CTerrainMapArea,ITerrainMapArea,false);

ColorValue gRenderColorValue = ColorValue::Blue;
uint gRenderColor = gRenderColorValue.getAsARGB();

CTerrainMapArea::CTerrainMapArea(void)
{	
	m_mapTerrainMapArea.clear();
	m_mapTerrainMapState.clear();
}

CTerrainMapArea::~CTerrainMapArea(void)
{
   m_mapTerrainMapArea.clear();
   m_mapTerrainMapState.clear();
}

bool CTerrainMapArea::Create(void)
{
	ISchemeEngine *pSchemeEngine = CreateSchemeEngineProc();

	if(!pSchemeEngine)
	{
		return false;
	}
	tstring strPath = MAPAREA_SCHEME_FILENAME;
	bool result =  pSchemeEngine->LoadScheme(strPath.c_str(),(ISchemeUpdateSink *)this);
	if (!result)
	{
			Error(ModuleNone, _GT("加载配置文件失败。文件名 = ") << strPath.c_str() << endl);
			return false;
	}
	return true;
}
void CTerrainMapArea::Destroy(void)
{	
	// 所有地形地图区域
	MTRACELN(_T("关闭区域系统"));
	m_mapTerrainMapArea.swap(TMAP_AREAINFO());	
	m_mapTerrainMapState.swap(TMAP_MAPSTATE());
}
//bool CTerrainMapArea::OnSchemeLoad(ICSVReader * pCSVReader,LPCTSTR szFileName)
bool CTerrainMapArea::OnSchemeLoad(TiXmlDocument * pTiXmlDocument,LPCTSTR szFileName)
{
	TiXmlElement* root = pTiXmlDocument->RootElement();
	if(!root)return false;

	for(TiXmlElement* child = root->FirstChildElement();child;child=child->NextSiblingElement())
	{
		SMapAreaInfo mapareainfo;

		int areaid;
		child->Attribute(_T("areaID"),&areaid);
		mapareainfo.nAreaID = areaid;

        int mapid;
		child->Attribute(_T("fatherID"),&mapid);
        mapareainfo.nFatherMapID = mapid;
		mapareainfo.szAreaName = child->Attribute(_T("areaName"));
		int nShowHP = 0;
		child->Attribute(_T("showHP"),&nShowHP);
		mapareainfo.nShowHp		= nShowHP;
		RECT rect;
        child->Attribute(_T("areaLeft"),(int *)&rect.left);
		child->Attribute(_T("areaRight"),(int *)&rect.right);
		child->Attribute(_T("areaTop"),(int *)&rect.top);
		child->Attribute(_T("areaBottom"),(int *)&rect.bottom);

		mapareainfo.areaMapRect = rect;

		size_t nVertexCount = 0;
		for(TiXmlElement *areaEdge = child->FirstChildElement();areaEdge;areaEdge = areaEdge->NextSiblingElement())
		{
			size_t ptVertex[2];
			areaEdge->Attribute(_T("locx"),(int*)&ptVertex[0]);
			areaEdge->Attribute(_T("locy"),(int*)&ptVertex[1]);

			POINT pt = {ptVertex[0],ptVertex[1]};
			mapareainfo.points.push_back(pt);
			nVertexCount++;
		}
		if(nVertexCount<3){
			return false;
		}
		m_mapTerrainMapArea.insert(std::pair<size_t,SMapAreaInfo>(mapareainfo.nAreaID,mapareainfo));
	}

	initStates();
	return true;
} 


/** loading notation for XML files
 * @ param :  pCSVReader
 * @ param :  szFileName   configuration file name
 * @ return  :  bool
 * @ warning :don't release  pCSVReader or pTiXmlDocument, for there are several sinks in a file
 * @ retval buffer 
*/ 
/*
 * @ desc: in the vector a area is after its father area 
 */
void CTerrainMapArea::initStates()
{
	std::set<size_t> areas;
	for ( TMAP_AREAINFO::iterator it = m_mapTerrainMapArea.begin(); it != m_mapTerrainMapArea.end(); ++it)
	{
		size_t stateid = getStateID(it->second.nAreaID) ;
		// each of map ids is bigger than 0
		if ( stateid > 0)
			addStateArea(getStateID(it->first), it->first);
	}

	for (TMAP_MAPSTATE::iterator it = m_mapTerrainMapState.begin(); it != m_mapTerrainMapState.end(); ++it){
		initStateQueryInfo(it->second);
	}
}
/** getstateID  function
 * @ param areaid:   a area ID needs a father ID 
 * @ return nAreaID: the father ID
 * @ desc: get a state ID, that's a map should be showed independent
 */
size_t CTerrainMapArea::getStateID(size_t areaid)
{
	TMAP_AREAINFO::iterator it = m_mapTerrainMapArea.find(areaid);
	// all map id is bigger than 0
	while(it != m_mapTerrainMapArea.end() && it->second.nFatherMapID >0){
		it = m_mapTerrainMapArea.find(it->second.nFatherMapID);
	}
	if (it == m_mapTerrainMapArea.end()){
		MERRORLN(_T("区域")<<areaid<<_T("不在任何场景地图中."));
		return -1;
	}
	// 1,-1.     so 1 (nAreaID) is the state ID 
	return it->second.nAreaID;
}

void CTerrainMapArea::addStateArea(size_t stateid, size_t areaid)
{
	TMAP_MAPSTATE::iterator it = m_mapTerrainMapState.find(stateid);
	if (it == m_mapTerrainMapState.end()){
		SMapStateInfo stateinfo;
		stateinfo.nStateID = stateid;
		std::pair<TMAP_MAPSTATE::iterator,bool> result = m_mapTerrainMapState.insert(std::pair<size_t,SMapStateInfo>(stateid,stateinfo));
		if (!result.second){
			MERRORLN(_T("添加地图")<<stateid<<_T("失败"));
			return ;
		}
		it = result.first;
	}

	it->second.nStateAreaInfo.push_back(areaid);
}

bool compare_queryitem(CTerrainMapArea::SMapStateInfo::QueryItem & lhs, CTerrainMapArea::SMapStateInfo::QueryItem & rhs)
{
	return lhs.tile < rhs.tile;
}

//bool CTerrainMapArea::writeAreaRect(int areaId, LPCRECT areaRect)
//{
//	TMAP_AREAINFO::iterator it = m_mapTerrainMapArea.find(areaId);
//	if (it == m_mapTerrainMapArea.end())
//		return false;
//	it->second.areaMapRect = areaRect;
//	return true;
//}

bool CTerrainMapArea::save()
{
	tstring strFilePathDir = rkt::getBasePath();	// 工作目录
	strFilePathDir += SAVEMAPAREA_SCHEME_FILENAME;
	return saveas(strFilePathDir.c_str());
}

bool CTerrainMapArea::saveas(const tchar * szFile)
{
	TiXmlDocument *doc = new TiXmlDocument();
    TiXmlDeclaration * decl = new TiXmlDeclaration(_T("1.0"),_T("gb2312"),_T("no"));
	doc->LinkEndChild(decl);
	TiXmlElement * root = new TiXmlElement(_T("scheme"));
	if(root != NULL)
	{
		for (TMAP_AREAINFO::iterator it = m_mapTerrainMapArea.begin(); it != m_mapTerrainMapArea.end(); ++it){
			TiXmlElement * child = new TiXmlElement(_T("area")); 
			child->SetAttribute(_T("areaName"),it->second.szAreaName.c_str());
			child->SetAttribute(_T("areaID"),it->second.nAreaID);
			child->SetAttribute(_T("fatherID"),it->second.nFatherMapID);
			child->SetAttribute(_T("areaLeft"),it->second.areaMapRect.left);
			child->SetAttribute(_T("areaRight"),it->second.areaMapRect.right);
			child->SetAttribute(_T("areaTop"),it->second.areaMapRect.top);
			child->SetAttribute(_T("areaBottom"),it->second.areaMapRect.bottom);
			int end = it->second.points.size();
			for (int i=0; i< end;++i)
			{
				TiXmlElement * areaEdge = new TiXmlElement(_T("areaEdge")); 
				areaEdge->SetAttribute(_T("locx"),it->second.points[i].x);
				areaEdge->SetAttribute(_T("locy"),it->second.points[i].y);
				child->LinkEndChild(areaEdge);
			}
			root->LinkEndChild(child);
		}
		doc->LinkEndChild(root);
	}
	if(!doc->SaveFile(szFile)){
		MERRORLN(_T("Terrainmaparea 写入文件失败."));
		return false;
	}
	return true;
}

bool CTerrainMapArea::writeAreaRectEx(TiXmlElement * elem, LPCRECT areaRect)
{
		  TiXmlElement *child = new TiXmlElement(_T("areaRect"));
		  child->SetAttribute(_T("areaRect.left"),areaRect->left);
		  child->SetAttribute(_T("areaRect.right"),areaRect->right);
		  child->SetAttribute(_T("areaRect.top"),areaRect->top);
		  child->SetAttribute(_T("areaRect.bottom"),areaRect->bottom);
		  elem->LinkEndChild(child);
		  return true;

}
/* initial state map query information, contains title.x and title's area Id
* @param mapinfo: is a truct shows a area's infomation such as poins,ids,string names
* @reurn: none
* @desc: for each level line in a rect, there record title.x of areas
*  line  *1   *2     *1 *2   POINT (Area1_edgeTitle1.x, areaid_1)  POINT(Area1_edgeTitle2.x, areaid_1)  
*                                 POINT (Area2_edgeTitle1.x, areaid_2)  POINT(Area2_edgeTitle2.x, areaid_2)   
*/
void CTerrainMapArea::initStateQueryInfo(SMapStateInfo & mapinfo)
{
	TMAP_AREAINFO::iterator it = m_mapTerrainMapArea.find(mapinfo.nStateID);
	//RECT rect = it->second.areaMapRect;
	RECT boundingBox;
	GetBoundingBox(mapinfo.nStateID, boundingBox);
	for (int y = boundingBox.top; y <= boundingBox.bottom; ++y){
		SMapStateInfo::MapLine line;
		for (SMapStateInfo::StateAreaList::iterator  it = mapinfo.nStateAreaInfo.begin(); it!= mapinfo.nStateAreaInfo.end(); ++it){
			TMAP_AREAINFO::iterator itArea = m_mapTerrainMapArea.find(*it);
			SMapAreaInfo area = itArea->second;
            
			for (size_t index = 0; index < area.points.size(); ++index){
				POINT beg = area.points[index];
				POINT end;
				if (index == area.points.size()-1) { end = area.points[0];}
				else{	end = area.points[index+1];}
				POINT prev ;
				if (index == 0){prev = area.points[area.points.size()-1];}
				else {prev = area.points[index-1];}
				POINT next;
				if (index >= area.points.size()-2){next = area.points[index+2-area.points.size()];}
				else{ next = area.points[index+2];}

				float x =0;

				if(y == beg.y)
				{
					//case 1:: in a line
					if (y == end.y&&y == prev.y){
						continue;
					}
					// case 2: beg.y is a min(it's former, itself, it later) or max, continue
					else if (prev.y > beg.y &&end.y>beg.y || (prev.y < beg.y &&end.y<beg.y) ){
					      continue;		
					}
					//case 3: record point is in line  Y=y;
					x= (float)beg.x;
				}
				else{
					int miny = std::min<int>(beg.y,end.y);
					int maxy = std::max<int>(beg.y,end.y);
					if (y <= miny || y >= maxy)
						continue;
					x = beg.x + 1.0f*(end.x-beg.x)*(y-beg.y)/(end.y-beg.y);
				}
				SMapStateInfo::QueryItem item = {(size_t)x,(size_t)*it};
				line.push_back(item);
			}
		}
		std::sort(line.begin(),line.end(),compare_queryitem);
		mapinfo.QueryVertex.push_back(line);
	}
}

/** loading notation for CSV files
 * @ param   pCSVReader：读取CSV的返回接口
 * @ param   szFileName：configuration file name
 * @ return  
 * @ warning 不要在此方法释放pCSVReader或者pTiXmlDocument,因为一个文件对应多个sink
 * @ retval buffer 
*/ 
bool CTerrainMapArea::OnSchemeUpdate(ICSVReader * pCSVReader, LPCTSTR szFileName)
{
	return false;
}

bool CTerrainMapArea::OnSchemeLoad(ICSVReader * pCSVReader, LPCTSTR szFileName)
{
	return false;
}

//purpose: notation when XML updating 
bool CTerrainMapArea::OnSchemeUpdate(TiXmlDocument * pTiXmlDocument, const tchar* szFileName)
{
	return true;
}

/*  get area name
 * @ param nAreaId :
 * @ return  string: name of area 
*/
tstring CTerrainMapArea::GetTerrainMapName(size_t nAreaID) const
{	
	TMAP_AREAINFO::const_iterator it = m_mapTerrainMapArea.find(nAreaID);
	if(it != m_mapTerrainMapArea.end())
	{
		return it->second.szAreaName;
	}
	else{
		return _T("");
	}
}

/**get Title's area ID
 * @ param  x,y:  is title 
 * @ param  nStateId: the title in map ID   
 * @ return : title's area ID
*/
size_t CTerrainMapArea::GetTerrainAreaID(size_t nStateID, int x, int y)const
{	
	//case 1 title or stateID is error return 0
	if(x<0 || y<0 || nStateID <=0)
		return 0;
	//case 2: title is not in map of nStateID return 0
    RECT rc;
	if(GetBoundingBox(nStateID,rc))
	{
		// for safety consideration, y = limitation should be deleted,  
		// as it may be a vertex point that not in linenum
		if(x<rc.left || x > rc.right || y<=rc.top || y >= rc.bottom)
			return 0;
	}
	TMAP_MAPSTATE::const_iterator it = m_mapTerrainMapState.find(nStateID);
	if (it == m_mapTerrainMapState.end())
		return 0;
	// put in case 2
	//if (y >=(int)it->second.QueryVertex.size())
	//	return 0;
	SMapStateInfo::MapLine const & line = it->second.QueryVertex[y];

	stack<size_t> stackArearId;
	size_t formerID = 0;
	size_t index = 0;
	//case x <min( line[].titles) or x > max(line.titles) is out of consideration
	while(x > line[index].tile ){
		if ( line[index].areaid !=formerID){
			stackArearId.push(line[index].areaid);
		}
		else{
			stackArearId.pop();
		}
		formerID = line[index].areaid;
		index++;
	}
	if(stackArearId.size() >0){
		return stackArearId.top();
	}
	else{
		return 0;
	}
}

/* return area's region which shows in it's father Map
 * @param nAreaId : area's ID
 * @return  LPRECT: a region 
 */
 LPCRECT CTerrainMapArea::GetAreaBounding(size_t nAreaID) const
{
	TMAP_AREAINFO::const_iterator itP = m_mapTerrainMapArea.find(nAreaID);
	if(itP != m_mapTerrainMapArea.end())
	{
		return &itP->second.areaMapRect;
	}
	else{
	   return 0;
	}
}
/** get all area ID
 * @param result: a vector to input all area IDs
 * @param count: maxnum of the vector
 * @return num: number of IDs
 */
 size_t CTerrainMapArea::getAllAreaIds(size_t * result, size_t count)
 {
	 if (result == 0 || count == 0)
		 return m_mapTerrainMapArea.size();
	 size_t num = 0; 
	 size_t index =0;
	 for (TMAP_AREAINFO::iterator it = m_mapTerrainMapArea.begin(); it != m_mapTerrainMapArea.end(); ++it)
	 {
		 if (num < count){
			 result[index++] = it->first;
			 ++num;
		 }else{
			 break;
		 }
	 }
	 return num;
 }
 /* check if RECT isChild is in RECT isFather
 * @param isChild: child Rect
 * @param isFather: father Rect
 * @return true/ fasle
 */
 bool CTerrainMapArea::rectContain(const LPCRECT isChild,const LPCRECT isFather)
 {
	 if(isFather->bottom >= isChild->bottom && isFather->top <= isChild->top 
		&& isFather->left<=isChild->left && isFather->right >=isChild->right)
		return true;
	 else
		 return false;
 }
 /*to see if areainfo is a child of nparentid
 * @param nparentid: that is a father map's id
 * @param areainfo: contain informations, including areaid, fatherid, and so on, of a area
 * @param recur: wheather need to find if nparendtid is a grandfather of areainfo.id
 * @return true/ fasle;
 */
 bool CTerrainMapArea::isChildOf(size_t nParentID, SMapAreaInfo & areainfo,bool recur)
 {
	 if (nParentID == areainfo.nFatherMapID)
		 return true;
	 if (recur){
		 TMAP_AREAINFO::iterator it = m_mapTerrainMapArea.find(areainfo.nFatherMapID);
		 if (it == m_mapTerrainMapArea.end())
			 return false;
		 return isChildOf(nParentID,it->second,recur);
	 }
	 return false;
 }

 /*get area's father map IDs
 * @param nFatherMapId: that is a father map's id
 * @param result: contain  the child area Ids, 
 * @param count: the number of  area Ids
 * @allArea: true means nFatherMap may be a grandfather,   false means nFather
 * @noting : if result== null, or connt <=0,return a needed size of points 
 */
 size_t CTerrainMapArea::getChildAreaID(size_t nFatherMapID, size_t * result, size_t count, bool allArea)
 {
	 size_t index =0;

	 for(TMAP_AREAINFO::iterator it = m_mapTerrainMapArea.begin(); it != m_mapTerrainMapArea.end(); ++it){
		 if (isChildOf(nFatherMapID,it->second,allArea)){
			 if(result && index < count)
				 result[index++] = it->second.nAreaID;
			 else if(count <=0 || !result)
				 index++;
		 }
	 }
	 return index;
 }

 /** get area edge points
 * @param nAreaId: areaId
 * @param points :  vector to contain the points
 * @param count: the size of vector
 * @noting : if points == NULL or count <=0, return a needed size of points 
 */
 size_t CTerrainMapArea::getAreaPoint(size_t nAreaID, POINT * points, size_t count)
 {
	 TMAP_AREAINFO::iterator it = m_mapTerrainMapArea.find(nAreaID);
	 size_t nCount = 0;
	 if(it != m_mapTerrainMapArea.end())
	 {
		 size_t  itVend =  it->second.points.size();
		 for(size_t itV=0; itV < itVend; ++itV){
			 if(nCount < count && points){
				 points[itV].x = it->second.points[itV].x;
				 points[itV].y = it->second.points[itV].y;
				 ++nCount;
			 }
			 else if(!points || count <=0)
				 ++nCount;
		 }
	 }
	 return nCount;
 }
 /* set area rect info
  * @ param arealeft: RECT.left
  * @ return bool
 */
 bool CTerrainMapArea::setAreaRect(size_t nAreaID,LONG &areaLeft, LONG &areaRight, LONG &areaTop, LONG &areaBottom) 
 {
	  	 TMAP_AREAINFO::iterator it = m_mapTerrainMapArea.find(nAreaID);
         if(it != m_mapTerrainMapArea.end())
		 {
               it->second.areaMapRect.left = areaLeft;
			   it->second.areaMapRect.right = areaRight;
			   it->second.areaMapRect.top = areaTop;
			   it->second.areaMapRect.bottom = areaBottom;
			   return true;
		 }
		 return false;
 }

/* return a smallest rect which contains this area
 * @param nAreaId : area's ID
 * @param rc : a RECT 
 * @return: true/fasle
 */
 bool CTerrainMapArea::GetBoundingBox(size_t nAreaID, RECT & rc) const
 {
	 if(nAreaID < 0)
		 return false;
	 TMAP_AREAINFO::const_iterator it = m_mapTerrainMapArea.find(nAreaID);
	 int leftVal = 0; int rightVal = 0; 
	 int topVal =0; int  bottomVal = 0;
	 POINT tempPt;
	 if(it != m_mapTerrainMapArea.end())
	 {
		 leftVal = rightVal = it->second.points[0].x;
		 topVal = bottomVal = it->second.points[0].y;

		 size_t indexEnd =  it->second.points.size();
		 for(size_t index =1; index < indexEnd; index++)
		 {
			  tempPt = it->second.points[index];
			  leftVal = std::min<int>(tempPt.x,leftVal);
			  rightVal = std::max<int>(tempPt.x,rightVal);
			  topVal = std::min<int>(tempPt.y, topVal);
			  bottomVal = std::max<int>(tempPt.y, bottomVal);
		 }
         rc.left = leftVal;
		 rc.right = rightVal;
		 rc.bottom = bottomVal;
		 rc.top = topVal;
		 return true;
	 }
	 else{
       return   false;
	 }
}


 void CTerrainMapArea::Render(IRenderSystem* pRenderSys,int mapID)
 {
	/*
	纯粹在编辑器生成区域地图时才需要调用这个Render
	*/ 

	 IRenderSystem *renderSys = getRenderEngine()->getRenderSystem();

	 size_t areaNum = getChildAreaID(mapID,NULL,0,true);	
	 if(areaNum==0)
		 return;

	 size_t* result = new size_t[areaNum];

	 getChildAreaID(mapID,result,areaNum,true);	

	 for(size_t i=0; i< areaNum ; ++i)
	 {
		 const int& areaID = result[i];

		 TMAP_AREAINFO::iterator it = m_mapTerrainMapArea.find(areaID);
		 size_t nCount = 0;
		 Vector3 pt1,pt2;
		 if(it != m_mapTerrainMapArea.end())
		 {
			 const SMapAreaInfo& info = it->second;

			 size_t count = info.points.size();

			 for(size_t j =0; j < count ; ++j)
			 {				 
				 if(j==count-1)
				 {
					 SUBSYSTEM(ISceneManager)->tile2Space(info.points[j],pt1);
					 SUBSYSTEM(ISceneManager)->tile2Space(info.points[0],pt2);
					 pt1.y=1000.0f;
					 pt2.y=1000.0f;
					 pRenderSys->line(pt1,pt2,gRenderColorValue);
				 }
				 else
				 {				
					 SUBSYSTEM(ISceneManager)->tile2Space(info.points[j],pt1);
					 SUBSYSTEM(ISceneManager)->tile2Space(info.points[j+1],pt2);
					 pt1.y=1000.0f;
					 pt2.y=1000.0f;
					 pRenderSys->line(pt1,pt2,gRenderColorValue);
				 }
			 }
		 }
	 }

	 delete[] result;

 }

 bool CTerrainMapArea::IsShowHP(int nAreaID)
 {
	TMAP_AREAINFO::const_iterator it = m_mapTerrainMapArea.find(nAreaID);
	if (it == m_mapTerrainMapArea.end())
	{
		return false;
	}
	if (0 == (*it).second.nShowHp)
	{
		return false;
	}
	return true;
 }