/* $Id: pgfx.h,v 1.15 2001/07/31 06:25:43 micahjd Exp $
 *
 * picogui/pgfx.h - The PicoGUI abstract graphics interface
 * 
 * This is a thin wrapper providing a set of primitives that can render to
 * a canvas (persistant or immediate mode) and other fast graphics interfaces
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000,2001 Micah Dowty <micahjd@users.sourceforge.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 * 
 * Contributors: 
 * 
 * 
 * 
 */

/*! 
 * \file pgfx.h
 * \brief PGFX Graphics API Header
 * 
 * PGFX is an abstract interface to graphics primitives. It provides a
 * higher level method of using the Canvas widget and other graphics
 * output devices.
 * Usually this file does not need to be included
 * separately, it is included with <tt>\#include <picogui.h></tt>
 */

#ifndef _H_PG_PGFX
#define _H_PG_PGFX

/*!
 * \file pgfx.h
 * \brief PGFX Graphics Interface
 *
 * The PGFX graphics library provides a single set of primitives that can
 * render to multiple output devices. A function is called to create a context
 * from a given output device, then any primitive may render to that context.
 */

/*!
 * \defgroup pgfx PGFX Graphics Interface
 *
 * The PGFX graphics library provides a single set of primitives that can
 * render to multiple output devices. A function is called to create a context
 * from a given output device, then any primitive may render to that context.
 *
 * \{
 */

/************ Data structures */

/*!
 * \defgroup pgfxdata Data Structures
 *
 * A few of the PGFX defined data structures, like pgcolor and pgu, will be
 * useful to any client that uses PGFX. Most of these however should be
 * for internal use only.
 *
 * \{
 */


//! Unit for coordinates in PGFX 
typedef int pgu;

/*! 
 * \brief RGB hardware-independant color
 * 
 * The format is 24-bit RGB, similar to that used by HTML.
 * The following are some example colors:
 * \code
#define BLACK   0x000000
#define WHITE   0xFFFFFF
#define GREY    0x808080
#define RED     0xFF0000
#define GREEN   0x00FF00
#define BLUE    0x0000FF
#define YELLOW  0xFFFF00
 * \endcode
 * 
 * Video drivers may define other formats that are selected by
 * setting a bit in the color's high byte. For example, in text-mode
 * drivers, a high byte set to 0x20 would indicate a raw character code.
 * Using the  driver, the expression (0x20000F00 | 'A') would be
 * a capital "A" with a white foreground and black background.
 * These formats are video-driver dependant, and under normal circumstances
 * the high byte should always be zero.
 */
typedef unsigned long pgcolor;

/*!
 * \brief Reference to a stored primitive
 * 
 * If the underlying output device is one that stores primitives, for example
 * the canvas in PGFX_PERSISTENT mode, this number can be used to refer to 
 * the primitive later.
 * 
 * Currently this functionality is unimplemented in PGFX. To manipulate
 * primitives in the canvas widget the pgWriteCmd interface must be used.
 *
 * \sa pgWriteCmd
 */
typedef int pgprim;


/*!
 * \brief Rendering context
 * 
 * This defines a rendering method and a device to output to.
 * It may be a canvas, bitmap, or any other device.
 * 
 * The members of this structure are not to be used by the client,
 * only the PGFX rendering backend. Their values are defined by
 * the rendering functions, and modification or interpretation by
 * the client may have unexpected results.
 * 
 * \sa pgNewCanvasContext, pgDeleteContext
 */
typedef struct pgfx_context {
   struct pgfx_lib *lib;   //!< Pointers to the rendering functions 
   pghandle device;        //!< Output device (canvas, bitmap, etc.)
   pgcolor color;          //!< Current color
   pgu cx;                 //!< Current x position for moveto/lineto
   pgu cy;                 //!< Current y position for moveto/lineto
   unsigned long flags;    //!< Backend-defined
   int sequence;           //!< Backend-defined
} *pgcontext;


/*!
 * \brief Defines the rendering backend
 *
 * pgfx_lib defines the set of functions a PGFX backend needs to implement.
 *
 * \sa pgPixel, pgLine, pgRect, pgFrame, pgSlab, pgBar, pgText, pgBitmap, pgTileBitmap, pgGradient
 */
struct pgfx_lib {
   //! Implementation of pgPixel
   pgprim (*pixel)     (pgcontext c, pgu x,  pgu y);
   //! Implementation of pgLine
   pgprim (*line)      (pgcontext c, pgu x1, pgu y1, pgu x2, pgu y2);
   //! Implementation of pgRect
   pgprim (*rect)      (pgcontext c, pgu x,  pgu y,  pgu w,  pgu h);
   //! Implementation of pgFrame
   pgprim (*frame)     (pgcontext c, pgu x,  pgu y,  pgu w,  pgu h);
   //! Implementation of pgSlab
   pgprim (*slab)      (pgcontext c, pgu x,  pgu y,  pgu w);
   //! Implementation of pgBar
   pgprim (*bar)       (pgcontext c, pgu x,  pgu y,  pgu h);
  //! Implementation of pgEllipse 
   pgprim (*ellipse)   (pgcontext c, pgu x,  pgu y,  pgu w,  pgu h); 
   //! Implementation of pgFEllipse 
   pgprim (*fellipse)  (pgcontext c, pgu x,  pgu y,  pgu w,  pgu h);  
   //! Implementation of pgFPolygon 
   pgprim (*fpolygon)  (pgcontext c, pghandle array);   
   //! Implementation of pgText
   pgprim (*text)      (pgcontext c, pgu x,  pgu y,  pghandle string);
   //! Implementation of pgBitmap
   pgprim (*bitmap)    (pgcontext c, pgu x,  pgu y,  pgu w,  pgu h,
			pghandle bitmap);
   //! Implementation of pgTileBitmap
   pgprim (*tilebitmap)(pgcontext c, pgu x,  pgu y,  pgu w,  pgu h,
			pghandle bitmap);
   //! Implementation of pgGradient
   pgprim (*gradient)  (pgcontext c, pgu x,  pgu y,  pgu w,  pgu h,
			pgu angle, pgcolor c1, pgcolor c2);

   /* Nonvisual primitives */
   
   //! Implementation of pgSetColor nonvisual primitive
   pgprim (*setcolor)(pgcontext c, pgcolor color);
   //! Implementation of pgSetFont nonvisual primitive
   pgprim (*setfont)(pgcontext c, pghandle font);
   //! Implementation of pgSetLgop nonvisual primitive
   pgprim (*setlgop)(pgcontext c, short lgop);
   //! Implementation of pgSetAngle nonvisual primitive
   pgprim (*setangle)(pgcontext c, pgu angle);              /* For text */
   //! Implementation of pgSetSrc nonvisual primitive
   pgprim (*setsrc)(pgcontext c, pgu x,pgu y,pgu w,pgu h);  /* For bitmaps */
   //! Implementation of pgSetMapping nonvisual primitive
   pgprim (*setmapping)(pgcontext c, pgu x,pgu y,pgu w,pgu h,short type);
   
   //! Implementation of pgUpdate
   void (*update)(pgcontext c);
   
   /* FIXME: add functions to manipulate grops after they're created 
    * also update documentation when this is done. */
};

//! \}

/************ Primitives */

/*!
 * \defgroup pgfxprim PGFX Primitives
 *
 * These functions all render their respective graphics primitive to
 * the supplied pgcontext.
 *
 * \{
 */

//! Plot a single pixel in the current color
inline pgprim pgPixel(pgcontext c,pgu x,pgu y);
//! Plot a line between two specified coordinate pairs
inline pgprim pgLine(pgcontext c,pgu x1,pgu y1,pgu x2,pgu y2);
//! Draw a filled rectangle in the current color
inline pgprim pgRect(pgcontext c,pgu x,pgu y,pgu w,pgu h);
//! Draw a non-filled rectangle in the current color
inline pgprim pgFrame(pgcontext c,pgu x,pgu y,pgu w,pgu h);
//! Draw a horizontal line beginning at \p (x,y) and extending right \p w pixels
inline pgprim pgSlab(pgcontext c,pgu x,pgu y,pgu w);
//! Draw a vertical line beginning at \p (x,y) and extending down \p h pixels
inline pgprim pgBar(pgcontext c,pgu x,pgu y,pgu h);
//! Draw a non-filled ellipse in the current color
inline pgprim pgEllipse(pgcontext c,pgu x,pgu y,pgu w,pgu h);
//! Draw a filled ellipse in the current color
inline pgprim pgFEllipse(pgcontext c,pgu x,pgu y,pgu w,pgu h);
//! Draw a filled polygon in the current color 
inline pgprim pgFPolygon(pgcontext c, pghandle array); 
/*! 
 * \brief Draw a string in the current color, font, and angle
 * 
 * The upper-left corner of the string is drawn at the given (x,y)
 * coordinates. If the string is rotated with pgSetAngle the origin is
 * at the corresponding point on the rotated text.
 * 
 * By default PicoGUI inserts a small font-dependant gap between the
 * specified coordinates and the actual edge of the text. To override this,
 * create a font with the PG_FSTYLE_FLUSH flag set.
 * 
 * \sa pgNewString, pgNewFont, pgSizeText, pgSetColor, pgSetFont, pgSetAngle
 */
inline pgprim pgText(pgcontext c,pgu x,pgu y,pghandle string);
/*!
 * \brief Draw a bitmap
 * 
 * If the specified width and height are larger than the bitmap, the bitmap
 * will be tiled. The (x,y) coordinate on the source bitmap that is mapped
 * to the destination (x,y) is determined by the x any y components of the
 * source rectangle.
 * 
 * \sa pgSetLgop, pgSetSrc, pgTileBitmap
 */
inline pgprim pgBitmap(pgcontext c,pgu x,pgu y,pgu w,pgu h,pghandle bitmap);
/*!
 * \brief Tile a portion of a bitmap
 * 
 * pgBitmap will automatically tile the source bitmap if the width and height
 * are sufficiently large. However, pgBitmap tiles the entire bitmap.
 * pgTileBitmap tiles a portion of the bitmap, defined by the current source
 * rectangle.
 * 
 * \sa pgSetLgop, pgSetSrc, pgBitmap
 */
inline pgprim pgTileBitmap(pgcontext c,pgu x,pgu y,pgu w,pgu h,pghandle bitmap);
/*!
 * \brief Render a linear color gradient
 *
 * \param angle Angle to rotate gradient, in degrees
 * 
 * This function fills the specified rectangle with a linear gradient between
 * colors \p c1 and \p c2.
 */
inline pgprim pgGradient(pgcontext c,pgu x,pgu y,pgu w,pgu h,
			 pgu angle,pgcolor c1,pgcolor c2);
//! Set the current color
inline pgprim pgSetColor(pgcontext c,pgcolor color);
//! Set the current font
inline pgprim pgSetFont(pgcontext c,pghandle font);
/*!
 * \brief Set the current logical operation
 *
 * The logical operation ("LGOP" for short) defines how
 * the the color of a drawn primitive is combined with the
 * color already on the screen.
 * 
 *  - PG_LGOP_NULL: Do not render the primitive
 *  - PG_LGOP_NONE: Render the primitive normally
 *  - PG_LGOP_OR: Bitwise 'or' the primitive with existing pixels
 *  - PG_LGOP_AND: Bitwise 'and' the primitive with existing pixels
 *  - PG_LGOP_XOR: Bitwise exclusive 'or' the primitive with existing pixels
 *  - PG_LGOP_INVERT: Invert all bits in the primitive's pixels
 *  - PG_LGOP_INVERT_OR: Invert, then 'or'
 *  - PG_LGOP_INVERT_AND: Invert, then 'and'
 *  - PG_LGOP_INVERT_XOR: Invert, then exclusive 'or'
 *  - PG_LGOP_SUBTRACT: Subtract primitive from existing pixels, clamping to black
 *  - PG_LGOP_ADD: Add primitive to existing pixels, clamping to white
 *  - PG_LGOP_MULTIPLY: Multiply the primitive by existing pixels, treating white as 1 and black as 0
 *  - PG_LGOP_STIPPLE: Only set pixels within a checkerboard pattern, for creating dotted lines or a 'grey' effect
 */
inline pgprim pgSetLgop(pgcontext c,short lgop);
/*!
 * \brief Set the angle for text
 * 
 * The angle is measured in degrees:
 *  - 0: Horizontal, left to right
 *  - 90: Vertical, bottom to top
 *  - 180: Upside down
 *  - 270: Vertical, top to bottom
 */ 
inline pgprim pgSetAngle(pgcontext c,pgu angle);
/*!
 * \brief Set the bitmap source rectangle
 *
 * The bitmap source rectangle is selects which piece of
 * the source bitmap to use in bitmap primitives
 * 
 * \sa pgBitmap, pgTileBitmap
 */
inline pgprim pgSetSrc(pgcontext c,pgu x,pgu y,pgu w,pgu h);
/*!
 * \brief Set coordinate system mapping
 *
 * This function defines how the coordinates in a primitive are converted
 * to device coordinates. The following types are supported:
 * 
 *  - PG_MAP_NONE: Primitives are in device coordinates, the supplied rectangle is ignored
 *  - PG_MAP_SCALE: The width and height supplied indicate the virtual size of the device, 
 *    these are stretched to the device's actual size
 * 
 * PG_MAP_SCALE is particularly useful in conjunction with PGFX_PERSISTENT and the canvas widget.
 * When the canvas is resized, its contents will stretch automatically with no client-side
 * intervention.
 * 
 */
inline pgprim pgSetMapping(pgcontext c,pgu x,pgu y,pgu w,pgu h,short type);
/*! 
 * \brief Draws any undrawn primitives
 * 
 * This must be called after a drawing primitives to force them to actually appear.
 * It will flush any buffers necessary and instructs the server to draw changed
 * areas of the screen if necessary.
 * 
 */
inline void pgContextUpdate(pgcontext c);

//! Sets a position to draw lines from
void    pgMoveTo(pgcontext c, pgu x, pgu y);
//! Draws a line from the position last set with pgMoveTo or pgLineTo
pgprim  pgLineTo(pgcontext c, pgu x, pgu y);

//! \}

/************ Constants */

/*!
 * \defgroup pgfxctx Rendering Context Management
 *
 * This section contains functions and constants to create and delete
 * PGFX rendering contexts. Note that rendering contexts themselves are
 * client-side objects (as are all objects without handles) and they are not
 * automatically cleaned up with handle contexts. You must explicitely use
 * pgDeleteContext().
 *
 * \{
 */


/*!
 * \brief PGFX canvas immediate mode
 * 
 * All primitives are rendered, but not stored. Note that this includes
 * nonvisual primitives like pgSetColor, pgSetMapping, and pgSetLgop.
 * 
 * For a slightly lower-level explanation, this mode sets the default gropnode
 * flags to PG_GROPF_TRANSIENT so that gropnodes are deleted immediately
 * after rendering.
 *
 * \sa pgWriteCmd, PG_WIDGET_CANVAS, PG_GROPF_TRANSIENT, pgNewCanvasContext
 */
#define PGFX_IMMEDIATE    1
/*!
 * \brief PGFX canvas persistent mode
 * 
 * Primitives are rendered and stored in the PicoGUI server. This allows
 * automatic redrawing by the server without client intervention.
 * Primitives can be manipulated after being sent to the server.
 * 
 * \sa pgWriteCmd, PG_WIDGET_CANVAS, pgSetMapping, pgNewCanvasContext
 */
#define PGFX_PERSISTENT   2

/************ Context management functions */

/*!
 * \brief Create a context for rendering to a canvas
 * \param canvas Handle to a canvas widget
 * \param mode Rendering mode: PGFX_IMMEDIATE or PGFX_PERSISTENT
 * \returns A pgcontext that can be used with PGFX primitives. It must be deleted with pgDeleteContext
 * 
 * \sa PGFX_IMMEDIATE, PGFX_PERSISTENT, PG_WIDGET_CANVAS, pgDeleteContext
 */
pgcontext pgNewCanvasContext(pghandle canvas,short mode);

/*!
 * \brief Create a context for rendering to a bitmap
 * \param bitmap Handle to a bitmap
 * \returns A pgcontext that can be used with PGFX primitives. It must be deleted with pgDeleteContext
 * 
 * \p bitmap may be zero to render directly to the display if your application
 * has registered for exclusive display access.
 *
 * \sa pgDeleteContext, pgNewBitmap, pgCreateBitmap, PG_OWN_DISPLAY, pgRegisterOwner
 */
pgcontext pgNewBitmapContext(pghandle bitmap);

//! Delete a PGFX context
void pgDeleteContext(pgcontext c);

//! \}
//! \}

#endif /* _H_PG_PGFX */

/* The End */
