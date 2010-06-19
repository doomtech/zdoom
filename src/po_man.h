#ifndef __PO_MAN_H
#define __PO_MAN_H

#include "tarray.h"
#include "r_defs.h"
#include "m_bbox.h"

//
// Linked lists of polyobjects
//
struct FPolyObj;
struct FPolyNode
{
	FPolyObj *poly;				// owning polyobject
	FPolyNode *pnext;			// next polyobj in list
	FPolyNode **pprev;			// previous polyobj

	subsector_t *subsector;		// containimg subsector
	FPolyNode *snext;			// next subsector
	FPolyNode **sprev;			// previous subsector

	TArray<seg_t> segs;			// segs for this node
};

struct FPolyVertex
{
	fixed_t x, y;
};

// ===== Polyobj data =====
struct FPolyObj
{
	TArray<side_t *>		Sidedefs;
	TArray<line_t *>		Linedefs;
	TArray<vertex_t *>		Vertices;
	TArray<FPolyVertex>		OriginalPts;
	TArray<FPolyVertex>		PrevPts;
	FPolyVertex				StartSpot;
	FBoundingBox			Bounds;	// Bounds in map coordinates 

	angle_t		angle;
	int			tag;			// reference tag assigned in HereticEd
	int			bbox[4];		// bounds in blockmap coordinates
	int			validcount;
	int			crush; 			// should the polyobj attempt to crush mobjs?
	bool		bHurtOnTouch;	// should the polyobj hurt anything it touches?
	int			seqType;
	fixed_t		size;			// polyobj size (area of POLY_AREAUNIT == size of FRACUNIT)
	FPolyNode	*subsectorlinks;
	DThinker	*specialdata;	// pointer to a thinker, if the poly is moving
	TObjPtr<DInterpolation> interpolation;

/*
	int			numsegs;
	seg_t		**segs;
	int			numlines;
	line_t		**lines;
	int			numvertices;
	vertex_t	**vertices;
	fixed_t		startSpot[2];
	vertex_t	*originalPts;	// used as the base for the rotations
	vertex_t	*prevPts; 		// use to restore the old point values
*/

	FPolyObj()
	{
		StartSpot.x = StartSpot.y = 0;
		angle = 0;
		tag = 0;
		memset(bbox, 0, sizeof(bbox));
		validcount = 0;
		crush = 0;
		bHurtOnTouch = false;
		seqType = 0;
		size = 0;
		subsectorlinks = NULL;
		specialdata = NULL;
	}

	~FPolyObj();
	DInterpolation *SetInterpolation();
	void StopInterpolation();
};
extern FPolyObj *polyobjs;		// list of all poly-objects on the level

inline FArchive &operator<< (FArchive &arc, FPolyObj *&poly)
{
	return arc.SerializePointer (polyobjs, (BYTE **)&poly, sizeof(FPolyObj));
}

inline FArchive &operator<< (FArchive &arc, const FPolyObj *&poly)
{
	return arc.SerializePointer (polyobjs, (BYTE **)&poly, sizeof(FPolyObj));
}

struct polyblock_t
{
	FPolyObj *polyobj;
	struct polyblock_t *prev;
	struct polyblock_t *next;
};



#endif