/*
 * ReactOS Win32 Subsystem
 *
 * Copyright (C) 1998 - 2004 ReactOS Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id: brush.c,v 1.33 2004/04/05 21:26:25 navaraf Exp $
 */

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/bitmaps.h>
#include <win32k/brush.h>
#include <internal/safe.h>
#include <include/object.h>
#include <include/inteng.h>
#include <include/error.h>
#include <include/tags.h>
#define NDEBUG
#include <win32k/debug1.h>

HBRUSH FASTCALL
IntGdiCreateBrushIndirect(PLOGBRUSH LogBrush)
{
   PGDIBRUSHOBJ BrushObject;
   HBRUSH hBrush;
  
   hBrush = BRUSHOBJ_AllocBrush();
   if (hBrush == NULL)
   {
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return NULL;
   }

   BrushObject = BRUSHOBJ_LockBrush(hBrush);

   switch (LogBrush->lbStyle)
   {
      case BS_NULL:
         BrushObject->flAttrs = GDIBRUSH_IS_NULL;
         break;
      
      /* FIXME */
      case BS_HATCHED:

      case BS_SOLID:
         BrushObject->flAttrs = GDIBRUSH_IS_SOLID;
         BrushObject->BrushAttr.lbColor = LogBrush->lbColor & 0xFFFFFF;
         BrushObject->BrushObject.iSolidColor = BrushObject->BrushAttr.lbColor;
         /* FIXME: Fill in the rest of fields!!! */
         break;

      case BS_PATTERN:
         BrushObject->flAttrs = GDIBRUSH_IS_BITMAP;
         BrushObject->hbmPattern = BITMAPOBJ_CopyBitmap((HBITMAP)LogBrush->lbHatch);
         BrushObject->BrushObject.iSolidColor = 0xFFFFFFFF;
         /* FIXME: Fill in the rest of fields!!! */
         break;

      default:
         DPRINT1("Brush Style: %d\n", LogBrush->lbStyle);
         UNIMPLEMENTED;
   }

   BRUSHOBJ_UnlockBrush(hBrush);
   return hBrush;
}

BOOL FASTCALL
IntPatBlt(
   PDC dc,
   INT XLeft,
   INT YLeft,
   INT Width,
   INT Height,
   DWORD ROP,
   PGDIBRUSHOBJ BrushObj)
{
   RECT DestRect;
   PSURFOBJ SurfObj;
   BOOL ret;

   SurfObj = (SURFOBJ *)AccessUserObject((ULONG)dc->Surface);
   if (SurfObj == NULL)
   {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return FALSE;
   }

   ASSERT(BrushObj);
   if (!(BrushObj->flAttrs & GDIBRUSH_IS_NULL))
   {
      if (Width > 0)
      {
         DestRect.left = XLeft + dc->w.DCOrgX;
         DestRect.right = XLeft + Width + dc->w.DCOrgX;
      }
      else
      {
         DestRect.left = XLeft + Width + 1 + dc->w.DCOrgX;
         DestRect.right = XLeft + dc->w.DCOrgX + 1;
      }

      if (Height > 0)
      {
         DestRect.top = YLeft + dc->w.DCOrgY;
         DestRect.bottom = YLeft + Height + dc->w.DCOrgY;
      }
      else
      {
         DestRect.top = YLeft + Height + dc->w.DCOrgY + 1;
         DestRect.bottom = YLeft + dc->w.DCOrgY + 1;
      }

      ret = IntEngBitBlt(
         SurfObj,
         NULL,
         NULL,
         dc->CombinedClip,
         NULL,
         &DestRect,
         NULL,
         NULL,
         &BrushObj->BrushObject,
         NULL,
         ROP);
   }

   return ret;
}

BOOL FASTCALL
IntGdiPolyPatBlt(
   HDC hDC,
   DWORD dwRop,
   PPATRECT pRects,
   int cRects,
   ULONG Reserved)
{
   int i;
   PPATRECT r;
   PGDIBRUSHOBJ BrushObj;
   DC *dc;
	
   dc = DC_LockDc(hDC);
   if (dc == NULL)
   {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return FALSE;
   }
	
   for (r = pRects, i = 0; i < cRects; i++)
   {
      BrushObj = BRUSHOBJ_LockBrush(r->hBrush);
      IntPatBlt(
         dc,
         r->r.left,
         r->r.top,
         r->r.right,
         r->r.bottom,
         dwRop,
         BrushObj);
      BRUSHOBJ_UnlockBrush(r->hBrush);
      r++;
   }

   DC_UnlockDc( hDC );
	
   return TRUE;
}

/* PUBLIC FUNCTIONS ***********************************************************/

HBRUSH STDCALL
NtGdiCreateBrushIndirect(CONST LOGBRUSH *LogBrush)
{
   LOGBRUSH SafeLogBrush;
   NTSTATUS Status;
  
   Status = MmCopyFromCaller(&SafeLogBrush, LogBrush, sizeof(LOGBRUSH));
   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      return 0;
   }
  
   return IntGdiCreateBrushIndirect(&SafeLogBrush);
}

HBRUSH STDCALL
NtGdiCreateDIBPatternBrush(HGLOBAL hDIBPacked, UINT ColorSpec)
{
   UNIMPLEMENTED;
   return 0;
}

HBRUSH STDCALL
NtGdiCreateDIBPatternBrushPt(CONST VOID *PackedDIB, UINT Usage)
{
   UNIMPLEMENTED;
   return 0;
}

HBRUSH STDCALL
NtGdiCreateHatchBrush(INT Style, COLORREF Color)
{
   LOGBRUSH LogBrush;

   if (Style < 0 || Style >= NB_HATCH_STYLES)
      return 0;

   LogBrush.lbStyle = BS_HATCHED;
   LogBrush.lbColor = Color;
   LogBrush.lbHatch = Style;

   return IntGdiCreateBrushIndirect(&LogBrush);
}

HBRUSH STDCALL
NtGdiCreatePatternBrush(HBITMAP hBitmap)
{
   LOGBRUSH LogBrush;

   LogBrush.lbStyle = BS_PATTERN;
   LogBrush.lbColor = 0;
   LogBrush.lbHatch = (ULONG)hBitmap;

   return IntGdiCreateBrushIndirect(&LogBrush);
}

HBRUSH STDCALL
NtGdiCreateSolidBrush(COLORREF Color)
{
   LOGBRUSH LogBrush;

   LogBrush.lbStyle = BS_SOLID;
   LogBrush.lbColor = Color;
   LogBrush.lbHatch = 0;

   return IntGdiCreateBrushIndirect(&LogBrush);
}

BOOL STDCALL
NtGdiFixBrushOrgEx(VOID)
{
   return FALSE;
}

/*
 * NtGdiSetBrushOrgEx
 *
 * The NtGdiSetBrushOrgEx function sets the brush origin that GDI assigns to
 * the next brush an application selects into the specified device context. 
 *
 * Status
 *    @implemented
 */

BOOL STDCALL
NtGdiSetBrushOrgEx(HDC hDC, INT XOrg, INT YOrg, LPPOINT Point)
{
   PDC dc = DC_LockDc(hDC);
   if (dc == NULL)
   {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return FALSE;
   }

   if (Point != NULL)
   {
      POINT SafePoint;
      SafePoint.x = dc->w.brushOrgX;
      SafePoint.y = dc->w.brushOrgY;
      MmCopyToCaller(Point, &SafePoint, sizeof(POINT));
   }

   dc->w.brushOrgX = XOrg;
   dc->w.brushOrgY = YOrg;
   DC_UnlockDc(hDC);

   return TRUE;
}

BOOL STDCALL
NtGdiPolyPatBlt(
   HDC hDC,
   DWORD dwRop,
   PPATRECT pRects,
   INT cRects,
   ULONG Reserved)
{
   PPATRECT rb;
   NTSTATUS Status;
   BOOL Ret;
    
   if (cRects > 0)
   {
      rb = ExAllocatePoolWithTag(PagedPool, sizeof(PATRECT) * cRects, TAG_PATBLT);
      if (!rb)
      {
         SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
         return FALSE;
      }
      Status = MmCopyFromCaller(rb, pRects, sizeof(PATRECT) * cRects);
      if (!NT_SUCCESS(Status))
      {
         ExFreePool(rb);
         SetLastNtError(Status);
         return FALSE;
      }
   }
    
   Ret = IntGdiPolyPatBlt(hDC, dwRop, pRects, cRects, Reserved);
	
   if (cRects > 0)
      ExFreePool(rb);

   return Ret;
}

BOOL STDCALL
NtGdiPatBlt(
   HDC hDC,
   INT XLeft,
   INT YLeft,
   INT Width,
   INT Height,
   DWORD ROP)
{
   PGDIBRUSHOBJ BrushObj;
   DC *dc = DC_LockDc(hDC);
   BOOL ret;

   if (dc == NULL)
   {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return FALSE;
   }

   BrushObj = BRUSHOBJ_LockBrush(dc->w.hBrush);
   if (BrushObj == NULL)
   {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      DC_UnlockDc(hDC);
      return FALSE;
   }

   ret = IntPatBlt(
      dc,
      XLeft,
      YLeft,
      Width,
      Height,
      ROP,
      BrushObj);

   BRUSHOBJ_UnlockBrush(dc->w.hBrush);
   DC_UnlockDc(hDC);

   return ret;
}

/* EOF */
