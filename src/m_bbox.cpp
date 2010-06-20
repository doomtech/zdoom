// Emacs style mode select	 -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:
//		Main loop menu stuff.
//		Random number LUT.
//		Default Config File.
//		PCX Screenshots.
//
//-----------------------------------------------------------------------------

#include "m_bbox.h"
#include "p_local.h"

void FBoundingBox::AddToBox (fixed_t x, fixed_t y)
{
	if (x < m_Box[BOXLEFT])
		m_Box[BOXLEFT] = x;
	if (x > m_Box[BOXRIGHT])
		m_Box[BOXRIGHT] = x;

	if (y < m_Box[BOXBOTTOM])
		m_Box[BOXBOTTOM] = y;
	if (y > m_Box[BOXTOP])
		m_Box[BOXTOP] = y;
}

//==========================================================================
//
// FBoundingBox :: BoxOnLineSide
//
// Considers the line to be infinite
// Returns side 0 or 1, -1 if box crosses the line.
//
//==========================================================================

int FBoundingBox::BoxOnLineSide (const line_t *ld) const
{
	int p1;
	int p2;
		
	switch (ld->slopetype)
	{
	case ST_HORIZONTAL:
		p1 = m_Box[BOXTOP] > ld->v1->y;
		p2 = m_Box[BOXBOTTOM] > ld->v1->y;
		if (ld->dx < 0)
		{
			p1 ^= 1;
			p2 ^= 1;
		}
		break;
		
	case ST_VERTICAL:
		p1 = m_Box[BOXRIGHT] < ld->v1->x;
		p2 = m_Box[BOXLEFT] < ld->v1->x;
		if (ld->dy < 0)
		{
			p1 ^= 1;
			p2 ^= 1;
		}
		break;
		
	case ST_POSITIVE:
		p1 = P_PointOnLineSide (m_Box[BOXLEFT], m_Box[BOXTOP], ld);
		p2 = P_PointOnLineSide (m_Box[BOXRIGHT], m_Box[BOXBOTTOM], ld);
		break;
		
	case ST_NEGATIVE:
	default:	// Just to assure GCC that p1 and p2 really do get initialized
		p1 = P_PointOnLineSide (m_Box[BOXRIGHT], m_Box[BOXTOP], ld);
		p2 = P_PointOnLineSide (m_Box[BOXLEFT], m_Box[BOXBOTTOM], ld);
		break;
	}

	return (p1 == p2) ? p1 : -1;
}


//==========================================================================
//
// FBoundingBox :: BoxOnLineSide
//
// Considers the line to be infinite
// Returns side 0 or 1, -1 if box crosses the line.
//
//==========================================================================

int FBoundingBox::BoxOnNodeSide (const node_t *nd) const
{
	int p1;
	int p2;

	if (nd->dx == 0) 
	{
		p1 = m_Box[BOXRIGHT] < nd->x;
		p2 = m_Box[BOXLEFT] < nd->x;
		if (nd->dy < 0)
		{
			p1 ^= 1;
			p2 ^= 1;
		}
	}
	else if (nd->dy == 0) 
	{
		p1 = m_Box[BOXTOP] > nd->y;
		p2 = m_Box[BOXBOTTOM] > nd->y;
		if (nd->dx < 0)
		{
			p1 ^= 1;
			p2 ^= 1;
		}
	}
	else if ((nd->dy ^ nd->dx) >= 0)
	{
		p1 = R_PointOnSide (m_Box[BOXLEFT], m_Box[BOXTOP], nd);
		p2 = R_PointOnSide (m_Box[BOXRIGHT], m_Box[BOXBOTTOM], nd);
	}
	else
	{
		p1 = R_PointOnSide (m_Box[BOXRIGHT], m_Box[BOXTOP], nd);
		p2 = R_PointOnSide (m_Box[BOXLEFT], m_Box[BOXBOTTOM], nd);
	}

	return (p1 == p2) ? p1 : -1;
}
